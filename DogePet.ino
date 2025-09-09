// Author: [Doge De Shibe]
// Description: ESP32-S3 Super Mini companion bot - Hardware Testing Version

// =============================================================================
// INCLUDES & DEPENDENCIES
// =============================================================================
#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>
#include <esp_system.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <Adafruit_NeoPixel.h>
#include <ChronosESP32.h>

// Custom headers
#include "config.h"
#include "mpu6050.h"

// MPU6050 calibration offset variables (required by mpu6050.h)
int32_t mpu_accel_off_x = 0, mpu_accel_off_y = 0, mpu_accel_off_z = 0;
int32_t mpu_gyro_off_x = 0, mpu_gyro_off_y = 0, mpu_gyro_off_z = 0;

// =============================================================================
// HARDWARE CONFIGURATION
// =============================================================================
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
ChronosESP32 chrono(BLE_DEVICE_NAME);

// TFT + LVGL setup
static TFT_eSPI tft = TFT_eSPI();
static lv_color_t lvgl_buf1[SCREEN_W * 20];
static lv_obj_t* face_eyeL = nullptr;
static lv_obj_t* face_eyeR = nullptr;
static lv_obj_t* face_pupilL = nullptr;
static lv_obj_t* face_pupilR = nullptr;
static lv_obj_t* face_eyelidL = nullptr;
static lv_obj_t* face_eyelidR = nullptr;
static lv_timer_t* face_blink_timer = nullptr;
static lv_obj_t* status_label = nullptr; // On-screen status text

// Hardware status
bool wifiConnected = false;
bool bleEnabled = true;
bool geminiReady = false;
uint32_t lastActivityMs = 0;
bool testsRan = false; // ensure auto test runs once

// =============================================================================
// SIMPLE UI FUNCTIONS
// =============================================================================
void showMessage(const String& msg) {
  Serial.print("UI: ");
  Serial.println(msg);
  if (status_label) {
    lv_label_set_text(status_label, msg.c_str());
  }
}

void updateStatusLED(uint32_t color) {
  strip.setPixelColor(0, color);
  strip.show();
}

// =============================================================================
// LVGL FACE SETUP
// =============================================================================
static void face_do_blink(lv_obj_t* eyelid) {
  int32_t eyeH = lv_obj_get_height(lv_obj_get_parent(eyelid));
  
  static lv_anim_t a_close;
  lv_anim_init(&a_close);
  lv_anim_set_var(&a_close, eyelid);
  lv_anim_set_values(&a_close, 0, eyeH);
  lv_anim_set_time(&a_close, 90);
  lv_anim_set_exec_cb(&a_close, [](void* obj, int32_t v){ lv_obj_set_height(static_cast<lv_obj_t*>(obj), v); });

  static lv_anim_t a_open;
  lv_anim_init(&a_open);
  lv_anim_set_var(&a_open, eyelid);
  lv_anim_set_values(&a_open, eyeH, 0);
  lv_anim_set_time(&a_open, 120);
  lv_anim_set_delay(&a_open, 25);
  lv_anim_set_exec_cb(&a_open, [](void* obj, int32_t v){ lv_obj_set_height(static_cast<lv_obj_t*>(obj), v); });

  lv_anim_set_ready_cb(&a_close, [](lv_anim_t* /*a*/){ lv_anim_start(&a_open); });
  lv_anim_start(&a_close);
}

static void face_blink_cb(lv_timer_t* t) {
  (void)t;
  if (face_eyelidL && face_eyelidR) {
    face_do_blink(face_eyelidL);
    face_do_blink(face_eyelidR);
  }
  uint32_t next = 1500 + (esp_random() % 2500);
  lv_timer_set_period(face_blink_timer, next);
}

