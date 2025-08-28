// Motion sector: wraps motion module and MPU6050
#pragma once
#include <Arduino.h>
#include <stdint.h>

namespace Sectors { namespace MotionSector {
  void begin();
  void update();
  
  // MPU6050 direct access functions
  void mpuInit();
  void mpuCalibrate(int samples = 200);
  void mpuReadRaw(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gx, int16_t &gy, int16_t &gz);
  void mpuRead(float &axg, float &ayg, float &azg, float &gx_dps, float &gy_dps, float &gz_dps);
}}
