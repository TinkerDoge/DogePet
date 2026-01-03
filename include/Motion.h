#ifndef MOTION_H
#define MOTION_H

#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "config.h"

class Motion {
public:
    static void init();
    static void calibrate(); // Now sets DMP offsets
    static void getRawData(int16_t& ax, int16_t& ay, int16_t& az, int16_t& gx, int16_t& gy, int16_t& gz);
    static void update();           // Call in loop - checks FIFO and sends DMP data
    static void setLogEnabled(bool enabled);
    static bool isReady();

private:
    static MPU6050 mpu;
    static bool ready;
    static bool logEnabled;
    static bool dmpReady;
    
    // DMP Variables
    static uint8_t mpuIntStatus;   // Holds actual interrupt status byte from MPU
    static uint8_t devStatus;      // Return status after each device operation (0 = success, !0 = error)
    static uint16_t packetSize;    // Expected DMP packet size (default is 42 bytes)
    static uint8_t fifoBuffer[64]; // FIFO storage buffer
    
    // Orientation/Motion Variables
    static Quaternion q;           // [w, x, y, z]         Quaternion container
    static VectorInt16 aa;         // [x, y, z]            Accel sensor measurements
    static VectorInt16 aaReal;     // [x, y, z]            Gravity-free accel sensor measurements
    static VectorInt16 aaWorld;    // [x, y, z]            World-frame accel sensor measurements
    static VectorFloat gravity;    // [x, y, z]            Gravity vector
    static float ypr[3];           // [yaw, pitch, roll]   Yaw/Pitch/Roll container and gravity vector
    
    // Last logged values for change detection (kept for raw fallback)
    static int16_t lastAx, lastAy, lastAz;
    static int16_t lastGx, lastGy, lastGz;
    static uint32_t lastLogMs;
    
    // Threshold for logging (raw units)
    static constexpr int16_t ACCEL_CHANGE_THRESHOLD = 500;   
    static constexpr int16_t GYRO_CHANGE_THRESHOLD  = 100;   
    static constexpr uint32_t MIN_LOG_INTERVAL_MS   = 100;   // 10Hz updates for smooth but not spammy motion
};

#endif
