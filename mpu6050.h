// Minimal MPU6050 helper (pullout from main sketch)
#ifndef DAISAI_MPU6050_H
#define DAISAI_MPU6050_H

#include <Wire.h>

// MPU6050 I2C address and registers
#define MPU6050_ADDR 0x68
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B

// calibration offsets (raw units)
extern int32_t mpu_accel_off_x, mpu_accel_off_y, mpu_accel_off_z;
extern int32_t mpu_gyro_off_x, mpu_gyro_off_y, mpu_gyro_off_z;

static inline void mpuWriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

static inline uint8_t mpuReadReg(uint8_t reg) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((int)MPU6050_ADDR, 1);
  return Wire.available() ? Wire.read() : 0;
}

static inline void mpuInit() {
  mpuWriteReg(MPU6050_REG_PWR_MGMT_1, 0x00);
  mpuWriteReg(MPU6050_REG_GYRO_CONFIG, 0x00);
  mpuWriteReg(MPU6050_REG_ACCEL_CONFIG, 0x00);
  delay(50);
}

static inline void mpuReadRaw(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gx, int16_t &gy, int16_t &gz) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_REG_ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom((int)MPU6050_ADDR, 14);
  if (Wire.available() >= 14) {
    uint8_t bh = Wire.read(); uint8_t bl = Wire.read(); ax = (int16_t)((bh<<8) | bl);
    bh = Wire.read(); bl = Wire.read(); ay = (int16_t)((bh<<8) | bl);
    bh = Wire.read(); bl = Wire.read(); az = (int16_t)((bh<<8) | bl);
    uint8_t temp_h = Wire.read(); uint8_t temp_l = Wire.read(); (void)temp_h; (void)temp_l;
    bh = Wire.read(); bl = Wire.read(); gx = (int16_t)((bh<<8) | bl);
    bh = Wire.read(); bl = Wire.read(); gy = (int16_t)((bh<<8) | bl);
    bh = Wire.read(); bl = Wire.read(); gz = (int16_t)((bh<<8) | bl);
  } else {
    ax=ay=az=gx=gy=gz=0;
  }
}

static inline void mpuCalibrate(int samples=200) {
  // Prevent division by zero
  if (samples <= 0) samples = 200;

  int64_t sax=0, say=0, saz=0, sgx=0, sgy=0, sgz=0;
  for (int i=0;i<samples;i++) {
    int16_t ax,ay,az,gx,gy,gz;
    mpuReadRaw(ax,ay,az,gx,gy,gz);
    sax += ax; say += ay; saz += az;
    sgx += gx; sgy += gy; sgz += gz;
    delay(5);
  }
  int32_t ax_av = sax / samples;
  int32_t ay_av = say / samples;
  int32_t az_av = saz / samples;
  int32_t gx_av = sgx / samples;
  int32_t gy_av = sgy / samples;
  int32_t gz_av = sgz / samples;
  mpu_accel_off_x = ax_av;
  mpu_accel_off_y = ay_av;
  mpu_accel_off_z = az_av - 16384;
  mpu_gyro_off_x = gx_av;
  mpu_gyro_off_y = gy_av;
  mpu_gyro_off_z = gz_av;
}

static inline void mpuRead(float &axg, float &ayg, float &azg, float &gx_dps, float &gy_dps, float &gz_dps) {
  int16_t ax,ay,az,gx,gy,gz;
  mpuReadRaw(ax,ay,az,gx,gy,gz);
  int32_t axc = (int32_t)ax - mpu_accel_off_x;
  int32_t ayc = (int32_t)ay - mpu_accel_off_y;
  int32_t azc = (int32_t)az - mpu_accel_off_z;
  int32_t gxc = (int32_t)gx - mpu_gyro_off_x;
  int32_t gyc = (int32_t)gy - mpu_gyro_off_y;
  int32_t gzc = (int32_t)gz - mpu_gyro_off_z;
  axg = (float)axc / 16384.0f;
  ayg = (float)ayc / 16384.0f;
  azg = (float)azc / 16384.0f;
  gx_dps = (float)gxc / 131.0f;
  gy_dps = (float)gyc / 131.0f;
  gz_dps = (float)gzc / 131.0f;
}

#endif // DAISAI_MPU6050_H
