#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include <Wire.h>

// GT911 Touch Controller Configuration for LilyGo T5 4.7" S3
#define TOUCH_SDA   18
#define TOUCH_SCL   17
#define TOUCH_INT   47
#define TOUCH_RST   -1

// Screen states
enum ScreenState {
  SCREEN_MAIN = 0,
  SCREEN_CURRENT_DETAIL,
  SCREEN_FORECAST,
  SCREEN_GRAPHS,
  SCREEN_INFO,
  SCREEN_HISTORY
};

// Touch zone structure
struct TouchZone {
  int x1, y1, x2, y2;
  ScreenState targetScreen;
};

// Global state variables (RTC memory persists through deep sleep)
RTC_DATA_ATTR ScreenState currentScreen = SCREEN_MAIN;
unsigned long lastTouchTime = 0;
unsigned long navigationStartTime = 0;
bool navigationModeActive = false;
bool historyShowWeek = false;  // false = 48h, true = 1 week

const unsigned long SCREEN_TIMEOUT_MS = 30000;      // 30 seconds
const unsigned long MAX_NAVIGATION_TIME_MS = 300000; // 5 minutes max

// Main screen touch zones
// Touch X and Y are both inverted: X=0 is physical right, Y=540 is physical top
const TouchZone mainScreenZones[] = {
  // Battery/WiFi area top-right - System info (inverted: X 0-360, Y 480-540)
  {0, 480, 360, 540, SCREEN_INFO},
  // Moon icon area - System info (X 810-960, Y 180-300)
  {810, 180, 960, 300, SCREEN_INFO},
  // Physical top 40% - current conditions (excluding battery area)
  {360, 180, 960, 540, SCREEN_CURRENT_DETAIL},
  {0, 180, 360, 480, SCREEN_CURRENT_DETAIL},
  {810, 300, 960, 540, SCREEN_CURRENT_DETAIL},
  // Physical middle 30% - 5-day forecast (Y 90-180)
  {0, 90, 960, 180, SCREEN_FORECAST},
  // Physical bottom 30% - graphs area split in two:
  // Right half (physical) = X 0-480 (inverted) -> Forecast graphs
  {0, 0, 480, 90, SCREEN_GRAPHS},
  // Left half (physical) = X 480-960 (inverted) -> Real history
  {480, 0, 960, 90, SCREEN_HISTORY}
};
const int NUM_MAIN_ZONES = sizeof(mainScreenZones) / sizeof(mainScreenZones[0]);

// GT911 Touch data
struct TouchPoint {
  int x;
  int y;
  bool touched;
};

TouchPoint lastTouch = {0, 0, false};

// GT911 I2C Addresses
#define GT911_ADDR1 0x5D
#define GT911_ADDR2 0x14
uint8_t gt911_addr = GT911_ADDR1;

// GT911 Registers
#define GT911_PRODUCT_ID    0x8140
#define GT911_TOUCH_STATUS  0x814E
#define GT911_POINT1_X_LOW  0x8150

bool touchInitialized = false;

// Initialize touch controller
bool InitializeTouch() {
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  Wire.setClock(400000);

  // Try first address
  Wire.beginTransmission(GT911_ADDR1);
  if (Wire.endTransmission() == 0) {
    gt911_addr = GT911_ADDR1;
    touchInitialized = true;
    Serial.println("GT911 found at address 0x5D");
    return true;
  }

  // Try second address
  Wire.beginTransmission(GT911_ADDR2);
  if (Wire.endTransmission() == 0) {
    gt911_addr = GT911_ADDR2;
    touchInitialized = true;
    Serial.println("GT911 found at address 0x14");
    return true;
  }

  Serial.println("GT911 touch controller not found!");
  touchInitialized = false;
  return false;
}

// Write to GT911 register
void gt911WriteRegister(uint16_t reg, uint8_t value) {
  Wire.beginTransmission(gt911_addr);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  Wire.write(value);
  Wire.endTransmission();
}

