#include <Wire.h>
#include <MPU9250.h>

constexpr uint32_t BAUD_RATE = 115200;
constexpr uint16_t SAMPLE_RATE_HZ = 100;
constexpr uint32_t SAMPLE_INTERVAL_US = 1000000UL / SAMPLE_RATE_HZ;
constexpr uint16_t TTL_PULSE_US = 1000;
constexpr uint8_t SYNC_PIN = 13;
constexpr uint8_t IMU_I2C_ADDRESS = 0x68;
constexpr uint32_t PACKET_MAGIC = 0x31554D49UL;  // "IMU1"

struct __attribute__((packed)) ImuPacket {
  uint32_t magic;
  uint32_t arduino_millis;
  uint32_t sample_index;
  float accel_x_g;
  float accel_y_g;
  float accel_z_g;
  float gyro_x_dps;
  float gyro_y_dps;
  float gyro_z_dps;
  float mag_x_mg;
  float mag_y_mg;
  float mag_z_mg;
  float roll_deg;
  float pitch_deg;
  float yaw_deg;
  float linear_accel_x_g;
  float linear_accel_y_g;
  float linear_accel_z_g;
};

MPU9250 imu;
bool streaming = false;
uint32_t sample_index = 0;
unsigned long next_sample_due_us = 0;

void emitSyncPulse() {
  digitalWrite(SYNC_PIN, HIGH);
  delayMicroseconds(TTL_PULSE_US);
  digitalWrite(SYNC_PIN, LOW);
}

void printInfo() {
  Serial.println(F("# 9-axis IMU Intan sync toolbox"));
  Serial.print(F("# baud_rate="));
  Serial.println(BAUD_RATE);
  Serial.print(F("# sample_rate_hz="));
  Serial.println(SAMPLE_RATE_HZ);
  Serial.print(F("# sync_pin="));
  Serial.println(SYNC_PIN);
  Serial.println(F("# commands: s=start, x=stop, p=pulse, i=info, c=calibrate"));
}

void printCalibrationValues() {
  Serial.println(F("CALIBRATION_HEADER,accel_bias_x_mg,accel_bias_y_mg,accel_bias_z_mg,gyro_bias_x_dps,gyro_bias_y_dps,gyro_bias_z_dps,mag_bias_x_mg,mag_bias_y_mg,mag_bias_z_mg,mag_scale_x,mag_scale_y,mag_scale_z"));
  Serial.print(F("CALIBRATION_VALUES,"));
  Serial.print(imu.getAccBiasX() * 1000.0f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
  Serial.print(',');
  Serial.print(imu.getAccBiasY() * 1000.0f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
  Serial.print(',');
  Serial.print(imu.getAccBiasZ() * 1000.0f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
  Serial.print(',');
  Serial.print(imu.getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.print(',');
  Serial.print(imu.getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.print(',');
  Serial.print(imu.getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.print(',');
  Serial.print(imu.getMagBiasX());
  Serial.print(',');
  Serial.print(imu.getMagBiasY());
  Serial.print(',');
  Serial.print(imu.getMagBiasZ());
  Serial.print(',');
  Serial.print(imu.getMagScaleX());
  Serial.print(',');
  Serial.print(imu.getMagScaleY());
  Serial.print(',');
  Serial.println(imu.getMagScaleZ());
}

void runCalibration() {
  streaming = false;
  Serial.println(F("# Accel/gyro calibration will start in 5 seconds."));
  Serial.println(F("# Keep the IMU still on a flat surface."));
  delay(5000);
  imu.verbose(true);
  imu.calibrateAccelGyro();

  Serial.println(F("# Magnetometer calibration will start in 5 seconds."));
  Serial.println(F("# Move the IMU in a figure-eight pattern until complete."));
  delay(5000);
  imu.calibrateMag();
  imu.verbose(false);
  printCalibrationValues();
}

void fillPacket(ImuPacket& packet) {
  packet.magic = PACKET_MAGIC;
  packet.arduino_millis = millis();
  packet.sample_index = sample_index;
  packet.accel_x_g = imu.getAccX();
  packet.accel_y_g = imu.getAccY();
  packet.accel_z_g = imu.getAccZ();
  packet.gyro_x_dps = imu.getGyroX();
  packet.gyro_y_dps = imu.getGyroY();
  packet.gyro_z_dps = imu.getGyroZ();
  packet.mag_x_mg = imu.getMagX();
  packet.mag_y_mg = imu.getMagY();
  packet.mag_z_mg = imu.getMagZ();
  packet.roll_deg = imu.getRoll();
  packet.pitch_deg = imu.getPitch();
  packet.yaw_deg = imu.getYaw();
  packet.linear_accel_x_g = imu.getLinearAccX();
  packet.linear_accel_y_g = imu.getLinearAccY();
  packet.linear_accel_z_g = imu.getLinearAccZ();
}

void sendPacket(const ImuPacket& packet) {
  emitSyncPulse();
  Serial.write(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
}

void handleCommand(char command) {
  switch (command) {
    case 's':
      streaming = true;
      sample_index = 0;
      next_sample_due_us = micros();
      break;
    case 'x':
      streaming = false;
      break;
    case 'p':
      emitSyncPulse();
      break;
    case 'i':
      printInfo();
      break;
    case 'c':
      runCalibration();
      break;
    default:
      break;
  }
}

void setup() {
  pinMode(SYNC_PIN, OUTPUT);
  digitalWrite(SYNC_PIN, LOW);

  Serial.begin(BAUD_RATE);
  Wire.begin();
  delay(2000);

  if (!imu.setup(IMU_I2C_ADDRESS)) {
    while (true) {
      Serial.println(F("ERROR: MPU9250 connection failed. Check SDA/SCL, power, and address."));
      delay(2000);
    }
  }

  printInfo();
}

void loop() {
  while (Serial.available() > 0) {
    handleCommand((char)Serial.read());
  }

  const bool has_new_imu_data = imu.update();
  if (!streaming || !has_new_imu_data) {
    return;
  }

  const unsigned long now_us = micros();
  if ((long)(now_us - next_sample_due_us) < 0) {
    return;
  }

  ImuPacket packet;
  fillPacket(packet);
  sendPacket(packet);
  sample_index += 1;
  next_sample_due_us += SAMPLE_INTERVAL_US;

  if ((long)(now_us - next_sample_due_us) > (long)SAMPLE_INTERVAL_US) {
    next_sample_due_us = now_us + SAMPLE_INTERVAL_US;
  }
}
