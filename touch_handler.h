#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include <Wire.h>

// GT911 Touch Controller Configuration for LilyGo T5 4.7" S3
#define TOUCH_SDA   18
#define TOUCH_SCL   17

// Screen states
enum ScreenState {
  SCREEN_MAIN = 0,
  SCREEN_CURRENT_DETAIL,
  SCREEN_FORECAST,
  SCREEN_GRAPHS,
  SCREEN_INFO,
  SCREEN_HISTORY,
  SCREEN_AIR_QUALITY,
  SCREEN_CALENDAR,
  SCREEN_CALENDAR_YEAR,
  SCREEN_INFO_FEATURES1,
  SCREEN_INFO_FEATURES2,
  SCREEN_INFO_HELP,
  SCREEN_INFO_CREDITS,
  SCREEN_CALLSIGN,
  SCREEN_BLE_CONFIG,
  SCREEN_WORLDCLOCK
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
int calendarMonthOffset = 0;   // 0 = current month, -1 = previous, +1 = next
int calendarYearOffset = 0;    // 0 = current year, -1 = previous, +1 = next

const unsigned long CALENDAR_HOLD_MS = 800;  // Hold time to activate calendar (hidden feature)

// Main screen touch zones
// Based on actual measurements: top-right corner = X~914, Y~281
// Zones are evaluated in order - first match wins
const TouchZone mainScreenZones[] = {
  // Battery/WiFi area top-right - System info (X 800-960, Y 230-330)
  {800, 230, 960, 330, SCREEN_INFO},
  // Moon/Sunrise/Sunset area (left 1/3) - Calendar (X 0-320, Y 70-150)
  {0, 70, 320, 150, SCREEN_CALENDAR},
  // Top section - current conditions (expanded to cover weather icon/wind rose)
  {0, 150, 800, 400, SCREEN_CURRENT_DETAIL},
  {800, 330, 960, 400, SCREEN_CURRENT_DETAIL},
  // Forecast icons (right 2/3 of middle section: X 320-960, Y 70-150)
  {320, 70, 960, 150, SCREEN_FORECAST},
  // Bottom section - graphs area split in two:
  // Left half -> History
  {480, 0, 960, 70, SCREEN_HISTORY},
  // Right half -> Forecast graphs
  {0, 0, 480, 70, SCREEN_GRAPHS}
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
#define SCREEN_CALENDAR_PREV 97
#define SCREEN_CALENDAR_NEXT 96
#define SCREEN_CALENDAR_YEAR_PREV 95
#define SCREEN_CALENDAR_YEAR_NEXT 94

// Check which zone was touched
ScreenState checkTouchZone(int touchX, int touchY, ScreenState currentState) {
  Serial.printf("Checking zones for touch at (%d, %d), current screen: %d\n", touchX, touchY, currentState);

  // Special handling for INFO screen - arrow to Features1 or clean/BLE buttons
  if (currentState == SCREEN_INFO) {
    // Right arrow zone (right side, Y 230-330 like calendar)
    if (touchX >= 800 && touchX <= 960 && touchY >= 230 && touchY <= 330) {
      Serial.println(">>> Info next arrow touched - going to Features 1/2");
      return SCREEN_INFO_FEATURES1;
    }
    // Bluetooth button zone: bottom left (X 50-230, Y 0-100 in touch coords)
    if (touchX >= 50 && touchX <= 230 && touchY >= 0 && touchY <= 100) {
      Serial.println(">>> Bluetooth button touched - entering BLE config");
      return SCREEN_BLE_CONFIG;
    }
    // Clean button zone: bottom center (inverted coords: X 350-630, Y 0-100)
    if (touchX >= 350 && touchX <= 630 && touchY >= 0 && touchY <= 100) {
      Serial.println(">>> Clean button touched on INFO screen");
      return (ScreenState)SCREEN_CLEAN_ACTION;
    }
    Serial.println(">>> Touch on INFO screen - returning to main");
    return SCREEN_MAIN;
  }

  // Special handling for INFO_FEATURES1 screen - arrow to Features2 or back to Info
  if (currentState == SCREEN_INFO_FEATURES1) {
    // Right arrow zone - go to Features2
    if (touchX >= 800 && touchX <= 960 && touchY >= 230 && touchY <= 330) {
      Serial.println(">>> Features1 next arrow - going to Features 2/2");
      return SCREEN_INFO_FEATURES2;
    }
    // Any other touch returns to Info
    Serial.println(">>> Touch on FEATURES1 - returning to Info");
    return SCREEN_INFO;
  }

  // Special handling for INFO_FEATURES2 screen - arrow to Help or back to Info
  if (currentState == SCREEN_INFO_FEATURES2) {
    // Right arrow zone - go to Help
    if (touchX >= 800 && touchX <= 960 && touchY >= 230 && touchY <= 330) {
      Serial.println(">>> Features2 next arrow - going to Help");
      return SCREEN_INFO_HELP;
    }
    // Any other touch returns to Info
    Serial.println(">>> Touch on FEATURES2 - returning to Info");
    return SCREEN_INFO;
  }

  // Special handling for INFO_HELP screen - arrow to Credits or back to Info
  if (currentState == SCREEN_INFO_HELP) {
    // Right arrow zone - go to Credits
    if (touchX >= 800 && touchX <= 960 && touchY >= 230 && touchY <= 330) {
      Serial.println(">>> Help next arrow - going to Credits");
      return SCREEN_INFO_CREDITS;
    }
    // Any other touch returns to Info
    Serial.println(">>> Touch on HELP - returning to Info");
    return SCREEN_INFO;
  }

  // Special handling for INFO_CREDITS screen - arrow back to Info (circular)
  if (currentState == SCREEN_INFO_CREDITS) {
    // Right arrow zone - go back to Info (circular)
    if (touchX >= 800 && touchX <= 960 && touchY >= 230 && touchY <= 330) {
      Serial.println(">>> Credits next arrow - going to Info");
      return SCREEN_INFO;
    }
    // QR Code zone - go to Callsign screen (X 520-680, Y 0-120 in touch coords - bottom of screen)
    if (touchX >= 520 && touchX <= 680 && touchY >= 0 && touchY <= 120) {
      Serial.println(">>> QR Code touched - going to Callsign screen");
      return SCREEN_CALLSIGN;
    }
    // Any other touch returns to Info
    Serial.println(">>> Touch on CREDITS - returning to Info");
    return SCREEN_INFO;
  }

  // Special handling for CALLSIGN screen
  if (currentState == SCREEN_CALLSIGN) {
    // Bottom section (subtitle area: Y_draw ~400-540 = touch Y ~0-140) -> World Clock
    if (touchY >= 0 && touchY <= 140) {
      Serial.println(">>> Callsign subtitle touched - going to World Clock");
      return SCREEN_WORLDCLOCK;
    }
    // Any other touch returns to Credits
    Serial.println(">>> Touch on CALLSIGN - returning to Credits");
    return SCREEN_INFO_CREDITS;
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

  // Special handling for CURRENT_DETAIL screen
  if (currentState == SCREEN_CURRENT_DETAIL) {
    // Bottom section below line Y=425 (inverted Y: 0-115) → Air Quality
    if (touchY >= 0 && touchY <= 115) {
      Serial.println(">>> Bottom section touched - going to Air Quality screen");
      return SCREEN_AIR_QUALITY;
    }
    // Top section → Main screen
    Serial.println(">>> Top section touched - returning to main");
    return SCREEN_MAIN;
  }

  // Special handling for AIR_QUALITY screen - any touch returns to Current Detail
  if (currentState == SCREEN_AIR_QUALITY) {
    Serial.println(">>> Touch on AIR_QUALITY screen - returning to Current Detail");
    return SCREEN_CURRENT_DETAIL;
  }

  // Special handling for CALENDAR screen - arrows near title, title for year view
  // Based on actual touch readings: title at Y_draw=25 gives touch Y~281
  if (currentState == SCREEN_CALENDAR) {
    // Title zone (center, Y 230-330 based on actual touch at 281)
    if (touchX >= 250 && touchX <= 710 && touchY >= 230 && touchY <= 330) {
      Serial.println(">>> Calendar title touched - going to year view");
      return SCREEN_CALENDAR_YEAR;
    }
    // Left arrow zone (left side, same Y range as title)
    if (touchX >= 0 && touchX <= 250 && touchY >= 230 && touchY <= 330) {
      Serial.println(">>> Calendar prev month touched");
      return (ScreenState)SCREEN_CALENDAR_PREV;
    }
    // Right arrow zone (right side, same Y range as title)
    if (touchX >= 710 && touchX <= 960 && touchY >= 230 && touchY <= 330) {
      Serial.println(">>> Calendar next month touched");
      return (ScreenState)SCREEN_CALENDAR_NEXT;
    }
    Serial.println(">>> Touch on CALENDAR screen - returning to main");
    calendarMonthOffset = 0;  // Reset offset when leaving
    return SCREEN_MAIN;
  }

  // Special handling for CALENDAR_YEAR screen - arrows for year navigation
  // Using same Y range as monthly calendar (based on actual touch readings)
  if (currentState == SCREEN_CALENDAR_YEAR) {
    // Left arrow zone - previous year
    if (touchX >= 0 && touchX <= 250 && touchY >= 230 && touchY <= 330) {
      Serial.println(">>> Calendar year prev touched");
      return (ScreenState)SCREEN_CALENDAR_YEAR_PREV;
    }
    // Right arrow zone - next year
    if (touchX >= 710 && touchX <= 960 && touchY >= 230 && touchY <= 330) {
      Serial.println(">>> Calendar year next touched");
      return (ScreenState)SCREEN_CALENDAR_YEAR_NEXT;
    }
    // Any other touch returns to monthly calendar
    Serial.println(">>> Touch on CALENDAR_YEAR screen - returning to monthly calendar");
    calendarYearOffset = 0;  // Reset offset when leaving
    return SCREEN_CALENDAR;
  }

  // Special handling for WORLDCLOCK screen - any touch returns to callsign
  if (currentState == SCREEN_WORLDCLOCK) {
    Serial.println(">>> Touch on WORLDCLOCK screen - returning to callsign");
    return SCREEN_CALLSIGN;
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

#endif // TOUCH_HANDLER_H