// Read from GT911 register
uint8_t gt911ReadRegister(uint16_t reg) {
  Wire.beginTransmission(gt911_addr);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  Wire.endTransmission(false);
  Wire.requestFrom(gt911_addr, (uint8_t)1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}

// Screen dimensions for coordinate mapping
#define TOUCH_SCREEN_WIDTH  960
#define TOUCH_SCREEN_HEIGHT 540
#define TOUCH_PANEL_MAX_Y   960  // Touch panel Y range is larger than screen

// Touch sensitivity settings
unsigned long lastValidTouch = 0;
const unsigned long TOUCH_DEBOUNCE_MS = 500;    // Minimum time between touches (ms)
const unsigned long TOUCH_MIN_DURATION_MS = 50; // Minimum touch duration to be valid (ms)
unsigned long touchStartTime = 0;
bool touchInProgress = false;

// Read touch point with coordinate correction and validation
bool readTouchPoint(TouchPoint &point) {
  if (!touchInitialized) return false;

  uint8_t status = gt911ReadRegister(GT911_TOUCH_STATUS);
  uint8_t touchCount = status & 0x0F;

  // Clear status first
  gt911WriteRegister(GT911_TOUCH_STATUS, 0);

  // Validate touch: must have buffer ready flag AND valid touch count (1-5)
  if ((status & 0x80) && touchCount > 0 && touchCount <= 5) {
    // Read first touch point coordinates
    Wire.beginTransmission(gt911_addr);
    Wire.write(GT911_POINT1_X_LOW >> 8);
    Wire.write(GT911_POINT1_X_LOW & 0xFF);
    Wire.endTransmission(false);
    Wire.requestFrom(gt911_addr, (uint8_t)4);

    if (Wire.available() >= 4) {
      uint8_t xLow = Wire.read();
      uint8_t xHigh = Wire.read();
      uint8_t yLow = Wire.read();
      uint8_t yHigh = Wire.read();

      int rawX = (xHigh << 8) | xLow;
      int rawY = (yHigh << 8) | yLow;

      // Validate coordinates are within reasonable range
      if (rawX < 0 || rawX > 960 || rawY < 0 || rawY > 960) {
        touchInProgress = false;
        point.touched = false;
        return false;
      }

      // Track touch duration - start timing when touch begins
      if (!touchInProgress) {
        touchInProgress = true;
        touchStartTime = millis();
        point.touched = false;
        return false;  // Wait for minimum duration
      }

      // Check minimum touch duration
      if (millis() - touchStartTime < TOUCH_MIN_DURATION_MS) {
        point.touched = false;
        return false;  // Still waiting for minimum duration
      }

      // Debounce - ignore rapid repeated touches
      if (millis() - lastValidTouch < TOUCH_DEBOUNCE_MS) {
        point.touched = false;
        return false;
      }

      // Map coordinates: swap X/Y, scale Y
      point.x = rawY;  // Raw Y becomes screen X
      point.y = (rawX * TOUCH_SCREEN_HEIGHT) / TOUCH_PANEL_MAX_Y;  // Raw X becomes screen Y, scaled (no inversion)

      // Clamp to valid range
      if (point.x < 0) point.x = 0;
      if (point.x > TOUCH_SCREEN_WIDTH) point.x = TOUCH_SCREEN_WIDTH;
      if (point.y < 0) point.y = 0;
      if (point.y > TOUCH_SCREEN_HEIGHT) point.y = TOUCH_SCREEN_HEIGHT;

      point.touched = true;
      lastValidTouch = millis();
      touchInProgress = false;

      Serial.printf("Raw: %d,%d -> Mapped: %d,%d\n", rawX, rawY, point.x, point.y);
      return true;
    }
  }

  // No touch detected - reset touch tracking
  touchInProgress = false;
  point.touched = false;
  return false;
}

// Special return values for actions
#define SCREEN_CLEAN_ACTION 99
#define SCREEN_HISTORY_TOGGLE 98

// Check which zone was touched
ScreenState checkTouchZone(int touchX, int touchY, ScreenState currentState) {
  Serial.printf("Checking zones for touch at (%d, %d), current screen: %d\n", touchX, touchY, currentState);

  // Special handling for INFO screen - check clean button zone (bottom area)
  if (currentState == SCREEN_INFO) {
    // Clean button zone: bottom center (inverted coords: X 300-660, Y 0-100)
    if (touchX >= 300 && touchX <= 660 && touchY >= 0 && touchY <= 100) {
      Serial.println(">>> Clean button touched on INFO screen");
      return (ScreenState)SCREEN_CLEAN_ACTION;
    }
    Serial.println(">>> Touch on INFO screen - returning to main");
    return SCREEN_MAIN;
  }

  // Special handling for HISTORY screen - toggle zone at top right
  if (currentState == SCREEN_HISTORY) {
    // Toggle button zone: top right (X 800-960, Y 200-350)
    if (touchX >= 800 && touchX <= 960 && touchY >= 200 && touchY <= 350) {
      Serial.println(">>> History toggle button touched");
      return (ScreenState)SCREEN_HISTORY_TOGGLE;
    }
    Serial.println(">>> Touch on HISTORY screen - returning to main");
    return SCREEN_MAIN;
  }

  // Any touch on other secondary screens returns to main
  if (currentState != SCREEN_MAIN) {
    Serial.println(">>> Touch on secondary screen - returning to main");
    return SCREEN_MAIN;
  }

  // Check main screen zones
  for (int i = 0; i < NUM_MAIN_ZONES; i++) {
    Serial.printf("Zone %d: (%d,%d) to (%d,%d) -> screen %d\n",
                  i, mainScreenZones[i].x1, mainScreenZones[i].y1,
                  mainScreenZones[i].x2, mainScreenZones[i].y2,
                  mainScreenZones[i].targetScreen);
    if (touchX >= mainScreenZones[i].x1 && touchX <= mainScreenZones[i].x2 &&
        touchY >= mainScreenZones[i].y1 && touchY <= mainScreenZones[i].y2) {
      Serial.printf(">>> Zone %d MATCHED -> Screen %d\n", i, mainScreenZones[i].targetScreen);
      return mainScreenZones[i].targetScreen;
    }
  }
  Serial.println("No zone matched");

  return currentState;  // No change
}

// Handle touch events
bool pollTouch() {
  TouchPoint point;
  if (readTouchPoint(point)) {
    lastTouch = point;
    lastTouchTime = millis();
    Serial.printf("Touch detected at: %d, %d\n", point.x, point.y);
    return true;
  }
  return false;
}

// Check for timeout on secondary screens
bool checkNavigationTimeout() {
  if (currentScreen != SCREEN_MAIN) {
    unsigned long elapsed = millis() - lastTouchTime;
    if (elapsed > SCREEN_TIMEOUT_MS) {
      Serial.println("Navigation timeout - returning to main screen");
      return true;
    }
    // Safety timeout
    if (millis() - navigationStartTime > MAX_NAVIGATION_TIME_MS) {
      Serial.println("Max navigation time exceeded - returning to main screen");
      return true;
    }
  }
  return false;
}

#endif // TOUCH_HANDLER_H