static void lvgl_init_and_make_face() {
  tft.init();
  tft.setRotation(0);
  // Quick hardware test pattern to confirm TFT is alive
  tft.fillScreen(TFT_RED);   delay(150);
  tft.fillScreen(TFT_GREEN); delay(150);
  tft.fillScreen(TFT_BLUE);  delay(150);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("TFT OK", SCREEN_W/2, SCREEN_H/2 - 8, 2);
  delay(300);
  // For ST7789 240x240 on some boards an offset is needed; uncomment if your display needs it:
  // tft.setSwapBytes(true); // use if pushing 16-bit pixel arrays that are byte-swapped
  tft.fillScreen(TFT_BLACK);

  lv_init();

  lv_display_t* disp = lv_display_create(SCREEN_W, SCREEN_H);
  // Ensure LVGL uses RGB565 color depth
#if LV_COLOR_DEPTH != 16
#error "LVGL must be configured for 16-bit color (RGB565) for TFT_eSPI"
#endif
  // Optional: Set DPI for LVGL if needed
  // lv_display_set_dpi(disp, 148);
  lv_display_set_flush_cb(disp, [](lv_display_t* d, const lv_area_t* a, uint8_t* px){
    uint32_t w = (a->x2 - a->x1 + 1);
    uint32_t h = (a->y2 - a->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(a->x1, a->y1, w, h);
    // LVGL provides pixels in RGB565 native order; TFT_eSPI expects uint16_t*
    tft.pushPixels(reinterpret_cast<uint16_t*>(px), w * h);
    tft.endWrite();
    lv_display_flush_ready(d);
  });
  lv_display_set_buffers(disp, lvgl_buf1, nullptr, sizeof(lvgl_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_obj_t* root = lv_scr_act();
  lv_obj_set_style_bg_color(root, lv_color_black(), 0);

  int margin = SCREEN_W / 14;
  int eyeW   = (SCREEN_W - margin*3) / 2;
  int eyeH   = (SCREEN_H * 3) / 5;
  if (eyeH > SCREEN_H - 2*margin) eyeH = SCREEN_H - 2*margin;
  int centerY = (SCREEN_H - eyeH) / 2;

  auto make_eye = [&](bool left){
    lv_obj_t* eye = lv_obj_create(root);
    lv_obj_remove_style_all(eye);
    lv_obj_set_size(eye, eyeW, eyeH);
    lv_obj_set_style_bg_color(eye, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(eye, eyeW/2, 0);
    lv_obj_set_style_border_width(eye, 4, 0);
    lv_obj_set_style_border_color(eye, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_pos(eye, left ? margin : (SCREEN_W - margin - eyeW), centerY);

    lv_obj_t* pupil = lv_obj_create(eye);
    lv_obj_remove_style_all(pupil);
    int pupilW = eyeW / 3; int pupilH = pupilW;
    lv_obj_set_size(pupil, pupilW, pupilH);
    lv_obj_center(pupil);
    lv_obj_set_style_bg_color(pupil, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(pupil, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pupil, pupilW/2, 0);

    lv_obj_t* lid = lv_obj_create(eye);
    lv_obj_remove_style_all(lid);
    lv_obj_set_size(lid, eyeW, 0);
    lv_obj_align(lid, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(lid, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(lid, LV_OPA_COVER, 0);
    lv_obj_move_foreground(lid);
    lv_obj_set_height(lid, 0);

    if (left) { face_eyeL = eye; face_pupilL = pupil; face_eyelidL = lid; }
    else { face_eyeR = eye; face_pupilR = pupil; face_eyelidR = lid; }
  };

  make_eye(true);
  make_eye(false);
  
  if (face_eyelidL) lv_obj_set_height(face_eyelidL, 0);
  if (face_eyelidR) lv_obj_set_height(face_eyelidR, 0);

  // Create status label at the bottom for test/info messages
  status_label = lv_label_create(root);
  lv_obj_set_width(status_label, SCREEN_W - 10);
  // If long mode API isn't available in your LVGL version, it's safe to remove the next line
  lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
  lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -4);
  lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
  lv_obj_set_style_bg_color(status_label, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(status_label, LV_OPA_50, 0);
  lv_obj_set_style_pad_all(status_label, 2, 0);
  lv_obj_move_foreground(status_label);
  lv_label_set_text(status_label, "Ready");

  uint32_t first = 1500 + (esp_random() % 2000);
  face_blink_timer = lv_timer_create(face_blink_cb, first, nullptr);
}

// =============================================================================
// HARDWARE TESTING FUNCTIONS
// =============================================================================
void testWiFi() {
  showMessage("Testing WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin("YourSSID", "YourPassword"); // Replace with actual credentials
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    updateStatusLED(strip.Color(0, 255, 0)); // Green
    showMessage("WiFi Connected: " + WiFi.localIP().toString());
  } else {
    updateStatusLED(strip.Color(255, 0, 0)); // Red
    showMessage("WiFi Failed");
  }
}

void testBLE() {
  showMessage("Testing BLE...");
  chrono.begin();
  if (chrono.isRunning()) {
    updateStatusLED(strip.Color(0, 0, 255)); // Blue
    showMessage("BLE Active");
  } else {
    updateStatusLED(strip.Color(255, 0, 0)); // Red
    showMessage("BLE Failed");
  }
}

void testIMU() {
  showMessage("Testing IMU...");
  float ax, ay, az, gx, gy, gz;
  mpuRead(ax, ay, az, gx, gy, gz);
  String imuData = "IMU: ax=" + String(ax, 2) + " ay=" + String(ay, 2) + " az=" + String(az, 2);
  showMessage(imuData);
}

void testGemini() {
  showMessage("Testing Gemini...");
  if (wifiConnected) {
    // TODO: Add actual Gemini API test when module is ready
    geminiReady = true;
    showMessage("Gemini Ready (stub)");
  } else {
    showMessage("Gemini needs WiFi");
  }
}

void testHardware() {
  showMessage("=== Hardware Test Sequence ===");
  delay(1000);
  
  testIMU();
  delay(1000);
  
  testBLE();
  delay(1000);
  
  testWiFi();
  delay(1000);
  
  testGemini();
  delay(1000);
  
  showMessage("=== Test Complete ===");
}

// =============================================================================
// INPUT HANDLING
// =============================================================================
void handleInput() {
  static bool lastButton = false;
  bool button = digitalRead(FUNC_BTN) == LOW; // Active low
  
  if (button && !lastButton) {
  // Button pressed (no longer triggers tests)
  showMessage("Button pressed");
  }
  lastButton = button;
  
  // Touch sensor
  static bool lastTouch = false;
  bool touch = digitalRead(TOUCH_PIN) == HIGH;
  
  if (touch && !lastTouch) {
    showMessage("Touch detected");
    lastActivityMs = millis();
  }
  lastTouch = touch;
}

// =============================================================================
// MAIN FUNCTIONS
// =============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("DogePet Hardware Test Starting...");
  
  // Initialize pins
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);
  pinMode(FUNC_BTN, INPUT_PULLUP);
  pinMode(VBAT_PIN, INPUT);
  pinMode(TFT_BL_PIN, OUTPUT);
  // DC mistakenly wired to GPIO1 and GPIO7 on PCB; we only drive GPIO1 via TFT_eSPI
  pinMode(7, INPUT); // high-Z to avoid bus contention
  digitalWrite(TFT_BL_PIN, HIGH);
  
  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  
  // Initialize MPU6050
  mpuInit();
  mpuCalibrate(); // Calibrate on startup
  
  // Initialize ADC
  analogReadResolution(12);
  analogSetPinAttenuation(VBAT_PIN, ADC_11db);
  
  // Initialize status LED
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.setPixelColor(0, strip.Color(255, 255, 0)); // Yellow = starting
  strip.show();
  
  // Initialize display
  lvgl_init_and_make_face();
  
  // Initial status
  showMessage("Starting auto hardware test...");
  updateStatusLED(strip.Color(0, 255, 255)); // Cyan = ready

  // Run automatic test sequence
  testHardware();
  testsRan = true;

  lastActivityMs = millis();
}

void loop() {
  uint32_t currentMs = millis();
  static uint32_t lastLvTick = currentMs;
  
  // LVGL tick
  uint32_t dt = currentMs - lastLvTick;
  if (dt > 0) { 
    lv_tick_inc(dt); 
    lastLvTick = currentMs; 
  }
  
  // Handle LVGL timer
  lv_timer_handler();
  
  // Handle inputs
  handleInput();

  
  delay(10); // Small delay to prevent tight loop
}
