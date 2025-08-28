// Display sector: RoboEyes and display management
#pragma once
#include <Arduino.h>
#include <stdint.h>

// Forward declaration
class Adafruit_SH1106G;

namespace Sectors { namespace Display {
  // RoboEyes management
  void begin(int screenW, int screenH, int eyeDistance);
  void update(); // Call regularly to drive eye animations
  
  // Eye control functions
  void setMood(uint8_t mood); // DEFAULT=0, TIRED=1, ANGRY=2, HAPPY=3
  void setPosition(uint8_t position); // N=1, NE=2, E=3, SE=4, S=5, SW=6, W=7, NW=8
  void blink();
  void open();
  void setWidth(int leftWidth, int rightWidth);
  void setHeight(int leftHeight, int rightHeight);
  void setEyeSize(int leftWidth, int rightWidth, int leftHeight, int rightHeight);
  
  // Eye behavior settings
  void setAutoblinker(bool enabled, int avgSeconds, int jitterSeconds);
  void setIdleMode(bool enabled, int frequency, int randomness);
  void setBorderradius(int leftRadius, int rightRadius);
  void setSpacebetween(int space);
  void setCuriosity(bool enabled);
  
  // Display control
  void setAutoFlush(bool enabled);
  void setViewportYMax(int maxY);
  
  // RoboEyes constants for external use
  static constexpr uint8_t MOOD_DEFAULT = 0;
  static constexpr uint8_t MOOD_TIRED = 1;
  static constexpr uint8_t MOOD_ANGRY = 2;
  static constexpr uint8_t MOOD_HAPPY = 3;
  
  static constexpr uint8_t POS_N = 1;  // north, top center
  static constexpr uint8_t POS_NE = 2; // north-east, top right
  static constexpr uint8_t POS_E = 3;  // east, middle right
  static constexpr uint8_t POS_SE = 4; // south-east, bottom right
  static constexpr uint8_t POS_S = 5;  // south, bottom center
  static constexpr uint8_t POS_SW = 6; // south-west, bottom left
  static constexpr uint8_t POS_W = 7;  // west, middle left
  static constexpr uint8_t POS_NW = 8; // north-west, top left
}}
