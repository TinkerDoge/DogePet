// Minimal MPU6050 I2C helper
// Simple inline functions for raw sensor access
#pragma once

#include <Wire.h>
#include <stdint.h>

// MPU6050 I2C address and registers
#define MPU6050_ADDR           0x68
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_WHO_AM_I     0x75

// Calibration offsets (raw units) - defined in Motion.cpp
extern int32_t mpu_accel_off_x, mpu_accel_off_y, mpu_accel_off_z;
extern int32_t mpu_gyro_off_x, mpu_gyro_off_y, mpu_gyro_off_z;

// Write a single register
static inline void mpuWriteReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

// Read a single register
static inline uint8_t mpuReadReg(uint8_t reg) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((int)MPU6050_ADDR, 1);
    return Wire.available() ? Wire.read() : 0;
}

// Initialize MPU6050 (wake up, configure)
static inline bool mpuInit() {
    // Check WHO_AM_I register
    uint8_t whoami = mpuReadReg(MPU6050_REG_WHO_AM_I);
    // MPU6050 returns 0x68, MPU6500/9250 may return 0x70/0x71
    if (whoami != 0x68 && whoami != 0x70 && whoami != 0x71 && 
        whoami != 0x19 && whoami != 0x34 && whoami != 0x38 && whoami != 0x39) {
        return false;  // Not a recognized MPU device
    }
    
    // Wake up (clear sleep bit)
    mpuWriteReg(MPU6050_REG_PWR_MGMT_1, 0x00);
    delay(10);
    
    // Configure gyro: ±250 dps (sensitivity: 131 LSB/dps)
    mpuWriteReg(MPU6050_REG_GYRO_CONFIG, 0x00);
    
    // Configure accel: ±2g (sensitivity: 16384 LSB/g)
    mpuWriteReg(MPU6050_REG_ACCEL_CONFIG, 0x00);
    delay(50);
    
    return true;
}

// Read raw 6-axis data (accel + gyro)
static inline void mpuReadRaw(int16_t &ax, int16_t &ay, int16_t &az, 
                               int16_t &gx, int16_t &gy, int16_t &gz) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(MPU6050_REG_ACCEL_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom((int)MPU6050_ADDR, 14);
    
    if (Wire.available() >= 14) {
        uint8_t bh, bl;
        bh = Wire.read(); bl = Wire.read(); ax = (int16_t)((bh << 8) | bl);
        bh = Wire.read(); bl = Wire.read(); ay = (int16_t)((bh << 8) | bl);
        bh = Wire.read(); bl = Wire.read(); az = (int16_t)((bh << 8) | bl);
        // Skip temperature (2 bytes)
        Wire.read(); Wire.read();
        bh = Wire.read(); bl = Wire.read(); gx = (int16_t)((bh << 8) | bl);
        bh = Wire.read(); bl = Wire.read(); gy = (int16_t)((bh << 8) | bl);
        bh = Wire.read(); bl = Wire.read(); gz = (int16_t)((bh << 8) | bl);
    } else {
        ax = ay = az = gx = gy = gz = 0;
    }
}

// Calibrate sensor (device must be flat and still!)
static inline void mpuCalibrate(int samples = 200) {
    if (samples <= 0) samples = 200;
    
    int64_t sax = 0, say = 0, saz = 0, sgx = 0, sgy = 0, sgz = 0;
    
    for (int i = 0; i < samples; i++) {
        int16_t ax, ay, az, gx, gy, gz;
        mpuReadRaw(ax, ay, az, gx, gy, gz);
        sax += ax; say += ay; saz += az;
        sgx += gx; sgy += gy; sgz += gz;
        delay(5);
    }
    
    // Calculate averages
    mpu_accel_off_x = sax / samples;
    mpu_accel_off_y = say / samples;
    mpu_accel_off_z = (saz / samples) - 16384;  // Subtract 1g (device is flat)
    mpu_gyro_off_x = sgx / samples;
    mpu_gyro_off_y = sgy / samples;
    mpu_gyro_off_z = sgz / samples;
}

// Read calibrated data in physical units (g and dps)
static inline void mpuRead(float &axg, float &ayg, float &azg, 
                            float &gx_dps, float &gy_dps, float &gz_dps) {
    int16_t ax, ay, az, gx, gy, gz;
    mpuReadRaw(ax, ay, az, gx, gy, gz);
    
    // Apply calibration offsets
    int32_t axc = (int32_t)ax - mpu_accel_off_x;
    int32_t ayc = (int32_t)ay - mpu_accel_off_y;
    int32_t azc = (int32_t)az - mpu_accel_off_z;
    int32_t gxc = (int32_t)gx - mpu_gyro_off_x;
    int32_t gyc = (int32_t)gy - mpu_gyro_off_y;
    int32_t gzc = (int32_t)gz - mpu_gyro_off_z;
    
    // Convert to physical units
    // Accel: ±2g range, 16384 LSB/g
    axg = (float)axc / 16384.0f;
    ayg = (float)ayc / 16384.0f;
    azg = (float)azc / 16384.0f;
    
    // Gyro: ±250 dps range, 131 LSB/dps
    gx_dps = (float)gxc / 131.0f;
    gy_dps = (float)gyc / 131.0f;
    gz_dps = (float)gzc / 131.0f;
}
