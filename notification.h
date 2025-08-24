// Notification Face Module - Header
// Handles the notification display functionality
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <Adafruit_SH110X.h>
#include "config.h"

// Shared toast overlay state
struct ToastState {
  char text[64];
  char fullText[64];
  uint32_t until;
  bool visible;
  uint32_t lastDrawMs;
  bool typewriter;
  uint8_t typePos;
  uint32_t typeSpeed;
  uint32_t lastTypeMs;
  bool scatter;
  bool noFrame;
};

// Toast helpers
void showToastTypewriter(ToastState& toast, const String& s, uint16_t ms=3000, uint32_t typeSpeed=50);
void showToast(ToastState& toast, const String& s, uint16_t ms=1200);
void showBinaryChatter(ToastState& toast);

// Toast overlay function
void drawToastIfAny(Adafruit_SH1106G& display, ToastState& toast);

// Function to draw the notification face
void drawNotif(Adafruit_SH1106G& display);

// External variables for notification text
extern char lastNotifTitle[64];
extern char lastNotifBody[128];

// Utility function to check if there are notifications
bool hasNotif();
