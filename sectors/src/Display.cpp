// Display sector implementation: RoboEyes wrapper
#include "Display.h"
#include "FluxGarage_RoboEyes.h"

extern Adafruit_SH1106G* oled_display; // Global display object

namespace Sectors { namespace Display {

void begin(int screenW, int screenH, int eyeDistance) {
  if (oled_display) {
    RoboEyes.begin(oled_display, screenW, screenH, eyeDistance);
  }
}

void update() {
  RoboEyes.update();
}

void setMood(uint8_t mood) {
  RoboEyes.setMood(mood);
}

void setPosition(uint8_t position) {
  RoboEyes.setPosition(position);
}

void blink() {
  RoboEyes.blink();
}

void open() {
  RoboEyes.open();
}

void setWidth(int leftWidth, int rightWidth) {
  RoboEyes.setWidth(leftWidth, rightWidth);
}

void setHeight(int leftHeight, int rightHeight) {
  RoboEyes.setHeight(leftHeight, rightHeight);
}

void setEyeSize(int leftWidth, int rightWidth, int leftHeight, int rightHeight) {
  RoboEyes.setEyeSize(leftWidth, rightWidth, leftHeight, rightHeight);
}

void setAutoblinker(bool enabled, int avgSeconds, int jitterSeconds) {
  RoboEyes.setAutoblinker(enabled, avgSeconds, jitterSeconds);
}

void setIdleMode(bool enabled, int frequency, int randomness) {
  RoboEyes.setIdleMode(enabled, frequency, randomness);
}

void setBorderradius(int leftRadius, int rightRadius) {
  RoboEyes.setBorderradius(leftRadius, rightRadius);
}

void setSpacebetween(int space) {
  RoboEyes.setSpacebetween(space);
}

void setCuriosity(bool enabled) {
  RoboEyes.setCuriosity(enabled);
}

void setAutoFlush(bool enabled) {
  RoboEyes.setAutoFlush(enabled);
}

void setViewportYMax(int maxY) {
  RoboEyes.setViewportYMax(maxY);
}

}}
