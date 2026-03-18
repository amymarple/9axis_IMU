# 9-Axis IMU Intan Sync Toolbox

This repository is organized as a small toolbox for collecting 9-axis IMU data with an Arduino Mega, sending a TTL sync pulse to an Intan system, and logging the IMU stream to a computer with Python.

The active toolbox now has one supported Arduino sketch and one supported Python logger:

- `arduino/imu_intan_sync_toolbox/imu_intan_sync_toolbox.ino`
- `python/read_imu.py`

Legacy experiments that are not part of the supported workflow are kept under `archive/`.

## Folder Layout

- `arduino/imu_intan_sync_toolbox/`
  Arduino sketch for the MPU9250 + Intan TTL sync workflow.
- `python/read_imu.py`
  Python logger that starts the Arduino stream, reads binary packets, and saves a CSV file.
- `python/requirements.txt`
  Python dependency list.
- `3D_printing_parts/`
  3D design files for mounting the IMU as part of an electrophysiology headstage assembly.
- `archive/`
  Older notebook and unrelated servo test material.

## Hardware

This toolbox assumes:

- Arduino Mega
- MPU9250 IMU
- Intan digital input for the sync pulse

Default wiring for the Arduino Mega:

- `MPU9250 VCC -> 3.3V`
- `MPU9250 GND -> GND`
- `MPU9250 SDA -> pin 20`
- `MPU9250 SCL -> pin 21`
- `Arduino pin 13 -> Intan digital input`
- `Arduino GND -> Intan GND`

Important:

- Verify your Intan digital input voltage compatibility before directly wiring the Arduino sync pin.
- Use a level shifter if your recording setup requires it.
- A shared ground between Arduino and Intan is required for reliable TTL synchronization.

## 3D Printing Parts

The folder `3D_printing_parts/` contains the mechanical design files for incorporating the IMU into the ephys headstage setup.

Included files:

- `mouse_cam_base_final_withgyro_wall_2.5mm.stl`
- `mouse_cam_base_final_withgyro_wall.stl`
- `mouse_cam_base_final_withgyro_wall.ipt`
- `mouse_cam_base_final_withgyro-base_10deg.ipt`

File types:

- `.stl` files are ready for slicing and 3D printing.
- `.ipt` files are Autodesk Inventor part files for editing the design.

Design purpose:

- provide a base that incorporates the IMU into the headstage assembly
- support integration with the electrophysiology headstage hardware
- make the IMU mounting reproducible across experiments

Practical note:

- Use the `.stl` files for printing the current version.
- Use the `.ipt` files if you need to modify the geometry for a different headstage, cable routing, wall thickness, or sensor placement.

## Arduino Behavior

The sketch:

- reads the MPU9250 at a fixed target rate of `100 Hz`
- emits a `1 ms` TTL pulse on Arduino pin `13`
- sends one binary IMU packet for each sample
- includes `arduino_millis` and `sample_index` in every packet

The default packet fields are:

- `arduino_millis`
- `sample_index`
- `accel_x/y/z_g`
- `gyro_x/y/z_dps`
- `mag_x/y/z_mg`
- `roll/pitch/yaw_deg`
- `linear_accel_x/y/z_g`

Supported serial commands:

- `s` start streaming and reset the sample counter
- `x` stop streaming
- `p` send one test TTL pulse
- `i` print sketch information
- `c` run sensor calibration and print calibration values

## IMU Calibration

Before recording, you can calibrate the IMU by opening the serial monitor or a serial terminal and sending:

```text
c
```

During calibration, the Arduino sketch prints these instructions:

```text
# Accel/gyro calibration will start in 5 seconds.
# Keep the IMU still on a flat surface.
# Magnetometer calibration will start in 5 seconds.
# Move the IMU in a figure-eight pattern until complete.
```

Recommended calibration workflow:

- Place the IMU flat on a stable surface.
- Send the `c` command.
- Do not touch the IMU during the accelerometer and gyroscope calibration step.
- When prompted for magnetometer calibration, move the IMU through a broad figure-eight motion until the routine finishes.
- Save the printed calibration values if you want a record of that session.

## Setup

### 1. Arduino library

Install the `MPU9250` Arduino library by hideakitai in the Arduino IDE or Arduino CLI.

### 2. Python environment

Install Python dependencies:

```bash
pip install -r python/requirements.txt
```

### 3. Upload the Arduino sketch

Upload:

`arduino/imu_intan_sync_toolbox/imu_intan_sync_toolbox.ino`

## Logging Data

Example command on Windows:

```bash
python python/read_imu.py --port COM5 --output-dir data
```

Optional: stop automatically after a fixed duration:

```bash
python python/read_imu.py --port COM5 --output-dir data --duration-seconds 600
```

The logger will:

- open the serial port
- wait for the Arduino to reboot
- stop any old stream
- start a fresh stream
- align to the IMU packet header
- save data as CSV in the chosen output folder

The output CSV contains:

- computer receive time in UTC
- computer Unix time in seconds
- Arduino timestamp and sample index
- all IMU channels from the packet

## Synchronizing With Intan

For each IMU packet, the Arduino sends one TTL pulse on pin `13`. Record that digital input in Intan and align the Intan pulse train with the CSV `sample_index` and `arduino_millis` columns.

This gives you:

- a hardware sync event recorded by Intan
- the corresponding IMU sample number in the computer log

## Notes

- If you need a different sampling rate, edit `SAMPLE_RATE_HZ` in the Arduino sketch.
- If you need a different sync pin, edit `SYNC_PIN` in the Arduino sketch.
- The `archive/` folder is kept only for reference and is not part of the supported toolbox workflow.
