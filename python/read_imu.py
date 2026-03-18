#!/usr/bin/env python3
"""Read binary IMU packets from Arduino and save them to CSV."""

from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
from pathlib import Path
import struct
import sys
import time

import serial


PACKET_MAGIC = 0x31554D49
PACKET_MAGIC_BYTES = PACKET_MAGIC.to_bytes(4, byteorder="little")
PACKET_STRUCT = struct.Struct("<III15f")
CSV_HEADER = [
    "computer_time_utc",
    "computer_unix_time_s",
    "arduino_millis",
    "sample_index",
    "accel_x_g",
    "accel_y_g",
    "accel_z_g",
    "gyro_x_dps",
    "gyro_y_dps",
    "gyro_z_dps",
    "mag_x_mg",
    "mag_y_mg",
    "mag_z_mg",
    "roll_deg",
    "pitch_deg",
    "yaw_deg",
    "linear_accel_x_g",
    "linear_accel_y_g",
    "linear_accel_z_g",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Read IMU data from Arduino and save it to CSV."
    )
    parser.add_argument("--port", required=True, help="Serial port, for example COM5.")
    parser.add_argument(
        "--baudrate",
        type=int,
        default=115200,
        help="Serial baud rate. Must match the Arduino sketch.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("data"),
        help="Directory where the CSV file will be saved.",
    )
    parser.add_argument(
        "--basename",
        default="imu_session",
        help="Prefix used for the CSV filename.",
    )
    parser.add_argument(
        "--duration-seconds",
        type=float,
        default=None,
        help="Optional maximum acquisition duration in seconds.",
    )
    parser.add_argument(
        "--timeout-seconds",
        type=float,
        default=2.0,
        help="Serial timeout in seconds.",
    )
    parser.add_argument(
        "--startup-delay-seconds",
        type=float,
        default=2.5,
        help="Delay after opening the serial port to allow Arduino reboot.",
    )
    parser.add_argument(
        "--print-every",
        type=int,
        default=100,
        help="Print one status update every N packets.",
    )
    parser.add_argument(
        "--flush-every",
        type=int,
        default=100,
        help="Flush the CSV file every N packets.",
    )
    return parser.parse_args()


def build_output_path(output_dir: Path, basename: str) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    return output_dir / f"{basename}_{timestamp}.csv"


def send_command(ser: serial.Serial, command: str) -> None:
    ser.write(command.encode("ascii"))
    ser.flush()


def align_to_packet_header(ser: serial.Serial) -> bytes:
    window = bytearray()
    while True:
        byte = ser.read(1)
        if not byte:
            raise TimeoutError("Timed out while waiting for IMU packet header.")

        window += byte
        if len(window) > len(PACKET_MAGIC_BYTES):
            window = window[-len(PACKET_MAGIC_BYTES) :]

        if bytes(window) == PACKET_MAGIC_BYTES:
            remainder = ser.read(PACKET_STRUCT.size - len(PACKET_MAGIC_BYTES))
            if len(remainder) != PACKET_STRUCT.size - len(PACKET_MAGIC_BYTES):
                raise TimeoutError("Timed out while reading the rest of the IMU packet.")
            return PACKET_MAGIC_BYTES + remainder


def read_packet(ser: serial.Serial) -> tuple[object, ...]:
    packet = ser.read(PACKET_STRUCT.size)
    if len(packet) != PACKET_STRUCT.size:
        raise TimeoutError("Timed out while reading an IMU packet.")

    if packet[: len(PACKET_MAGIC_BYTES)] != PACKET_MAGIC_BYTES:
        packet = align_to_packet_header(ser)

    unpacked = PACKET_STRUCT.unpack(packet)
    if unpacked[0] != PACKET_MAGIC:
        raise ValueError("Corrupted packet: bad magic value.")
    return unpacked


def packet_to_row(packet: tuple[object, ...]) -> list[object]:
    now_utc = datetime.now(timezone.utc)
    return [
        now_utc.isoformat(),
        f"{now_utc.timestamp():.6f}",
        packet[1],
        packet[2],
        *packet[3:],
    ]


def main() -> int:
    args = parse_args()
    output_path = build_output_path(args.output_dir, args.basename)

    with serial.Serial(
        port=args.port,
        baudrate=args.baudrate,
        timeout=args.timeout_seconds,
    ) as ser, output_path.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(CSV_HEADER)

        time.sleep(args.startup_delay_seconds)
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        send_command(ser, "x")
        time.sleep(0.1)
        ser.reset_input_buffer()
        send_command(ser, "s")

        packet_count = 0
        start_time = time.monotonic()
        first_packet = align_to_packet_header(ser)
        packet = PACKET_STRUCT.unpack(first_packet)
        writer.writerow(packet_to_row(packet))
        packet_count += 1

        print(f"Logging IMU data to {output_path}")
        try:
            while True:
                if (
                    args.duration_seconds is not None
                    and time.monotonic() - start_time >= args.duration_seconds
                ):
                    break

                packet = read_packet(ser)
                writer.writerow(packet_to_row(packet))
                packet_count += 1

                if packet_count % args.flush_every == 0:
                    csv_file.flush()

                if packet_count % args.print_every == 0:
                    elapsed = time.monotonic() - start_time
                    rate = packet_count / elapsed if elapsed > 0 else 0.0
                    print(
                        "packets="
                        f"{packet_count} sample_index={packet[2]} arduino_millis={packet[1]} "
                        f"rate={rate:.2f} Hz"
                    )
        except KeyboardInterrupt:
            print("Stopping acquisition on keyboard interrupt.")
        finally:
            send_command(ser, "x")
            csv_file.flush()

    print(f"Saved {packet_count} packets to {output_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
