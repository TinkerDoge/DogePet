#include "include/Motion.h"

// Static member initialization
MPU6050 Motion::mpu;
bool Motion::ready = false;
bool Motion::logEnabled = true;
bool Motion::dmpReady = false;

uint8_t Motion::mpuIntStatus;
uint8_t Motion::devStatus;
uint16_t Motion::packetSize;
uint8_t Motion::fifoBuffer[64];

Quaternion Motion::q;
VectorInt16 Motion::aa;
VectorInt16 Motion::aaReal;
VectorInt16 Motion::aaWorld;
VectorFloat Motion::gravity;
float Motion::ypr[3];

int16_t Motion::lastAx = 0, Motion::lastAy = 0, Motion::lastAz = 0;
int16_t Motion::lastGx = 0, Motion::lastGy = 0, Motion::lastGz = 0;
uint32_t Motion::lastLogMs = 0;

void Motion::init() {
    Wire.begin();
    Wire.setClock(400000); 
    
    mpu.initialize();
    Serial.println("{\"status\":\"info\",\"msg\":\"Initializing MPU6050...\"}");
    
    if (!mpu.testConnection()) {
        Serial.println("{\"status\":\"error\",\"msg\":\"MPU6050 connection test failed\"}");
        ready = false;
        return;
    }
    
    // Initialize DMP
    devStatus = mpu.dmpInitialize();
    
    // Supply gyro offsets (scaled for min sensitivity)
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788); 

    if (devStatus == 0) {
        // Turn on DMP
        mpu.setDMPEnabled(true);
        mpuIntStatus = mpu.getIntStatus();
        dmpReady = true;
        packetSize = mpu.dmpGetFIFOPacketSize();
        ready = true;
        Serial.println("{\"status\":\"ok\",\"msg\":\"DMP Ready\"}");
    } else {
        // ERROR!
        ready = false;
        dmpReady = false;
        Serial.printf("{\"status\":\"error\",\"msg\":\"DMP Init Failed: %d\"}\n", devStatus);
    }
}

void Motion::calibrate() {
    if (!ready || !dmpReady) return;
    
    // DMP usually auto-calibrates over time if configured.
    // For now, we trust the offsets set in init().
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
}

void Motion::getRawData(int16_t& ax, int16_t& ay, int16_t& az, int16_t& gx, int16_t& gy, int16_t& gz) {
    if (!ready) {
        ax = ay = az = gx = gy = gz = 0;
        return;
    }
    // Even over DMP, we can usually read raw registers
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
}

bool Motion::isReady() {
    return ready;
}

void Motion::setLogEnabled(bool enabled) {
    logEnabled = enabled;
}

void Motion::update() {
    if (!dmpReady) return;
    
    // Check FIFO count
    uint16_t fifoCount = mpu.getFIFOCount();
    
    if (fifoCount == 1024) {
        // FIFO overflow - reset and skip this cycle
        mpu.resetFIFO();
        return;
    }
    
    // Wait for complete packet(s)
    if (fifoCount < packetSize) return;
    
    // Drain all but the latest packet to stay current
    while (fifoCount >= packetSize * 2) {
        mpu.getFIFOBytes(fifoBuffer, packetSize);
        fifoCount -= packetSize;
    }
    
    // Read the latest packet
    mpu.getFIFOBytes(fifoBuffer, packetSize);
    
    // Get Quaternion from DMP
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    
    // Rate limit logging (DMP runs fast, ~100Hz internal)
    uint32_t now = millis();
    if (logEnabled && (now - lastLogMs >= MIN_LOG_INTERVAL_MS)) {
        // Output JSON with 4 decimal places
        Serial.printf("{\"type\":\"dmp\",\"w\":%.4f,\"x\":%.4f,\"y\":%.4f,\"z\":%.4f}\n", 
                      q.w, q.x, q.y, q.z);
        
        lastLogMs = now;
    }
}
