// ESP32 Weather Display and a LilyGo EPD 4.7" Display, obtains Open Weather Map data, decodes and then displays it.
// This software, the ideas and concepts is Copyright (c) David Bird 2021. All rights to this software are reserved.
// Mods by XE1E
// #################################################################################################################

// @note      Arduino Setting 
//            Tools ->
//                  Board:"ESP32S3 Dev Module"
//                  USB CDC On Boot:"Enable"
//                  USB DFU On Boot:"Disable"
//                  Flash Size : "16MB(128Mb)"
//                  Flash Mode"QIO 80MHz
//                  Partition Scheme:"16M Flash(3M APP/9.9MB FATFS)"
//                  PSRAM:"OPI PSRAM"
//                  Upload Mode:"UART0/Hardware CDC"
//                  USB Mode:"Hardware CDC and JTAG"
//            Board Manager ->
//                  esp32 by Espressif Systems 2.0.17
//            Libraries ->
//                  EPD47-master https://github.com/DFRobotdl/EPD47/archive/refs/heads/master.zip
//                  Arduinojson by Benoit Blanchon 6.19.0
//                  put only this 2 libraries in libraries folder
//            To force upload mode ->
//                  Press and hold the BOOT(IO0) button
//                  While still pressing the BOOT(IO0) button, press RST
//                  Release the RST
//                  Release the BOOT(IO0) button
//
//

#include <Arduino.h>            // In-built
#include <esp_task_wdt.h>       // In-built
#include "freertos/FreeRTOS.h"  // In-built
#include "freertos/task.h"      // In-built
#include "epd_driver.h"         // https://github.com/Xinyuan-LilyGO/LilyGo-EPD47
#include "esp_adc_cal.h"        // In-built

#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson
#include <HTTPClient.h>         // In-built

#include <WiFi.h>               // In-built
#include <SPI.h>                // In-built
#include <time.h>               // In-built
#include <FFat.h>               // In-built - for weather history storage (uses FATFS partition)

#include "owm_credentials.h"
#include "wifi_manager.h"       // AP mode and captive portal
#include "forecast_record.h"
#include "weather_history.h"    // SPIFFS weather history storage
#include "lang.h"
#include "sunset.h"
#include "sunrise.h"
#include "moon.h"

#define SCREEN_WIDTH   EPD_WIDTH
#define SCREEN_HEIGHT  EPD_HEIGHT

//################  VERSION  ##################################################
String version = "2.5 / 4.7in";  // Programme version, see change log at end
//################ VARIABLES ##################################################

enum alignment {LEFT, RIGHT, CENTER};
#define White         0xFF
#define LightGrey     0xBB
#define Grey          0x88
#define DarkGrey      0x44
#define Black         0x00

#define autoscale_on  true
#define autoscale_off false
#define barchart_on   true
#define barchart_off  false

boolean LargeIcon   = true;
boolean SmallIcon   = false;
#define Small  8            // For icon drawing
#define Large  20           // For icon drawing
#define XLarge 25           // Extra large for secondary screen
int iconScaleOverride = 0;  // 0 = use default, >0 = use this scale
String  Time_str = "--:--:--";
String  Date_str = "-- --- ----";
int     wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0, vref = 1100;

// Unit conversion constants
#define HPA_TO_INHG 0.02953
#define MM_TO_INCHES 0.0393701

//################ PROGRAM VARIABLES and OBJECTS ##########################################
#define max_readings 40 // 5-day forecast (8 readings per day x 5 days)

Forecast_record_type  WxConditions[1];
Forecast_record_type  WxForecast[max_readings];

float pressure_readings[max_readings]    = {0};
float temperature_readings[max_readings] = {0};
float humidity_readings[max_readings]    = {0};
float rain_readings[max_readings]        = {0};
float snow_readings[max_readings]        = {0};

long SleepDuration   = 10; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int  WakeupHour      = 8;  // Don't wakeup until after 07:00 to save battery power
int  SleepHour       = 1; // Sleep after 01:00 to save battery power
long StartTime       = 0;
long SleepTimer      = 0;
long Delta           = 30; // ESP32 rtc speed compensation, prevents display at xx:59:yy and then xx:00:yy (one minute later) to save power

// AP Mode state
bool inAPMode = false;
const bool FORCE_AP_MODE = false;  // Set to true to force AP mode on next boot
unsigned long apModeStartTime = 0;
const unsigned long AP_RETRY_TIMEOUT = 60000;  // 1 minute timeout to retry WiFi
const int AP_MAX_RETRIES = 5;  // Max WiFi connection attempts before permanent sleep
int apRetryCount = 0;

//fonts
#include "opensans8b.h"
#include "opensans9b.h"
#include "opensans10b.h"
#include "opensans12b.h"
#include "opensans14b.h"
#include "opensans16b.h"
#include "opensans18b.h"
#include "opensans24b.h"

// Touch navigation (screen functions defined later in this file)
#include "touch_handler.h"

// Forward declarations for screen functions
void DisplayCurrentDetailScreen();
void DisplayForecastScreen();
void DisplayGraphsScreen();
void DisplayInfoScreen();
void DisplayHistoryScreen();
void DisplayWeather();
void DisplayConditionsSection(int x, int y, String IconName, bool IconSize);
void deepCleanDisplay();
String WindDegToOrdinalDirection(float winddirection);
String TitleCase(String text);
String ConvertUnixTime(int unix_time);
void DrawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, float DataArray[], int readings, boolean auto_scale, boolean barchart_mode, int title_x_offset = 0, int hours_span = 0);
void DrawHistoryGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, String unit, float DataArray[], int readings, boolean auto_scale, boolean barchart_mode, int hours_span, boolean weekly_view, int start_hour);
float SumOfPrecip(float DataArray[], int readings);
void drawString(int x, int y, String text, alignment align);
void drawScreenHeader();
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color);
void setFont(GFXfont const &font);
void edp_update();

GFXfont  currentFont;
uint8_t *framebuffer;

void BeginSleep() {
  // Don't sleep while navigating secondary screens
  if (navigationModeActive) {
    Serial.println("Navigation active - skipping sleep");
    return;
  }

  // Don't sleep while web config is being accessed
  if (isWebActive()) {
    Serial.println("Web config active - skipping sleep");
    return;
  }

  // Stop web server and WiFi before sleep
  stopWebServer();
  StopWiFi();

  // Save current screen to NVS (RTC_DATA_ATTR doesn't work reliably on ESP32-S3)
  preferences.begin("weather", false);
  preferences.putInt("lastscreen", (int)currentScreen);
  preferences.end();

  displayPowerOff();
  UpdateLocalTime();
  SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec)) + Delta; //Some ESP32 have a RTC that is too fast to maintain accurate time, so add an offset
  esp_sleep_enable_timer_wakeup(SleepTimer * 1000000LL); // in Secs, 1000000LL converts to Secs as unit = 1uSec
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Entering " + String(SleepTimer) + " (secs) of sleep time");
  Serial.println("Starting deep-sleep period...");
  esp_deep_sleep_start();  // Sleep for e.g. 30 minutes
}

boolean SetupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
  setenv("TZ", Timezone, 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset(); // Set the TZ environment variable
  delay(100);
  return UpdateLocalTime();
}

uint8_t StartWiFi() {
  IPAddress dns(1,1,1,1); // Use Cloudflare DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  // Build network list from stored config + hardcoded credentials
  WiFiCredentialEntry configNetworks[6];
  int configNetworkCount = getConfiguredNetworks(configNetworks, 3);

  // Scan for available networks
  Serial.println("\r\nScanning for WiFi networks...");
  int networksFound = WiFi.scanNetworks();
  Serial.printf("Found %d networks\n", networksFound);

  // Find the best network (highest RSSI) among configured SSIDs
  int bestNetworkIndex = -1;
  int bestRSSI = -999;
  bool useStoredConfig = false;

  for (int i = 0; i < networksFound; i++) {
    String foundSSID = WiFi.SSID(i);
    int foundRSSI = WiFi.RSSI(i);
    Serial.printf("  %s (%d dBm)\n", foundSSID.c_str(), foundRSSI);

    // First check stored config networks
    for (int j = 0; j < configNetworkCount; j++) {
      if (foundSSID == configNetworks[j].ssid) {
        Serial.printf("    -> Stored config network found!\n");
        if (foundRSSI > bestRSSI) {
          bestRSSI = foundRSSI;
          bestNetworkIndex = j;
          useStoredConfig = true;
        }
        break;
      }
    }

    // Then check hardcoded networks (lower priority)
    if (!useStoredConfig || foundRSSI > bestRSSI) {
      for (int j = 0; j < wifiNetworkCount; j++) {
        if (foundSSID == wifiNetworks[j].ssid) {
          Serial.printf("    -> Hardcoded network found!\n");
          if (foundRSSI > bestRSSI) {
            bestRSSI = foundRSSI;
            bestNetworkIndex = j;
            useStoredConfig = false;
          }
          break;
        }
      }
    }
  }

  WiFi.scanDelete(); // Free memory from scan results

  // Connect to the best network found
  const char* selectedSSID = nullptr;
  const char* selectedPass = nullptr;

  if (bestNetworkIndex >= 0) {
    if (useStoredConfig) {
      selectedSSID = configNetworks[bestNetworkIndex].ssid;
      selectedPass = configNetworks[bestNetworkIndex].password;
    } else {
      selectedSSID = wifiNetworks[bestNetworkIndex].ssid;
      selectedPass = wifiNetworks[bestNetworkIndex].password;
    }
    Serial.printf("Connecting to: %s (signal: %d dBm)\n", selectedSSID, bestRSSI);
    WiFi.begin(selectedSSID, selectedPass);

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("STA: Failed!\n");
      WiFi.disconnect(false);
      delay(200);  // Reduced from 500ms
      WiFi.begin(selectedSSID, selectedPass);
      WiFi.waitForConnectResult();
    }
  } else {
    Serial.println("No configured networks found in scan results!");
    // Try stored config first, then hardcoded as fallback
    if (configNetworkCount > 0) {
      Serial.printf("Trying stored config: %s\n", configNetworks[0].ssid);
      WiFi.begin(configNetworks[0].ssid, configNetworks[0].password);
      WiFi.waitForConnectResult();
    }
    if (WiFi.status() != WL_CONNECTED && wifiNetworkCount > 0) {
      Serial.printf("Trying hardcoded fallback: %s\n", wifiNetworks[0].ssid);
      WiFi.begin(wifiNetworks[0].ssid, wifiNetworks[0].password);
      WiFi.waitForConnectResult();
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  }
  else {
    Serial.println("WiFi connection *** FAILED ***");
  }
  return WiFi.status();
}

void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi switched Off");
}

void InitialiseSystem() {
  StartTime = millis();
  Serial.begin(115200);
  delay(200); // Reduced from 1000ms - enough for serial init
  pinMode(21, INPUT);  // SENSOP_VN
  Serial.println(String(__FILE__) + "\nStarting...");
  epd_init();
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) Serial.println("Memory alloc failed!");
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}

// Track if display is powered on for navigation
bool displayPoweredOn = false;

// Fast screen refresh - no epd_clear(), just overwrite framebuffer
void refreshScreenFast() {
  if (!displayPoweredOn) {
    epd_poweron();
    displayPoweredOn = true;
  }

  // Clear framebuffer only (fast) - no physical clear
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  switch (currentScreen) {
    case SCREEN_MAIN:
      DisplayWeather();
      break;
    case SCREEN_CURRENT_DETAIL:
      DisplayCurrentDetailScreen();
      break;
    case SCREEN_FORECAST:
      DisplayForecastScreen();
      break;
    case SCREEN_GRAPHS:
      DisplayGraphsScreen();
      break;
    case SCREEN_INFO:
      DisplayInfoScreen();
      break;
    case SCREEN_HISTORY:
      DisplayHistoryScreen();
      break;
  }

  edp_update();
}

// Full screen refresh with physical clear (use sparingly)
void refreshScreen() {
  epd_poweron();
  displayPoweredOn = true;
  epd_clear();
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  switch (currentScreen) {
    case SCREEN_MAIN:
      DisplayWeather();
      break;
    case SCREEN_CURRENT_DETAIL:
      DisplayCurrentDetailScreen();
      break;
    case SCREEN_FORECAST:
      DisplayForecastScreen();
      break;
    case SCREEN_GRAPHS:
      DisplayGraphsScreen();
      break;
    case SCREEN_INFO:
      DisplayInfoScreen();
      break;
    case SCREEN_HISTORY:
      DisplayHistoryScreen();
      break;
  }

  edp_update();
}

// Power off display
void displayPowerOff() {
  if (displayPoweredOn) {
    epd_poweroff_all();
    displayPoweredOn = false;
  }
}

// Deep clean display - multiple refresh cycles to remove ghosting
void deepCleanDisplay() {
  Serial.println("Starting deep clean...");
  epd_poweron();
  displayPoweredOn = true;

  // Do 3 cycles of black-white to clean ghosting
  for (int i = 0; i < 3; i++) {
    Serial.printf("Clean cycle %d/3\n", i + 1);
    memset(framebuffer, 0x00, EPD_WIDTH * EPD_HEIGHT / 2);  // All black
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    delay(300);
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);  // All white
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    delay(300);
  }

  epd_clear();  // Final clear
  Serial.println("Deep clean complete");

  // Redraw current screen
  refreshScreenFast();
}

// Handle touch input and screen navigation
void handleTouchNavigation() {
  if (pollTouch()) {
    lastTouchTime = millis();  // Reset timeout on any touch
    ScreenState newScreen = checkTouchZone(lastTouch.x, lastTouch.y, currentScreen);

    Serial.printf("Current screen: %d, New screen: %d\n", currentScreen, newScreen);

    // Check for special deep clean action
    if (newScreen == SCREEN_CLEAN_ACTION) {
      deepCleanDisplay();
      return;
    }

    // Check for history toggle action
    if (newScreen == SCREEN_HISTORY_TOGGLE) {
      historyShowWeek = !historyShowWeek;
      Serial.printf("History view toggled: %s\n", historyShowWeek ? "1 week" : "48 hours");
      refreshScreen();  // Full refresh to clear previous content
      return;
    }

    if (newScreen != currentScreen) {
      Serial.println("Changing screen...");
      currentScreen = newScreen;

      if (currentScreen != SCREEN_MAIN) {
        navigationModeActive = true;
        navigationStartTime = millis();
      } else {
        navigationModeActive = false;
      }

      refreshScreen();  // Full refresh for navigation
    }
  }

  // Check for timeout (configurable seconds without touch)
  unsigned long timeoutMs = config.sleep_timeout * 1000UL;
  if (millis() - lastTouchTime > timeoutMs) {
    // Don't sleep while web config is being accessed
    if (isWebActive()) {
      Serial.println("Web config active - postponing sleep");
      lastTouchTime = millis();  // Reset timeout
      return;
    }
    Serial.println("Timeout - going to sleep");
    if (!config.keep_screen_on_sleep) {
      currentScreen = SCREEN_MAIN;
    }
    navigationModeActive = false;
    refreshScreen();  // Full refresh before sleep
    displayPowerOff();
    delay(100);  // Reduced from 500ms - just enough for display to stabilize
    BeginSleep();
  }
}

void loop() {
  // Handle AP mode web server
  if (inAPMode) {
    handleAPMode();

    // Check if 1 minute has passed to retry WiFi connection
    if (millis() - apModeStartTime >= AP_RETRY_TIMEOUT) {
      Serial.println("AP mode timeout - retrying WiFi connection...");
      stopAPMode();
      inAPMode = false;

      // Try to connect to WiFi
      if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
        Serial.println("WiFi connected! Fetching weather data...");
        apRetryCount = 0;  // Reset retry counter on successful connection
        bool RxWeather  = false;
        bool RxForecast = false;
        WiFiClient client;
        byte Attempts = 1;
        while ((RxWeather == false || RxForecast == false) && Attempts <= 2) {
          if (RxWeather  == false) RxWeather  = obtainWeatherData(client, "weather");
          if (RxForecast == false) RxForecast = obtainWeatherData(client, "forecast");
          Attempts++;
        }
        if (RxWeather && RxForecast) {
          currentScreen = SCREEN_MAIN;
          epd_poweron();
          displayPoweredOn = true;
          epd_clear();
          memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
          DisplayWeather();
          edp_update();
          // Keep display on for navigation
          lastTouchTime = millis();
          navigationModeActive = true;
          return;
        }
      }

      // WiFi failed again
      apRetryCount++;
      Serial.println("WiFi retry failed (attempt " + String(apRetryCount) + "/" + String(AP_MAX_RETRIES) + ")");

      // Check if max retries reached
      if (apRetryCount >= AP_MAX_RETRIES) {
        Serial.println("Max retries reached - entering permanent deep sleep");
        Serial.println("Manual reset required to wake up");

        // Show sleep message on display
        epd_poweron();
        epd_clear();
        memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
        setFont(OpenSans12B);
        drawString(480, 200, TXT_AP_WIFI_FAILED, CENTER);
        drawString(480, 240, TXT_AP_ATTEMPTS_EXHAUSTED + " (" + String(AP_MAX_RETRIES) + ")", CENTER);
        drawString(480, 300, TXT_AP_PERMANENT_SLEEP, CENTER);
        drawString(480, 340, TXT_AP_PRESS_RESET, CENTER);
        edp_update();
        delay(2000);
        epd_poweroff();

        // Enter permanent deep sleep (no wake timer)
        esp_deep_sleep_start();
      }

      // Return to AP mode for another attempt
      inAPMode = true;
      apModeStartTime = millis();
      displayAPModeScreen();
      startAPMode();
    }
    return;
  }

  // Handle web server requests for configuration
  handleWebServer();

  // Always handle touch navigation
  handleTouchNavigation();
  delay(100);  // Reduce CPU load while polling
}

// Apply stored configuration to global variables
void applyStoredConfig() {
  if (strlen(config.api_key) > 0) {
    apikey = String(config.api_key);
  }
  if (strlen(config.city) > 0) {
    City = String(config.city);
  }
  if (strlen(config.latitude) > 0) {
    Latitude = String(config.latitude);
  }
  if (strlen(config.longitude) > 0) {
    Longitude = String(config.longitude);
  }
  if (strlen(config.language) > 0) {
    Language = String(config.language);
  }
  if (strlen(config.hemisphere) > 0) {
    Hemisphere = String(config.hemisphere);
  }
  if (strlen(config.units) > 0) {
    Units = String(config.units);
  }
  if (strlen(config.timezone) > 0) {
    Timezone = config.timezone;
  }
  if (config.gmt_offset != 0 || hasValidConfig()) {
    gmtOffset_sec = config.gmt_offset;
  }
  if (config.dst_offset != 0 || hasValidConfig()) {
    daylightOffset_sec = config.dst_offset;
  }
}

// Display AP mode info on screen
void displayAPModeScreen() {
  epd_poweron();
  epd_clear();
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  setFont(OpenSans24B);
  drawString(SCREEN_WIDTH / 2, 100, TXT_AP_WIFI_CONFIG_MODE, CENTER);

  setFont(OpenSans12B);
  drawString(SCREEN_WIDTH / 2, 180, TXT_AP_CONNECT_TO_WIFI, CENTER);

  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 220, AP_SSID, CENTER);

  setFont(OpenSans12B);
  drawString(SCREEN_WIDTH / 2, 270, TXT_AP_PASSWORD + " " + String(AP_PASSWORD), CENTER);

  drawString(SCREEN_WIDTH / 2, 330, TXT_AP_OPEN_BROWSER, CENTER);

  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 370, "http://192.168.4.1", CENTER);

  setFont(OpenSans10B);
  drawString(SCREEN_WIDTH / 2, 450, TXT_AP_DEVICE_RESTART, CENTER);

  // Show retry count if we've had failed attempts
  if (apRetryCount > 0) {
    drawString(SCREEN_WIDTH / 2, 490, TXT_AP_ATTEMPTS_REMAINING + " " + String(AP_MAX_RETRIES - apRetryCount), CENTER);
  }

  edp_update();
  epd_poweroff_all();
}

void setup() {
  InitialiseSystem();

  // Initialize touch controller
  if (InitializeTouch()) {
    Serial.println("Touch controller initialized successfully");
  } else {
    Serial.println("Warning: Touch controller not available");
  }

  // Initialize weather history (SPIFFS)
  if (initWeatherHistory()) {
    Serial.printf("Weather history: %d readings (%.1f days)\n", historyCount, getHistoryDays());
  }

  // Load stored configuration
  loadConfig();
  applyStoredConfig();

  // Load last screen from NVS (if configured to keep screen)
  if (config.keep_screen_on_sleep) {
    preferences.begin("weather", true);
    currentScreen = (ScreenState)preferences.getInt("lastscreen", SCREEN_MAIN);
    preferences.end();
  }

  // Sync sleep duration with config
  SleepDuration = config.update_interval;

  // Initialize language based on config
  initLanguage(config.language);

  // Check for forced AP mode (hardcoded flag or stored preference)
  if (FORCE_AP_MODE || forceAPMode) {
    Serial.println("Entering AP mode (forced via flag)...");
    inAPMode = true;
    apRetryCount = 0;  // Reset retry counter
    apModeStartTime = millis();
    displayAPModeScreen();
    startAPMode();
    return;  // Stay in loop() for AP mode handling
  }

  // Try to connect to WiFi
  if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
    bool WakeUp = false;
    if (WakeupHour > SleepHour)
      WakeUp = (CurrentHour >= WakeupHour || CurrentHour < SleepHour);
    else
      WakeUp = (CurrentHour >= WakeupHour && CurrentHour < SleepHour);
    if (WakeUp) {
      byte Attempts = 1;
      bool RxWeather  = false;
      bool RxForecast = false;
      WiFiClient client;   // wifi client object
      while ((RxWeather == false || RxForecast == false) && Attempts <= 2) { // Try up-to 2 times for Weather and Forecast data
        if (RxWeather  == false) RxWeather  = obtainWeatherData(client, "weather");
        if (RxForecast == false) RxForecast = obtainWeatherData(client, "forecast");
        Attempts++;
      }
      Serial.println("Received all weather data...");
      if (RxWeather && RxForecast) { // Only if received both Weather or Forecast proceed
        // Save current weather to history
        addWeatherReading(
          WxConditions[0].Temperature,
          WxConditions[0].Humidity,
          WxConditions[0].Pressure,
          WxConditions[0].Rainfall,
          WxConditions[0].Feelslike
        );

        // Start web server for configuration (WiFi stays on)
        startWebServer();

        // Only reset to main screen if not configured to keep current screen
        if (!config.keep_screen_on_sleep) {
          currentScreen = SCREEN_MAIN;
        }
        refreshScreen();    // Display the appropriate screen based on currentScreen
        // Keep display on for touch navigation

        // Initialize touch time and let loop() handle navigation
        Serial.println("Touch navigation active - 30 second timeout");
        Serial.print("Config page: http://");
        Serial.println(WiFi.localIP());
        lastTouchTime = millis();
        navigationModeActive = true;  // Enable touch handling in loop()
        return;  // Go to loop() for continuous touch handling
      }
    }
  } else {
    // WiFi connection failed - enter AP mode
    Serial.println("WiFi connection failed! Entering AP mode...");
    inAPMode = true;
    apRetryCount = 0;  // Reset retry counter
    apModeStartTime = millis();
    displayAPModeScreen();
    startAPMode();
    return;  // Stay in loop() for AP mode handling
  }

  if(digitalRead(21)) {  // SENSOP_VN button NOT pressed
    BeginSleep();
  }
}

void Convert_Readings_to_Imperial() { // Only the first 3-hours are used
  WxConditions[0].Pressure = hPa_to_inHg(WxConditions[0].Pressure);
  WxForecast[0].Rainfall   = mm_to_inches(WxForecast[0].Rainfall);
  WxForecast[0].Snowfall   = mm_to_inches(WxForecast[0].Snowfall);
}

bool DecodeWeather(WiFiClient& json, String Type) {
  Serial.print(F("\nCreating object...and "));
  DynamicJsonDocument doc(64 * 1024);                      // allocate the JsonDocument
  DeserializationError error = deserializeJson(doc, json); // Deserialize the JSON document
  if (error) {                                             // Test if parsing succeeds.
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }
  // convert it to a JsonObject
  JsonObject root = doc.as<JsonObject>();
  Serial.println(" Decoding " + Type + " data");
  if (Type == "weather") {
    // All Serial.println statements are for diagnostic purposes and some are not required, remove if not needed with //
    //WxConditions[0].lon         = root["coord"]["lon"].as<float>();              Serial.println(" Lon: " + String(WxConditions[0].lon));
    //WxConditions[0].lat         = root["coord"]["lat"].as<float>();              Serial.println(" Lat: " + String(WxConditions[0].lat));
    WxConditions[0].Main0       = root["weather"][0]["main"].as<char*>();        Serial.println("Main: " + String(WxConditions[0].Main0));
    WxConditions[0].Forecast0   = root["weather"][0]["description"].as<char*>(); Serial.println("For0: " + String(WxConditions[0].Forecast0));
    //WxConditions[0].Forecast1   = root["weather"][1]["description"].as<char*>(); Serial.println("For1: " + String(WxConditions[0].Forecast1));
    //WxConditions[0].Forecast2   = root["weather"][2]["description"].as<char*>(); Serial.println("For2: " + String(WxConditions[0].Forecast2));
    WxConditions[0].Icon        = root["weather"][0]["icon"].as<char*>();        Serial.println("Icon: " + String(WxConditions[0].Icon));
    WxConditions[0].Temperature = root["main"]["temp"].as<float>();              Serial.println("Temp: " + String(WxConditions[0].Temperature));
    WxConditions[0].Feelslike   = root["main"]["feels_like"].as<float>();        Serial.println("Feel: " + String(WxConditions[0].Feelslike));
    WxConditions[0].Pressure    = root["main"]["pressure"].as<float>();          Serial.println("Pres: " + String(WxConditions[0].Pressure));
    WxConditions[0].Humidity    = root["main"]["humidity"].as<float>();          Serial.println("Humi: " + String(WxConditions[0].Humidity));
    WxConditions[0].Low         = root["main"]["temp_min"].as<float>();          Serial.println("TLow: " + String(WxConditions[0].Low));
    WxConditions[0].High        = root["main"]["temp_max"].as<float>();          Serial.println("THig: " + String(WxConditions[0].High));
    WxConditions[0].Windspeed   = root["wind"]["speed"].as<float>();             Serial.println("WSpd: " + String(WxConditions[0].Windspeed));
    WxConditions[0].Winddir     = root["wind"]["deg"].as<float>();               Serial.println("WDir: " + String(WxConditions[0].Winddir));
    WxConditions[0].Cloudcover  = root["clouds"]["all"].as<int>();               Serial.println("CCov: " + String(WxConditions[0].Cloudcover)); // in % of cloud cover
    WxConditions[0].Visibility  = root["visibility"].as<int>();                  Serial.println("Visi: " + String(WxConditions[0].Visibility)); // in metres
    WxConditions[0].Rainfall    = root["rain"]["1h"].as<float>();                Serial.println("Rain: " + String(WxConditions[0].Rainfall));
    WxConditions[0].Snowfall    = root["snow"]["1h"].as<float>();                Serial.println("Snow: " + String(WxConditions[0].Snowfall));
    //WxConditions[0].Country     = root["sys"]["country"].as<char*>();            Serial.println("Ctry: " + String(WxConditions[0].Country));
    WxConditions[0].Sunrise     = root["sys"]["sunrise"].as<int>();              Serial.println("SRis: " + String(WxConditions[0].Sunrise));
    WxConditions[0].Sunset      = root["sys"]["sunset"].as<int>();               Serial.println("SSet: " + String(WxConditions[0].Sunset));
    WxConditions[0].Timezone    = root["timezone"].as<int>();                    Serial.println("TZon: " + String(WxConditions[0].Timezone));
  }
  if (Type == "forecast") {
    //Serial.println(json);
    Serial.print(F("\nReceiving Forecast period - ")); //------------------------------------------------
    JsonArray list                  = root["list"];
    for (byte r = 0; r < max_readings; r++) {
      Serial.println("\nPeriod-" + String(r) + "--------------");
      WxForecast[r].Dt                = list[r]["dt"].as<int>();
      WxForecast[r].Temperature       = list[r]["main"]["temp"].as<float>();              Serial.println("Temp: " + String(WxForecast[r].Temperature));
      WxForecast[r].Low               = list[r]["main"]["temp_min"].as<float>();          Serial.println("TLow: " + String(WxForecast[r].Low));
      WxForecast[r].High              = list[r]["main"]["temp_max"].as<float>();          Serial.println("THig: " + String(WxForecast[r].High));
      WxForecast[r].Pressure          = list[r]["main"]["pressure"].as<float>();          Serial.println("Pres: " + String(WxForecast[r].Pressure));
      WxForecast[r].Humidity          = list[r]["main"]["humidity"].as<float>();          Serial.println("Humi: " + String(WxForecast[r].Humidity));
      //WxForecast[r].Forecast0         = list[r]["weather"][0]["main"].as<char*>();        Serial.println("For0: " + String(WxForecast[r].Forecast0));
      //WxForecast[r].Forecast1         = list[r]["weather"][1]["main"].as<char*>();        Serial.println("For1: " + String(WxForecast[r].Forecast1));
      //WxForecast[r].Forecast2         = list[r]["weather"][2]["main"].as<char*>();        Serial.println("For2: " + String(WxForecast[r].Forecast2));
      WxForecast[r].Icon              = list[r]["weather"][0]["icon"].as<char*>();        Serial.println("Icon: " + String(WxForecast[r].Icon));
      //WxForecast[r].Description       = list[r]["weather"][0]["description"].as<char*>(); Serial.println("Desc: " + String(WxForecast[r].Description));
      //WxForecast[r].Cloudcover        = list[r]["clouds"]["all"].as<int>();               Serial.println("CCov: " + String(WxForecast[r].Cloudcover)); // in % of cloud cover
      //WxForecast[r].Windspeed         = list[r]["wind"]["speed"].as<float>();             Serial.println("WSpd: " + String(WxForecast[r].Windspeed));
      //WxForecast[r].Winddir           = list[r]["wind"]["deg"].as<float>();               Serial.println("WDir: " + String(WxForecast[r].Winddir));
      WxForecast[r].Rainfall          = list[r]["rain"]["3h"].as<float>();                Serial.println("Rain: " + String(WxForecast[r].Rainfall));
      WxForecast[r].Snowfall          = list[r]["snow"]["3h"].as<float>();                Serial.println("Snow: " + String(WxForecast[r].Snowfall));
      WxForecast[r].Period            = list[r]["dt_txt"].as<char*>();                    Serial.println("Peri: " + String(WxForecast[r].Period));
    }
    //------------------------------------------
    float pressure_trend = WxForecast[0].Pressure - WxForecast[2].Pressure; // Measure pressure slope between ~now and later
    pressure_trend = ((int)(pressure_trend * 10)) / 10.0; // Remove any small variations less than 0.1
    WxConditions[0].Trend = "=";
    if (pressure_trend > 0)  WxConditions[0].Trend = "+";
    if (pressure_trend < 0)  WxConditions[0].Trend = "-";
    if (pressure_trend == 0) WxConditions[0].Trend = "0";

    if (Units == "I") Convert_Readings_to_Imperial();
  }
  return true;
}

String ConvertUnixTime(int unix_time) {
  // Returns either '21:12  ' or ' 09:12pm' depending on Units mode
  time_t tm = unix_time;
  struct tm *now_tm = localtime(&tm);
  char output[40];
  if (Units == "M") {
    strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
  }
  else {
    strftime(output, sizeof(output), "%I:%M%P %m/%d/%y", now_tm);
  }
  return output;
}

bool obtainWeatherData(WiFiClient & client, const String & RequestType) {
  const String units = (Units == "M" ? "metric" : "imperial");
  client.stop(); // close connection before sending a new request
  HTTPClient http;
  //String uri = "/data/2.5/" + RequestType + "?q=" + City + "," + Country + "&APPID=" + apikey + "&mode=json&units=" + units + "&lang=" + Language;
  String uri = "/data/2.5/" + RequestType + "?lat=" + Latitude + "&lon=" + Longitude + "&appid=" + apikey + "&mode=json&units=" + units + "&lang=" + Language;  

  if (RequestType != "weather") {
    uri += "&cnt=" + String(max_readings);
  }
  http.begin(client, server, 80, uri); //http.begin(uri,test_root_ca); //HTTPS example connection
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    if (!DecodeWeather(http.getStream(), RequestType)) return false;
    client.stop();
    http.end();
    return true;
  }
  else
  {
    Serial.printf("connection failed, error: %s", http.errorToString(httpCode).c_str());
    client.stop();
    http.end();
    return false;
  }
  http.end();
  return true;
}

float mm_to_inches(float value_mm) {
  return MM_TO_INCHES * value_mm;
}

float hPa_to_inHg(float value_hPa) {
  return HPA_TO_INHG * value_hPa;
}

int JulianDate(int d, int m, int y) {
  int mm, yy, k1, k2, k3, j;
  yy = y - (int)((12 - m) / 10);
  mm = m + 9;
  if (mm >= 12) mm = mm - 12;
  k1 = (int)(365.25 * (yy + 4712));
  k2 = (int)(30.6001 * mm + 0.5);
  k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
  // 'j' for dates in Julian calendar:
  j = k1 + k2 + d + 59 + 1;
  if (j > 2299160) j = j - k3; // 'j' is the Julian date at 12h UT (Universal Time) For Gregorian calendar:
  return j;
}

float SumOfPrecip(float DataArray[], int readings) {
  float sum = 0;
  for (int i = 0; i <= readings; i++) {
    sum += DataArray[i];
  }
  return sum;
}

String TitleCase(String text) {
  if (text.length() > 0) {
    String temp_text = text.substring(0, 1);
    temp_text.toUpperCase();
    return temp_text + text.substring(1); // Title-case the string
  }
  else return text;
}

void DisplayWeather() {                          // 4.7" e-paper display is 960x540 resolution
  DisplayStatusSection(600, 33, wifi_signal);    // Wi-Fi signal strength and Battery voltage
  DisplayGeneralInfoSection();                   // Top line of the display
  DisplayDisplayWindSection(137, 160, WxConditions[0].Winddir, WxConditions[0].Windspeed, 100);
  DisplayAstronomySection(15, 260);              // Astronomy section: Sunrise/set, Moon phase and icon
  DisplayMainWeatherSection(305, 100);           // Centre section: Location, temperature, Weather report, Wx Symbol
  DisplayWeatherIcon(840, 170);                  // Display weather icon (scale = Large)
  DisplayFeelsLike(280, 238);                    // Feels like temperature above forecast
  DisplayForecastSection(280, 240);              // 3hr forecast boxes
}

void DisplayGeneralInfoSection() {
  setFont(OpenSans14B);
  // Draw city multiple times for darker/blacker text
  drawString(20, 13, City, LEFT);
  drawString(20, 13, City, LEFT);
  drawString(20, 13, City, LEFT);
  drawScreenHeader();
}

void DisplayWeatherIcon(int x, int y) {
  DisplayConditionsSection(x, y, WxConditions[0].Icon, LargeIcon);
}

void DisplayMainWeatherSection(int x, int y) {
  setFont(OpenSans8B);
  DisplayTemperatureSection(x, y - 40);
  DisplayForecastTextSection(x - 55, y + 30);
  DisplayPressureSection(x, y + 70, WxConditions[0].Pressure, WxConditions[0].Trend);
}

void DisplayDisplayWindSection(int x, int y, float angle, float windspeed, int Cradius) {
  arrow(x, y, Cradius - 22, angle, 18, 33); // Show wind direction on outer circle of width and length
  setFont(OpenSans8B);
  int dxo, dyo, dxi, dyi;
  drawCircle(x, y, Cradius, Black);       // Draw compass circle
  drawCircle(x, y, Cradius + 1, Black);   // Draw compass circle
  drawCircle(x, y, Cradius * 0.7, Black); // Draw compass inner circle
  for (float a = 0; a < 360; a = a + 22.5) {
    dxo = Cradius * cos((a - 90) * PI / 180);
    dyo = Cradius * sin((a - 90) * PI / 180);
    if (a == 45)  drawString(dxo + x + 15, dyo + y - 18, TXT_NE, CENTER);
    if (a == 135) drawString(dxo + x + 20, dyo + y - 2,  TXT_SE, CENTER);
    if (a == 225) drawString(dxo + x - 20, dyo + y - 2,  TXT_SW, CENTER);
    if (a == 315) drawString(dxo + x - 15, dyo + y - 18, TXT_NW, CENTER);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    drawLine(dxo + x, dyo + y, dxi + x, dyi + y, Black);
    dxo = dxo * 0.7;
    dyo = dyo * 0.7;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    drawLine(dxo + x, dyo + y, dxi + x, dyi + y, Black);
  }
  drawString(x, y - Cradius - 20,     TXT_N, CENTER);
  drawString(x, y + Cradius + 10,     TXT_S, CENTER);
  drawString(x - Cradius - 15, y - 5, TXT_W, CENTER);
  drawString(x + Cradius + 10, y - 5, TXT_E, CENTER);
  drawString(x + 3, y + 45, String(angle, 0) + "°", CENTER);
  setFont(OpenSans12B);
  drawString(x, y - 55, WindDegToOrdinalDirection(angle), CENTER);
  setFont(OpenSans24B);
  drawString(x + 3, y - 23, String(windspeed, 1), CENTER);
  setFont(OpenSans12B);
  drawString(x, y + 20, (Units == "M" ? "m/s" : "mph"), CENTER);
}

String WindDegToOrdinalDirection(float winddirection) {
  if (winddirection >= 348.75 || winddirection < 11.25)  return TXT_N;
  if (winddirection >=  11.25 && winddirection < 33.75)  return TXT_NNE;
  if (winddirection >=  33.75 && winddirection < 56.25)  return TXT_NE;
  if (winddirection >=  56.25 && winddirection < 78.75)  return TXT_ENE;
  if (winddirection >=  78.75 && winddirection < 101.25) return TXT_E;
  if (winddirection >= 101.25 && winddirection < 123.75) return TXT_ESE;
  if (winddirection >= 123.75 && winddirection < 146.25) return TXT_SE;
  if (winddirection >= 146.25 && winddirection < 168.75) return TXT_SSE;
  if (winddirection >= 168.75 && winddirection < 191.25) return TXT_S;
  if (winddirection >= 191.25 && winddirection < 213.75) return TXT_SSW;
  if (winddirection >= 213.75 && winddirection < 236.25) return TXT_SW;
  if (winddirection >= 236.25 && winddirection < 258.75) return TXT_WSW;
  if (winddirection >= 258.75 && winddirection < 281.25) return TXT_W;
  if (winddirection >= 281.25 && winddirection < 303.75) return TXT_WNW;
  if (winddirection >= 303.75 && winddirection < 326.25) return TXT_NW;
  if (winddirection >= 326.25 && winddirection < 348.75) return TXT_NNW;
  return "?";
}

void DisplayTemperatureSection(int x, int y) {
  setFont(OpenSans24B);
  drawString(x + 20, y, String(WxConditions[0].Temperature, 1) + "°    " + String(WxConditions[0].Humidity, 0) + "%", LEFT);
  setFont(OpenSans12B);
  drawString(x + 150, y + 40, String(WxConditions[0].High, 0) + "° | " + String(WxConditions[0].Low, 0) + "°", CENTER); // High and Low
}

void DisplayForecastTextSection(int x, int y) {
  setFont(OpenSans14B);
  //Wx_Description = WxConditions[0].Main0;          // e.g. typically 'Clouds'
  String Wx_Description = WxConditions[0].Forecast0; // e.g. typically 'overcast clouds' ... you choose which
  Wx_Description.replace(".", ""); // remove any '.'
  if (WxForecast[0].Rainfall > 0) Wx_Description += " (" + String(WxForecast[0].Rainfall, 1) + String((Units == "M" ? "mm" : "in")) + ")";
  int spaceRemaining = 0, p = 0, charCount = 0, Width = 30;
  while (p < Wx_Description.length()) {
    if (Wx_Description.substring(p, p + 1) == " ") spaceRemaining = p;
    if (charCount > Width - 1) { // '~' is the end of line marker
      Wx_Description = Wx_Description.substring(0, spaceRemaining) + "~" + Wx_Description.substring(spaceRemaining + 1);
      charCount = 0;
    }
    p++;
    charCount++;
  }
  String Line1 = Wx_Description.substring(0, Wx_Description.indexOf("~"));
  String Line2 = Wx_Description.substring(Wx_Description.indexOf("~") + 1);
  drawString(x + 28, y + 10, TitleCase(Line1), LEFT);
  if (Line1 != Line2) drawString(x + 28, y + 35, Line2, LEFT);
}

void DisplayPressureSection(int x, int y, float pressure, String slope) {
  setFont(OpenSans14B);
  DrawPressureAndTrend(x - 25, y + 40, pressure, slope);
  if (WxConditions[0].Visibility > 0) {
    Visibility(x + 143, y + 30, String(WxConditions[0].Visibility / 1000.0, 2) + "km");
    x += 150; // Offset if visibility shown
  }
  if (WxConditions[0].Cloudcover > 0) CloudCover(x + 163, y + 30, WxConditions[0].Cloudcover);
}

void DisplayFeelsLike(int x, int y) {
  setFont(OpenSans10B);
  String feelsText = TXT_FEELS_LIKE + ": " + String(WxConditions[0].Feelslike, 1) + "°";
  drawString(x, y, feelsText, LEFT);
}

// Find first forecast index within 1 hour of current time
int getFirstRelevantForecastIndex() {
  time_t now = time(NULL);
  for (int i = 0; i < max_readings; i++) {
    long forecastLocal = WxForecast[i].Dt + WxConditions[0].Timezone;
    // If forecast local time is within 1 hour of now or in the future
    if (forecastLocal >= now - 3600) {
      return i;
    }
  }
  return 0;
}

void DisplayForecastWeather(int x, int y, int displayPos, int dataIndex) {
  int fwidth = 95;
  x = x + fwidth * displayPos;
  DisplayConditionsSection(x + fwidth / 2, y + 90, WxForecast[dataIndex].Icon, SmallIcon);
  setFont(OpenSans10B);
  drawString(x + fwidth / 2, y + 30, String(ConvertUnixTime(WxForecast[dataIndex].Dt + WxConditions[0].Timezone).substring(0, 5)), CENTER);
  drawString(x + fwidth / 2, y + 117, String(WxForecast[dataIndex].High, 0) + "°/" + String(WxForecast[dataIndex].Low, 0) + "°", CENTER);
}


double NormalizedMoonPhase(int d, int m, int y) {
  int j = JulianDate(d, m, y);
  //Calculate approximate moon phase
  double Phase = (j + 4.867) / 29.53059;
  return (Phase - (int) Phase);
}

void DisplayAstronomySection(int x, int y) {
  setFont(OpenSans10B);
  time_t now = time(NULL);
  struct tm * now_utc  = gmtime(&now);
  drawString(x + 5, y + 95, MoonPhase(now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900, Hemisphere), LEFT);
  DrawMoonImage(x + 10, y + 23-5); // Different references!
  DrawMoon(x - 28, y - 15-5, 75, now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900, Hemisphere); // Spaced at 1/2 moon size, so 10 - 75/2 = -28
  drawString(x + 115, y + 30, ConvertUnixTime(WxConditions[0].Sunrise).substring(0, 5), LEFT); // Sunrise
  drawString(x + 115, y + 70, ConvertUnixTime(WxConditions[0].Sunset).substring(0, 5), LEFT);  // Sunset
  DrawSunriseImage(x + 180, y + 10);
  DrawSunsetImage(x + 180, y + 50);
}

void DrawMoon(int x, int y, int diameter, int dd, int mm, int yy, String hemisphere) {
  double Phase = NormalizedMoonPhase(dd, mm, yy);
  hemisphere.toLowerCase();
  if (hemisphere == "south") Phase = 1 - Phase;
  // Draw dark part of moon
  fillCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, DarkGrey);
  const int number_of_lines = 90;
  for (double Ypos = 0; Ypos <= number_of_lines / 2; Ypos++) {
    double Xpos = sqrt(number_of_lines / 2 * number_of_lines / 2 - Ypos * Ypos);
    // Determine the edges of the lighted part of the moon
    double Rpos = 2 * Xpos;
    double Xpos1, Xpos2;
    if (Phase < 0.5) {
      Xpos1 = -Xpos;
      Xpos2 = Rpos - 2 * Phase * Rpos - Xpos;
    }
    else {
      Xpos1 = Xpos;
      Xpos2 = Xpos - 2 * Phase * Rpos + Rpos;
    }
    // Draw light part of moon
    double pW1x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW1y = (number_of_lines - Ypos)  / number_of_lines * diameter + y;
    double pW2x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW2y = (number_of_lines - Ypos)  / number_of_lines * diameter + y;
    double pW3x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW3y = (Ypos + number_of_lines)  / number_of_lines * diameter + y;
    double pW4x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW4y = (Ypos + number_of_lines)  / number_of_lines * diameter + y;
    drawLine(pW1x, pW1y, pW2x, pW2y, White);
    drawLine(pW3x, pW3y, pW4x, pW4y, White);
  }
  drawCircle(x + diameter - 1, y + diameter, diameter / 2, Black);
}

String MoonPhase(int d, int m, int y, String hemisphere) {
  int c, e;
  double jd;
  int b;
  if (m < 3) {
    y--;
    m += 12;
  }
  ++m;
  c   = 365.25 * y;
  e   = 30.6  * m;
  jd  = c + e + d - 694039.09;     /* jd is total days elapsed */
  jd /= 29.53059;                        /* divide by the moon cycle (29.53 days) */
  b   = jd;                              /* int(jd) -> b, take integer part of jd */
  jd -= b;                               /* subtract integer part to leave fractional part of original jd */
  b   = jd * 8 + 0.5;                /* scale fraction from 0-8 and round by adding 0.5 */
  b   = b & 7;                           /* 0 and 8 are the same phase so modulo 8 for 0 */
  if (hemisphere == "south") b = 7 - b;
  if (b == 0) return TXT_MOON_NEW;              // New;              0%  illuminated
  if (b == 1) return TXT_MOON_WAXING_CRESCENT;  // Waxing crescent; 25%  illuminated
  if (b == 2) return TXT_MOON_FIRST_QUARTER;    // First quarter;   50%  illuminated
  if (b == 3) return TXT_MOON_WAXING_GIBBOUS;   // Waxing gibbous;  75%  illuminated
  if (b == 4) return TXT_MOON_FULL;             // Full;            100% illuminated
  if (b == 5) return TXT_MOON_WANING_GIBBOUS;   // Waning gibbous;  75%  illuminated
  if (b == 6) return TXT_MOON_THIRD_QUARTER;    // Third quarter;   50%  illuminated
  if (b == 7) return TXT_MOON_WANING_CRESCENT;  // Waning crescent; 25%  illuminated
  return "";
}

void DrawMoonImage(int x, int y) {
  Rect_t area = {
    .x = x, .y = y, .width  = moon_width, .height =  moon_height
  };
  epd_draw_grayscale_image(area, (uint8_t *) moon_data);
}

void DrawSunriseImage(int x, int y) {
  Rect_t area = {
    .x = x, .y = y, .width  = sunrise_width, .height =  sunrise_height
  };
  epd_draw_grayscale_image(area, (uint8_t *) sunrise_data);
}

void DrawSunsetImage(int x, int y) {
  Rect_t area = {
    .x = x, .y = y, .width  = sunset_width, .height =  sunset_height
  };
  epd_draw_grayscale_image(area, (uint8_t *) sunset_data);
}


void DisplayForecastSection(int x, int y) {
  const int main_screen_readings = 24;  // 3 days for main screen graphs
  int startIdx = getFirstRelevantForecastIndex();
  for(int i = 0; i < min(7, max_readings - startIdx); i++) {
    DisplayForecastWeather(x, y, i, startIdx + i);
  }
  int r = 0;
  do { // Pre-load temporary arrays with with data - because C parses by reference and remember that[1] has already been converted to I units
    if (Units == "I") pressure_readings[r] = WxForecast[r].Pressure * HPA_TO_INHG;   else pressure_readings[r] = WxForecast[r].Pressure;
    if (Units == "I") rain_readings[r]     = WxForecast[r].Rainfall * MM_TO_INCHES; else rain_readings[r]     = WxForecast[r].Rainfall;
    if (Units == "I") snow_readings[r]     = WxForecast[r].Snowfall * MM_TO_INCHES; else snow_readings[r]     = WxForecast[r].Snowfall;
    temperature_readings[r]                = WxForecast[r].Temperature;
    humidity_readings[r]                   = WxForecast[r].Humidity;
    r++;
  } while (r < max_readings);
  int gwidth = 175, gheight = 100;
  int gx = (SCREEN_WIDTH - gwidth * 4) / 5 + 8;
  int gy = (SCREEN_HEIGHT - gheight - 30);
  int gap = gwidth + gx;
  // (x,y,width,height,MinValue, MaxValue, Title, Data Array, AutoScale, ChartMode) - Using 24 readings (3 days) for main screen
  DrawGraph(gx - 10, gy, gwidth, gheight, 10, 30, Units == "M" ? TXT_TEMPERATURE_C : TXT_TEMPERATURE_F, temperature_readings, main_screen_readings, autoscale_on, barchart_off, 15);
  DrawGraph(gx + 1 * gap, gy, gwidth, gheight, 900, 1050, Units == "M" ? TXT_PRESSURE_HPA : TXT_PRESSURE_IN, pressure_readings, main_screen_readings, autoscale_on, barchart_off);
  DrawGraph(gx + 2 * gap, gy, gwidth, gheight, 0, 100,   TXT_HUMIDITY_PERCENT, humidity_readings, main_screen_readings, autoscale_off, barchart_off);
  if (SumOfPrecip(rain_readings, main_screen_readings) >= SumOfPrecip(snow_readings, main_screen_readings))
    DrawGraph(gx + 3 * gap, gy, gwidth, gheight, 0, 30, Units == "M" ? TXT_RAINFALL_MM : TXT_RAINFALL_IN, rain_readings, main_screen_readings, autoscale_on, barchart_on);
  else
    DrawGraph(gx + 3 * gap, gy, gwidth, gheight, 0, 30, Units == "M" ? TXT_SNOWFALL_MM : TXT_SNOWFALL_IN, snow_readings, main_screen_readings, autoscale_on, barchart_on);
}

void DisplayConditionsSection(int x, int y, String IconName, bool IconSize) {
  Serial.println("Icon name: " + IconName);
  if      (IconName == "01d" || IconName == "01n")  Sunny(x, y, IconSize, IconName);
  else if (IconName == "02d" || IconName == "02n")  MostlySunny(x, y, IconSize, IconName);
  else if (IconName == "03d" || IconName == "03n")  Cloudy(x, y, IconSize, IconName);
  else if (IconName == "04d" || IconName == "04n")  MostlySunny(x, y, IconSize, IconName);
  else if (IconName == "09d" || IconName == "09n")  ChanceRain(x, y, IconSize, IconName);
  else if (IconName == "10d" || IconName == "10n")  Rain(x, y, IconSize, IconName);
  else if (IconName == "11d" || IconName == "11n")  Tstorms(x, y, IconSize, IconName);
  else if (IconName == "13d" || IconName == "13n")  Snow(x, y, IconSize, IconName);
  else if (IconName == "50d")                       Haze(x, y, IconSize, IconName);
  else if (IconName == "50n")                       Fog(x, y, IconSize, IconName);
  else                                              Nodata(x, y, IconSize, IconName);
}

void arrow(int x, int y, int asize, float aangle, int pwidth, int plength) {
  float dx = (asize - 10) * cos((aangle - 90) * PI / 180) + x; // calculate X position
  float dy = (asize - 10) * sin((aangle - 90) * PI / 180) + y; // calculate Y position
  float x1 = 0;         float y1 = plength;
  float x2 = pwidth / 2;  float y2 = pwidth / 2;
  float x3 = -pwidth / 2; float y3 = pwidth / 2;
  float angle = aangle * PI / 180 - 135;
  float xx1 = x1 * cos(angle) - y1 * sin(angle) + dx;
  float yy1 = y1 * cos(angle) + x1 * sin(angle) + dy;
  float xx2 = x2 * cos(angle) - y2 * sin(angle) + dx;
  float yy2 = y2 * cos(angle) + x2 * sin(angle) + dy;
  float xx3 = x3 * cos(angle) - y3 * sin(angle) + dx;
  float yy3 = y3 * cos(angle) + x3 * sin(angle) + dy;
  fillTriangle(xx1, yy1, xx3, yy3, xx2, yy2, Black);
}

void DrawSegment(int x, int y, int o1, int o2, int o3, int o4, int o11, int o12, int o13, int o14) {
  drawLine(x + o1,  y + o2,  x + o3,  y + o4,  Black);
  drawLine(x + o11, y + o12, x + o13, y + o14, Black);
}

void DrawPressureAndTrend(int x, int y, float pressure, String slope) {
  drawString(x + 25, y - 10, String(pressure, (Units == "M" ? 0 : 1)) + (Units == "M" ? "hPa" : "in"), LEFT);
  // Arrow with shaft and head for pressure trend
  int ax = x + 8;  // Arrow center x
  int ay = y;      // Arrow center y (lowered)
  if (slope == "+") {
    // Upward arrow
    fillRect(ax - 2, ay + 2, 5, 14, Black);  // Shaft
    fillTriangle(ax, ay - 4, ax - 6, ay + 4, ax + 6, ay + 4, Black);  // Head
  }
  else if (slope == "0" || slope == "=") {
    // Rightward arrow (stable)
    fillRect(ax - 8, ay - 3, 16, 6, Black);  // Shaft
    fillTriangle(ax + 14, ay, ax + 4, ay - 8, ax + 4, ay + 8, Black);  // Head
  }
  else if (slope == "-") {
    // Downward arrow
    fillRect(ax - 2, ay - 8, 5, 14, Black);  // Shaft
    fillTriangle(ax, ay + 10, ax - 6, ay + 2, ax + 6, ay + 2, Black);  // Head
  }
}

void DisplayStatusSection(int x, int y, int rssi) {
  setFont(OpenSans10B);
  DrawRSSI(x + 295, y + 15, rssi);
  DrawBattery(x+100, y);
}

void DrawRSSI(int x, int y, int rssi) {
  int WIFIsignal = 0;
  int xpos = 1;
  for (int _rssi = -110; _rssi <= rssi; _rssi = _rssi + 20) {
    if (_rssi <= -20)  WIFIsignal = 30; // <-20dBm displays 5-bars
    if (_rssi <= -40)  WIFIsignal = 24; // -40dBm to -21dBm displays 4-bars
    if (_rssi <= -60)  WIFIsignal = 18; // -60dBm to -41dBm displays 3-bars
    if (_rssi <= -80)  WIFIsignal = 12; // -80dBm to -61dBm displays 2-bars
    if (_rssi <= -100) WIFIsignal = 6;  // -100dBm to -81dBm displays 1-bar
    if (_rssi <= -100) WIFIsignal = 6;  // -110dBm to -101dBm displays 1-bar
    fillRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
    xpos++;
  }
  // Show signal strength in dBm below bars
  setFont(OpenSans8B);
  drawString(x + 25, y + 8, String(rssi) + "dB", CENTER);
}

boolean UpdateLocalTime() {
  struct tm timeinfo;
  char   time_output[30], day_output[30], update_time[30];
  while (!getLocalTime(&timeinfo, 5000)) { // Wait for 5-sec for time to synchronise
    Serial.println("Failed to obtain time");
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
  //See http://www.cplusplus.com/reference/ctime/strftime/
  Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");      // Displays: Saturday, June 24 2017 14:05:49
  if (Units == "M") {
    sprintf(day_output, "%s, %02u %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
    strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);  // Creates: '@ 14:05:49'   and change from 30 to 8 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    sprintf(time_output, "%s", update_time);
  }
  else
  {
    strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo); // Creates  'Sat May-31-2019'
    strftime(update_time, sizeof(update_time), "%r", &timeinfo);        // Creates: '@ 02:05:49pm'
    sprintf(time_output, "%s", update_time);
  }
  Date_str = day_output;
  Time_str = time_output;
  return true;
}

void DrawBattery(int x, int y) {
  uint8_t percentage = 100;
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    vref = adc_chars.vref;
  }
  float voltage = analogRead(14) / 4096.0 * 6.566 * (vref / 1000.0); 
  Serial.println("\nVoltage = " + String(voltage));
  //voltage = 4.0; // TODO
  if (voltage > 1 ) { // Only display if there is a valid reading
    int pct = (voltage - 3.2) * 100;  // 3.2V=0%, 4.2V=100%
    if (pct > 100) pct = 100;
    if (pct < 0) pct = 0;
    percentage = pct;
    drawRect(x + 25, y - 14, 40, 15, Black);
    fillRect(x + 65, y - 10, 4, 7, Black);
    fillRect(x + 27, y - 12, 36 * percentage / 100.0, 11, Black);
    drawString(x + 85, y - 14, String(percentage) + "%  " + String(voltage, 1) + "v", LEFT);
  }
}

// Symbols are drawn on a relative 10x10grid and 1 scale unit = 1 drawing unit
void addcloud(int x, int y, int scale, int linesize) {
  fillCircle(x - scale * 3, y, scale, Black);                                                              // Left most circle
  fillCircle(x + scale * 3, y, scale, Black);                                                              // Right most circle
  fillCircle(x - scale, y - scale, scale * 1.4, Black);                                                    // left middle upper circle
  fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, Black);                                       // Right middle upper circle
  fillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, Black);                                 // Upper and lower lines
  fillCircle(x - scale * 3, y, scale - linesize, White);                                                   // Clear left most circle
  fillCircle(x + scale * 3, y, scale - linesize, White);                                                   // Clear right most circle
  fillCircle(x - scale, y - scale, scale * 1.4 - linesize, White);                                         // left middle upper circle
  fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, White);                            // Right middle upper circle
  fillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2, White); // Upper and lower lines
}

void addrain(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) {
    setFont(OpenSans8B);
    drawString(x - 25, y + 12, "///////", LEFT);
  }
  else
  {
    setFont(OpenSans18B);
    drawString(x - 60, y + 25, "///////", LEFT);
  }
}

void addsnow(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) {
    setFont(OpenSans8B);
    drawString(x - 25, y + 15, "* * * *", LEFT);
  }
  else
  {
    setFont(OpenSans18B);
    drawString(x - 60, y + 30, "* * * *", LEFT);
  }
}

void addtstorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 0; i < 5; i++) {
    drawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale, Black);
    if (scale != Small) {
      drawLine(x - scale * 4 + scale * i * 1.5 + 1, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 1, y + scale, Black);
      drawLine(x - scale * 4 + scale * i * 1.5 + 2, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 2, y + scale, Black);
    }
    drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, Black);
    if (scale != Small) {
      drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 1, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 1, Black);
      drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 2, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 2, Black);
    }
    drawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, Black);
    if (scale != Small) {
      drawLine(x - scale * 3.5 + scale * i * 1.4 + 1, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 1, y + scale * 1.5, Black);
      drawLine(x - scale * 3.5 + scale * i * 1.4 + 2, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 2, y + scale * 1.5, Black);
    }
  }
}

void addsun(int x, int y, int scale, bool IconSize) {
  int linesize = 5;
  fillRect(x - scale * 2, y, scale * 4, linesize, Black);
  fillRect(x, y - scale * 2, linesize, scale * 4, Black);
  drawLine(x - scale * 1.3, y - scale * 1.3, x + scale * 1.3, y + scale * 1.3, Black);
  drawLine(x - scale * 1.3, y + scale * 1.3, x + scale * 1.3, y - scale * 1.3, Black);
  if (IconSize == LargeIcon) {
    drawLine(1 + x - scale * 1.3, y - scale * 1.3, 1 + x + scale * 1.3, y + scale * 1.3, Black);
    drawLine(2 + x - scale * 1.3, y - scale * 1.3, 2 + x + scale * 1.3, y + scale * 1.3, Black);
    drawLine(3 + x - scale * 1.3, y - scale * 1.3, 3 + x + scale * 1.3, y + scale * 1.3, Black);
    drawLine(1 + x - scale * 1.3, y + scale * 1.3, 1 + x + scale * 1.3, y - scale * 1.3, Black);
    drawLine(2 + x - scale * 1.3, y + scale * 1.3, 2 + x + scale * 1.3, y - scale * 1.3, Black);
    drawLine(3 + x - scale * 1.3, y + scale * 1.3, 3 + x + scale * 1.3, y - scale * 1.3, Black);
  }
  fillCircle(x, y, scale * 1.3, White);
  fillCircle(x, y, scale, Black);
  fillCircle(x, y, scale - linesize, White);
}

void addfog(int x, int y, int scale, int linesize, bool IconSize) {
  if (IconSize == SmallIcon) {
    y -= 10;
    linesize = 1;
  }
  for (int i = 0; i < 6; i++) {
    fillRect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, Black);
    fillRect(x - scale * 3, y + scale * 2.0, scale * 6, linesize, Black);
    fillRect(x - scale * 3, y + scale * 2.5, scale * 6, linesize, Black);
  }
}

void Sunny(int x, int y, bool IconSize, String IconName) {
  int scale = Small, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  else y = y - 3; // Shift up small sun icon
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  scale = scale * 1.6;
  addsun(x, y, scale, IconSize);
}

void MostlySunny(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
}

void MostlyCloudy(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
}

void Cloudy(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x + 15, y - 22, scale / 2, linesize); // Cloud top right
  addcloud(x - 10, y - 18, scale / 2, linesize); // Cloud top left
  addcloud(x, y, scale, linesize);             // Main cloud
}

void Rain(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}

void ExpectRain(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}

void ChanceRain(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}

void Tstorms(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addtstorm(x, y, scale);
}

void Snow(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addsnow(x, y, scale, IconSize);
}

void Fog(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x, y - 5, scale, linesize);
  addfog(x, y - 5, scale, linesize, IconSize);
}

void Haze(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (iconScaleOverride > 0) {
    scale = iconScaleOverride;
    Offset = 45;
  } else if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addsun(x, y - 5, scale * 1.4, IconSize);
  addfog(x, y - 5, scale * 1.4, linesize, IconSize);
}

void CloudCover(int x, int y, int CCover) {
  addcloud(x - 9, y + 2+5, Small * 0.4, 2); // Cloud top left
  addcloud(x + 3, y - 2+5, Small * 0.4, 2); // Cloud top right
  addcloud(x, y + 10+5, Small * 0.7, 2); // Main cloud
  drawString(x + 30, y, String(CCover) + "%", LEFT);
}

void Visibility(int x, int y, String Visi) {
  float start_angle = 0.52, end_angle = 2.61, Offset = 10;
  int r = 16;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    drawPixel(x + r * cos(i), y - r / 2 + r * sin(i) + Offset, Black);
    drawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i) + Offset, Black);
  }
  start_angle = 3.61; end_angle = 5.78;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    drawPixel(x + r * cos(i), y + r / 2 + r * sin(i) + Offset, Black);
    drawPixel(x + r * cos(i), 1 + y + r / 2 + r * sin(i) + Offset, Black);
  }
  fillCircle(x, y + Offset, r / 4, Black);
  drawString(x + 20, y, Visi, LEFT);
}

void addmoon(int x, int y, int scale, bool IconSize) {
  if (IconSize == LargeIcon) {
    fillCircle(x - 85, y - 100, uint16_t(scale * 0.8), Black);
    fillCircle(x - 57, y - 100, uint16_t(scale * 1.6), White);
  }
  else
  {
    fillCircle(x - 28, y - 37, uint16_t(scale * 1.0), Black);
    fillCircle(x - 20, y - 37, uint16_t(scale * 1.6), White);
  }
}

void Nodata(int x, int y, bool IconSize, String IconName) {
  if (IconSize == LargeIcon) setFont(OpenSans24B); else setFont(OpenSans12B);
  drawString(x - 3, y - 10, "?", CENTER);
}

/* (C) D L BIRD
    This function will draw a graph on a ePaper/TFT/LCD display using data from an array containing data to be graphed.
    The variable 'max_readings' determines the maximum number of data elements for each array. Call it with the following parametric data:
    x_pos-the x axis top-left position of the graph
    y_pos-the y-axis top-left position of the graph, e.g. 100, 200 would draw the graph 100 pixels along and 200 pixels down from the top-left of the screen
    width-the width of the graph in pixels
    height-height of the graph in pixels
    Y1_Max-sets the scale of plotted data, for example 5000 would scale all data to a Y-axis of 5000 maximum
    data_array1 is parsed by value, externally they can be called anything else, e.g. within the routine it is called data_array1, but externally could be temperature_readings
    auto_scale-a logical value (TRUE or FALSE) that switches the Y-axis autoscale On or Off
    barchart_on-a logical value (TRUE or FALSE) that switches the drawing mode between barhcart and line graph
    barchart_colour-a sets the title and graph plotting colour
    If called with Y!_Max value of 500 and the data never goes above 500, then autoscale will retain a 0-500 Y scale, if on, the scale increases/decreases to match the data.
    auto_scale_margin, e.g. if set to 1000 then autoscale increments the scale by 1000 steps.
*/
void DrawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, float DataArray[], int readings, boolean auto_scale, boolean barchart_mode, int title_x_offset, int hours_span) {
#define auto_scale_margin 0 // Sets the autoscale increment, so axis steps up fter a change of e.g. 3
#define y_minor_axis 5      // 5 y-axis division markers
  setFont(OpenSans9B);
  int maxYscale = -10000;
  int minYscale =  10000;
  int last_x, last_y;
  float x2, y2;
  if (auto_scale == true) {
    for (int i = 1; i < readings; i++ ) {
      if (DataArray[i] >= maxYscale) maxYscale = DataArray[i];
      if (DataArray[i] <= minYscale) minYscale = DataArray[i];
    }
    maxYscale = round(maxYscale + auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Max
    Y1Max = round(maxYscale + 0.5);
    if (minYscale != 0) minYscale = round(minYscale - auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Min
    Y1Min = round(minYscale);
  }
  // Prevent division by zero - ensure Y1Max > Y1Min
  if (Y1Max <= Y1Min) {
    Y1Max = Y1Min + 10;  // Add minimum range
  }
  // Draw the graph
  last_x = x_pos + 1;
  last_y = y_pos + (Y1Max - constrain(DataArray[1], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
  drawRect(x_pos, y_pos, gwidth + 3, gheight + 2, DarkGrey);
  drawString(x_pos - 20 + gwidth / 2 + title_x_offset, y_pos - 28, title, CENTER);

  // Draw alternating background bands FIRST (very light grey)
  for (int spacing = 0; spacing < y_minor_axis; spacing++) {
    if (spacing % 2 == 1) {  // Alternate bands
      int bandY = y_pos + (gheight * spacing / y_minor_axis);
      int bandH = gheight / y_minor_axis;
      fillRect(x_pos + 1, bandY, gwidth, bandH, 0xDD);  // Very light grey background
    }
  }
  // Draw dashed horizontal grid lines
  const int number_of_dashes = 20;
  for (int spacing = 0; spacing < y_minor_axis; spacing++) {
    for (int j = 0; j < number_of_dashes; j++) {
      drawFastHLine((x_pos + 3 + j * gwidth / number_of_dashes), y_pos + (gheight * spacing / y_minor_axis), gwidth / (2 * number_of_dashes), Grey);
    }
  }
  // Draw data lines ON TOP
  for (int gx = 0; gx < readings; gx++) {
    x2 = x_pos + gx * gwidth / (readings - 1) - 1 ; // max_readings is the global variable that sets the maximum data that can be plotted
    y2 = y_pos + (Y1Max - constrain(DataArray[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1;
    if (barchart_mode) {
      // Thin bars (2px wide) like line thickness
      fillRect(x2, y2, 2, y_pos + gheight - y2 + 2, Black);
    } else {
      drawLine(last_x, last_y - 1, x2, y2 - 1, Black); // Two lines for hi-res display
      drawLine(last_x, last_y, x2, y2, Black);
    }
    last_x = x2;
    last_y = y2;
  }
  // Draw axis values
  for (int spacing = 0; spacing <= y_minor_axis; spacing++) {
    float axisValue = Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01;
    // Use 0 decimals for integer values (like 0, 20, 40, etc.)
    if (axisValue < 0.1) {
      drawString(x_pos - 10, y_pos + gheight * spacing / y_minor_axis - 5, "0", RIGHT);
    }
    else if (axisValue < 5 || title == TXT_PRESSURE_IN) {
      drawString(x_pos - 10, y_pos + gheight * spacing / y_minor_axis - 5, String(axisValue, 1), RIGHT);
    }
    else
    {
      if (Y1Min < 1 && Y1Max < 10) {
        drawString(x_pos - 3, y_pos + gheight * spacing / y_minor_axis - 5, String(axisValue, 1), RIGHT);
      }
      else {
        drawString(x_pos - 7, y_pos + gheight * spacing / y_minor_axis - 5, String(axisValue, 0), RIGHT);
      }
    }
  }
  // Day labels - calculate based on hours_span if provided, otherwise use readings
  int numDays;
  if (hours_span > 0) {
    numDays = hours_span / 24;
  } else {
    numDays = readings / 8;  // For forecast (8 readings per day = 3-hour intervals)
  }
  if (numDays < 1) numDays = 1;
  for (int i = 0; i < numDays; i++) {
    int sectionWidth = gwidth / numDays;
    int labelX = x_pos + sectionWidth * i + sectionWidth / 2;
    drawString(labelX, y_pos + gheight + 2, String(i) + "d", CENTER);
    if (i < numDays - 1) {
      drawFastVLine(x_pos + sectionWidth * (i + 1), y_pos, gheight, Grey);
    }
  }
}

// History graph - 48H shows full details, weekly shows only midnight lines
void DrawHistoryGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, String unit, float DataArray[], int readings, boolean auto_scale, boolean barchart_mode, int hours_span, boolean weekly_view, int start_hour) {
#define y_minor_axis_hist 5
  setFont(OpenSans9B);
  float maxYscale = -10000;
  float minYscale =  10000;
  int last_x, last_y;
  float x2, y2;
  bool isWeeklyView = weekly_view;

  // Calculate min/max from data
  for (int i = 0; i < readings; i++) {
    if (DataArray[i] > maxYscale) maxYscale = DataArray[i];
    if (DataArray[i] < minYscale) minYscale = DataArray[i];
  }
  float dataMin = minYscale;
  float dataMax = maxYscale;

  if (auto_scale == true) {
    Y1Max = round(maxYscale + 0.5);
    if (minYscale != 0) Y1Min = round(minYscale - 0.5);
    else Y1Min = 0;
  }
  if (Y1Max <= Y1Min) {
    Y1Max = Y1Min + 10;
  }

  // Current value and trend (only for 48H view)
  float currentValue = DataArray[readings - 1];
  float trendValue = 0;
  if (readings > 6) {
    trendValue = currentValue - DataArray[readings - 7];
  }

  // Draw graph frame
  last_x = x_pos + 1;
  last_y = y_pos + (Y1Max - constrain(DataArray[0], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
  drawRect(x_pos, y_pos, gwidth + 3, gheight + 2, DarkGrey);

  // Title - different for 48H vs weekly
  if (isWeeklyView) {
    // Simple title for weekly
    drawString(x_pos + gwidth / 2, y_pos - 28, title + " (" + unit + ")", CENTER);
  } else {
    // 48H: title with current value and trend arrow
    String titleWithCurrent = title + " ";
    if (currentValue < 10 && currentValue > -10) {
      titleWithCurrent += String(currentValue, 1);
    } else {
      titleWithCurrent += String((int)currentValue);
    }
    titleWithCurrent += unit;
    drawString(x_pos + gwidth / 2, y_pos - 28, titleWithCurrent, CENTER);

    // Draw trend arrow after title - adjust position based on title length
    int arrowX = x_pos + gwidth / 2 + 85;
    int arrowY = y_pos - 18;
    // Fine-tune per graph (compare with all language variants)
    if (title == TXT_GRAPH_TEMP) { arrowX += 15; }
    else if (title == TXT_GRAPH_PRESSURE) { arrowX += 10; arrowY -= 2; }
    else if (title == TXT_GRAPH_HUMIDITY) { arrowY -= 2; }
    else if (title == TXT_GRAPH_RAIN) { arrowY -= 2; }
    if (trendValue > 0.5) {
      // Upward arrow
      fillRect(arrowX - 1, arrowY + 2, 3, 8, Black);  // Shaft
      fillTriangle(arrowX, arrowY - 2, arrowX - 4, arrowY + 3, arrowX + 4, arrowY + 3, Black);  // Head
    } else if (trendValue < -0.5) {
      // Downward arrow
      fillRect(arrowX - 1, arrowY - 2, 3, 8, Black);  // Shaft
      fillTriangle(arrowX, arrowY + 10, arrowX - 4, arrowY + 5, arrowX + 4, arrowY + 5, Black);  // Head
    } else {
      // Rightward arrow (stable/no change)
      fillRect(arrowX - 5, arrowY + 2, 10, 3, Black);  // Shaft
      fillTriangle(arrowX + 8, arrowY + 3, arrowX + 2, arrowY - 1, arrowX + 2, arrowY + 7, Black);  // Head
    }
  }

  // Draw alternating background bands
  for (int spacing = 0; spacing < y_minor_axis_hist; spacing++) {
    if (spacing % 2 == 1) {
      int bandY = y_pos + (gheight * spacing / y_minor_axis_hist);
      int bandH = gheight / y_minor_axis_hist;
      fillRect(x_pos + 1, bandY, gwidth, bandH, 0xDD);
    }
  }

  // Draw dashed horizontal grid lines
  const int number_of_dashes = 20;
  for (int spacing = 0; spacing < y_minor_axis_hist; spacing++) {
    for (int j = 0; j < number_of_dashes; j++) {
      drawFastHLine((x_pos + 3 + j * gwidth / number_of_dashes), y_pos + (gheight * spacing / y_minor_axis_hist), gwidth / (2 * number_of_dashes), Grey);
    }
  }

  // Draw vertical lines based on view type
  if (hours_span > 0) {
    float pixelsPerHour = (float)gwidth / hours_span;

    if (isWeeklyView) {
      // Weekly: only midnight lines - iterate through all hours in span
      for (int hoursFromStart = 1; hoursFromStart <= hours_span; hoursFromStart++) {
        int hourOfDay = (start_hour + hoursFromStart) % 24;
        if (hourOfDay == 0) {  // Midnight
          int lineX = x_pos + (int)(hoursFromStart * pixelsPerHour);
          if (lineX >= x_pos + 5 && lineX <= x_pos + gwidth - 5) {
            drawFastVLine(lineX - 1, y_pos, gheight, DarkGrey);
            drawFastVLine(lineX, y_pos, gheight, DarkGrey);
            drawFastVLine(lineX + 1, y_pos, gheight, DarkGrey);
          }
        }
      }
    } else {
      // 48H: lines every 6 hours
      for (int hoursFromStart = 1; hoursFromStart <= hours_span; hoursFromStart++) {
        int hourOfDay = (start_hour + hoursFromStart) % 24;
        if (hourOfDay % 6 == 0) {  // 0, 6, 12, 18
          int lineX = x_pos + (int)(hoursFromStart * pixelsPerHour);
          if (lineX >= x_pos && lineX <= x_pos + gwidth) {
            if (hourOfDay == 0) {
              // Midnight - thick line
              drawFastVLine(lineX - 1, y_pos, gheight, DarkGrey);
              drawFastVLine(lineX, y_pos, gheight, DarkGrey);
              drawFastVLine(lineX + 1, y_pos, gheight, DarkGrey);
            } else {
              // 6, 12, 18 hours
              drawFastVLine(lineX, y_pos, gheight, Grey);
            }
          }
        }
      }
    }
  }

  // Draw data lines ON TOP
  for (int gx = 0; gx < readings; gx++) {
    x2 = x_pos + gx * gwidth / (readings - 1) - 1;
    y2 = y_pos + (Y1Max - constrain(DataArray[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1;
    if (barchart_mode) {
      fillRect(x2, y2, 2, y_pos + gheight - y2 + 2, Black);
    } else {
      drawLine(last_x, last_y - 1, x2, y2 - 1, Black);
      drawLine(last_x, last_y, x2, y2, Black);
    }
    last_x = x2;
    last_y = y2;
  }

  // Draw Y axis values
  for (int spacing = 0; spacing <= y_minor_axis_hist; spacing++) {
    float axisValue = Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis_hist * spacing + 0.01;
    if (axisValue < 0.1) {
      drawString(x_pos - 10, y_pos + gheight * spacing / y_minor_axis_hist - 5, "0", RIGHT);
    } else if (axisValue < 5) {
      drawString(x_pos - 10, y_pos + gheight * spacing / y_minor_axis_hist - 5, String(axisValue, 1), RIGHT);
    } else {
      drawString(x_pos - 7, y_pos + gheight * spacing / y_minor_axis_hist - 5, String(axisValue, 0), RIGHT);
    }
  }

  // Hour labels and min/max only for 48H view
  if (!isWeeklyView && hours_span > 0) {
    setFont(OpenSans8B);
    float pixelsPerHour = (float)gwidth / hours_span;

    // Draw only 0, 6, 12, 18 hour labels where they fall in the data range
    for (int hoursFromStart = 1; hoursFromStart <= hours_span; hoursFromStart++) {
      int hourOfDay = (start_hour + hoursFromStart) % 24;
      if (hourOfDay == 0 || hourOfDay == 6 || hourOfDay == 12 || hourOfDay == 18) {
        int labelX = x_pos + (int)(hoursFromStart * pixelsPerHour);
        // Skip if too close to edges
        if (labelX >= x_pos + 15 && labelX <= x_pos + gwidth - 15) {
          drawString(labelX, y_pos + gheight + 2, String(hourOfDay), CENTER);
        }
      }
    }

    // Min/Max labels below graph
    String minStr = "min:" + (dataMin < 10 ? String(dataMin, 1) : String((int)dataMin));
    String maxStr = "max:" + (dataMax < 10 ? String(dataMax, 1) : String((int)dataMax));
    drawString(x_pos + 3, y_pos + gheight + 14, minStr, LEFT);
    drawString(x_pos + gwidth - 3, y_pos + gheight + 14, maxStr, RIGHT);
  }
}

void drawString(int x, int y, String text, alignment align) {
  char * data  = const_cast<char*>(text.c_str());
  int  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  int w, h;
  int xx = x, yy = y;
  get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);
  if (align == RIGHT)  x = x - w;
  if (align == CENTER) x = x - w / 2;
  int cursor_y = y + h;
  write_string(&currentFont, data, &x, &cursor_y, framebuffer);
}

// Common screen header with date and time
void drawScreenHeader() {
  setFont(OpenSans10B);
  drawString(300, 15, Date_str + "  @ " + Time_str, LEFT);
}

void fillCircle(int x, int y, int r, uint8_t color) {
  epd_fill_circle(x, y, r, color, framebuffer);
}

void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_hline(x0, y0, length, color, framebuffer);
}

void drawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_vline(x0, y0, length, color, framebuffer);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  epd_write_line(x0, y0, x1, y1, color, framebuffer);
}

void drawCircle(int x0, int y0, int r, uint8_t color) {
  epd_draw_circle(x0, y0, r, color, framebuffer);
}

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_draw_rect(x, y, w, h, color, framebuffer);
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_fill_rect(x, y, w, h, color, framebuffer);
}

void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2, uint16_t color) {
  epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, framebuffer);
}

void drawPixel(int x, int y, uint8_t color) {
  epd_draw_pixel(x, y, color, framebuffer);
}

void setFont(GFXfont const &font) {
  currentFont = font;
}

void edp_update() {
  epd_draw_grayscale_image(epd_full_screen(), framebuffer); // Update the screen
}

// ==================== TOUCH SCREEN FUNCTIONS ====================

// Screen 2: Current conditions detail
void DisplayCurrentDetailScreen() {

  // Date/Time same format and position as main screen
  drawScreenHeader();

  // Title
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 50, TXT_CURRENT_CONDITIONS, CENTER);
  drawFastHLine(150, 85, SCREEN_WIDTH - 300, Grey);

  String tempUnit = (Units == "M") ? "°C" : "°F";

  // === TOP SECTION: Temperature, Pressure, Humidity ===
  int col1X = 230, col2X = 510, col3X = 760;
  int topY = 110;

  // Temperature - value large, unit small aligned at bottom
  setFont(OpenSans24B);
  drawString(col1X, topY, String(WxConditions[0].Temperature, 1), RIGHT);
  setFont(OpenSans14B);
  drawString(col1X, topY + 10, tempUnit, LEFT);
  setFont(OpenSans10B);
  drawString(col1X - 25, topY + 45, TXT_MAX + " " + String(WxConditions[0].High, 0) + " / " + TXT_MIN + " " + String(WxConditions[0].Low, 0), CENTER);

  // Pressure - value large, unit small aligned at bottom
  setFont(OpenSans24B);
  float pressValue = (Units == "M") ? WxConditions[0].Pressure : WxConditions[0].Pressure * HPA_TO_INHG;
  String pressUnit = (Units == "M") ? "hPa" : "inHg";
  drawString(col2X, topY, String(pressValue, 0), RIGHT);
  setFont(OpenSans14B);
  drawString(col2X, topY + 10, pressUnit, LEFT);
  setFont(OpenSans10B);
  drawString(col2X - 20, topY + 45, TXT_PRESSURE, CENTER);

  // Humidity - value large, unit small aligned at bottom
  setFont(OpenSans24B);
  drawString(col3X, topY, String(WxConditions[0].Humidity, 0), RIGHT);
  setFont(OpenSans14B);
  drawString(col3X, topY + 10, "%", LEFT);
  setFont(OpenSans10B);
  drawString(col3X - 15, topY + 45, TXT_HUMIDITY, CENTER);

  // === MIDDLE SECTION: Weather Icon + Details ===
  drawFastHLine(50, 190, SCREEN_WIDTH - 100, Grey);

  // Left: Weather icon (extra large) - moved 40px right
  iconScaleOverride = XLarge;
  DisplayConditionsSection(220, 300, WxConditions[0].Icon, LargeIcon);
  iconScaleOverride = 0;

  // Weather description below icon
  setFont(OpenSans10B);
  drawString(220, 385, TitleCase(WxConditions[0].Forecast0), CENTER);

  // Right: Visibility, Wind, Clouds, Feels Like
  int rightColLabel = 675, rightColValue = 805;
  int row1Y = 225, row2Y = 270, row3Y = 315, row4Y = 360;

  setFont(OpenSans12B);
  drawString(rightColLabel, row1Y, TXT_VISIBILITY + ":", RIGHT);
  drawString(rightColLabel, row2Y, TXT_WIND + ":", RIGHT);
  drawString(rightColLabel, row3Y, TXT_CLOUDINESS + ":", RIGHT);
  drawString(rightColLabel, row4Y, TXT_FEELS_LIKE + ":", RIGHT);

  setFont(OpenSans18B);
  float visKm = WxConditions[0].Visibility / 1000.0;
  drawString(rightColValue, row1Y - 5, String(visKm, 1) + " km", CENTER);
  drawString(rightColValue, row2Y - 5, String(WxConditions[0].Windspeed, 1) + ((Units == "M") ? " m/s" : " mph"), CENTER);
  drawString(rightColValue, row3Y - 5, String(WxConditions[0].Cloudcover) + "%", CENTER);
  drawString(rightColValue, row4Y - 5, String(WxConditions[0].Feelslike, 1) + tempUnit, CENTER);

  // === BOTTOM SECTION: Sunrise, Sunset, Rain Probability ===
  drawFastHLine(50, 420, SCREEN_WIDTH - 100, Grey);

  int bottomY = 445;  // Moved up 25px
  int bot1X = 160, bot2X = 480, bot3X = 800;

  setFont(OpenSans18B);
  drawString(bot1X, bottomY, ConvertUnixTime(WxConditions[0].Sunrise).substring(0, 5), CENTER);
  drawString(bot2X, bottomY, ConvertUnixTime(WxConditions[0].Sunset).substring(0, 5), CENTER);
  drawString(bot3X, bottomY, String(WxForecast[0].Pop * 100, 0) + "%", CENTER);

  setFont(OpenSans10B);
  drawString(bot1X, bottomY + 35, TXT_SUNRISE, CENTER);
  drawString(bot2X, bottomY + 35, TXT_SUNSET, CENTER);
  drawString(bot3X, bottomY + 35, TXT_RAIN_PROB, CENTER);
}

// Screen 3: Extended forecast
void DisplayForecastScreen() {
  // Date/Time same format and position as main screen
  drawScreenHeader();

  // Title
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 50, TXT_5DAY_FORECAST, CENTER);
  drawFastHLine(150, 85, SCREEN_WIDTH - 300, Grey);

  // === SECTION 1: Hourly forecast - 8 columns ===
  setFont(OpenSans12B);
  drawString(SCREEN_WIDTH / 2, 100, TXT_NEXT_24H, CENTER);

  int hourlyY = 145;
  int hourlyWidth = SCREEN_WIDTH / 8;  // 120 pixels each
  int startIdx = getFirstRelevantForecastIndex();
  for (int i = 0; i < min(8, max_readings - startIdx); i++) {
    int dataIdx = startIdx + i;
    int xPos = hourlyWidth / 2 + i * hourlyWidth;

    setFont(OpenSans10B);
    String timeStr = ConvertUnixTime(WxForecast[dataIdx].Dt + WxConditions[0].Timezone).substring(0, 5);
    drawString(xPos, hourlyY, timeStr, CENTER);

    DisplayConditionsSection(xPos, hourlyY + 65, WxForecast[dataIdx].Icon, SmallIcon);

    setFont(OpenSans10B);
    drawString(xPos, hourlyY + 105, String(WxForecast[dataIdx].High, 0) + "°/" + String(WxForecast[dataIdx].Low, 0) + "°", CENTER);
  }

  // Divider
  drawFastHLine(50, 275, SCREEN_WIDTH - 100, Grey);

  // === SECTION 2: 5-day forecast - 5 columns ===
  setFont(OpenSans12B);
  drawString(SCREEN_WIDTH / 2, 300, TXT_5DAY_OUTLOOK, CENTER);

  int dailyY = 335;
  int dailyWidth = SCREEN_WIDTH / 5;  // 192 pixels each

  for (int day = 0; day < 5; day++) {
    int baseIdx = day * 8;
    if (baseIdx >= max_readings) break;

    int xPos = dailyWidth / 2 + day * dailyWidth;

    // Day name
    time_t tm = WxForecast[baseIdx].Dt;
    struct tm *dt = localtime(&tm);
    setFont(OpenSans12B);
    drawString(xPos, dailyY, weekday_D[dt->tm_wday], CENTER);

    // Calculate high/low/rain for the day
    float dayHigh = -100, dayLow = 100, dayRain = 0;
    String dayIcon = "";
    for (int h = 0; h < 8 && (baseIdx + h) < max_readings; h++) {
      int idx = baseIdx + h;
      if (WxForecast[idx].High > dayHigh) dayHigh = WxForecast[idx].High;
      if (WxForecast[idx].Low < dayLow) dayLow = WxForecast[idx].Low;
      dayRain += WxForecast[idx].Rainfall;
      if (h == 4 || h == 5) dayIcon = WxForecast[idx].Icon;
    }
    if (dayIcon == "") dayIcon = WxForecast[baseIdx].Icon;

    // Weather icon
    DisplayConditionsSection(xPos, dailyY + 70, dayIcon, SmallIcon);

    // Temperature
    setFont(OpenSans12B);
    drawString(xPos, dailyY + 120, String(dayHigh, 0) + "°/" + String(dayLow, 0) + "°", CENTER);

    // Rain if any
    if (dayRain > 0.1) {
      setFont(OpenSans10B);
      drawString(xPos, dailyY + 150, String(dayRain, 1) + "mm", CENTER);
    }
  }
}

// Screen 4: Expanded graphs
void DisplayGraphsScreen() {
  // Date/Time same format and position as main screen
  drawScreenHeader();

  // Title
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 50, TXT_WEATHER_TRENDS, CENTER);
  drawFastHLine(150, 85, SCREEN_WIDTH - 300, Grey);

  // Prepare data
  for (int r = 0; r < max_readings; r++) {
    if (Units == "I") {
      pressure_readings[r] = WxForecast[r].Pressure * HPA_TO_INHG;
      rain_readings[r] = WxForecast[r].Rainfall * MM_TO_INCHES;
      snow_readings[r] = WxForecast[r].Snowfall * MM_TO_INCHES;
    } else {
      pressure_readings[r] = WxForecast[r].Pressure;
      rain_readings[r] = WxForecast[r].Rainfall;
      snow_readings[r] = WxForecast[r].Snowfall;
    }
    temperature_readings[r] = WxForecast[r].Temperature;
    humidity_readings[r] = WxForecast[r].Humidity;
  }

  // 2x2 grid of graphs (same layout as History screen)
  int gwidth = 390;
  int gheight = 140;
  int gx1 = 65, gx2 = 530;
  int gy1 = 120, gy2 = 320;

  // Temperature - top left
  if (Units == "M")
    DrawGraph(gx1, gy1, gwidth, gheight, 10, 40, TXT_TEMPERATURE_C, temperature_readings, max_readings, autoscale_on, barchart_off);
  else
    DrawGraph(gx1, gy1, gwidth, gheight, 50, 95, TXT_TEMPERATURE_F, temperature_readings, max_readings, autoscale_on, barchart_off);

  // Pressure - top right
  if (Units == "M")
    DrawGraph(gx2, gy1, gwidth, gheight, 900, 1050, TXT_PRESSURE_HPA, pressure_readings, max_readings, autoscale_on, barchart_off);
  else
    DrawGraph(gx2, gy1, gwidth, gheight, 26, 31, TXT_PRESSURE_IN, pressure_readings, max_readings, autoscale_on, barchart_off);

  // Humidity - bottom left
  DrawGraph(gx1, gy2, gwidth, gheight, 0, 100, TXT_HUMIDITY_PERCENT, humidity_readings, max_readings, autoscale_off, barchart_off);

  // Rain/Snow - bottom right (thin bars)
  if (SumOfPrecip(rain_readings, max_readings) >= SumOfPrecip(snow_readings, max_readings)) {
    if (Units == "M")
      DrawGraph(gx2, gy2, gwidth, gheight, 0, 30, TXT_RAINFALL_MM, rain_readings, max_readings, autoscale_on, barchart_on);
    else
      DrawGraph(gx2, gy2, gwidth, gheight, 0, 1.2, TXT_RAINFALL_IN, rain_readings, max_readings, autoscale_on, barchart_on);
  } else {
    if (Units == "M")
      DrawGraph(gx2, gy2, gwidth, gheight, 0, 30, TXT_SNOWFALL_MM, snow_readings, max_readings, autoscale_on, barchart_on);
    else
      DrawGraph(gx2, gy2, gwidth, gheight, 0, 1.2, TXT_SNOWFALL_IN, snow_readings, max_readings, autoscale_on, barchart_on);
  }

  // Footer
  setFont(OpenSans12B);
  drawString(SCREEN_WIDTH / 2, 510, TXT_NEXT_DAYS, CENTER);
}

// Screen 5: System Information
void DisplayInfoScreen() {
  // Date/Time same format and position as main screen
  drawScreenHeader();

  // Title
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 50, TXT_SYSTEM_INFO, CENTER);
  drawFastHLine(150, 85, SCREEN_WIDTH - 300, Grey);

  int col1X = 50;    // Labels left
  int col2X = 230;   // Values left
  int col3X = 500;   // Labels right
  int col4X = 680;   // Values right

  setFont(OpenSans10B);

  // === LEFT COLUMN - Explicit Y coordinates ===

  // Section 1: System Info
  drawString(col1X, 110, TXT_VERSION, LEFT);
  drawString(col2X, 110, version, LEFT);

  drawString(col1X, 138, TXT_COMPILED, LEFT);
  drawString(col2X, 138, String(__DATE__) + " " + String(__TIME__), LEFT);

  drawString(col1X, 166, TXT_FREE_HEAP, LEFT);
  drawString(col2X, 166, String(ESP.getFreeHeap() / 1024) + " KB", LEFT);

  drawString(col1X, 194, TXT_TOTAL_HEAP, LEFT);
  drawString(col2X, 194, String(ESP.getHeapSize() / 1024) + " KB", LEFT);

  drawString(col1X, 222, "PSRAM:", LEFT);
  drawString(col2X, 222, String(ESP.getFreePsram() / 1024) + " KB", LEFT);

  // Section 2: WiFi Info (with gap)
  drawString(col1X, 265, "WiFi SSID:", LEFT);
  drawString(col2X, 265, WiFi.isConnected() ? WiFi.SSID() : TXT_DISCONNECTED, LEFT);

  drawString(col1X, 293, "IP:", LEFT);
  drawString(col2X, 293, WiFi.isConnected() ? WiFi.localIP().toString() : "--", LEFT);

  drawString(col1X, 321, TXT_WIFI_SIGNAL, LEFT);
  drawString(col2X, 321, WiFi.isConnected() ? String(WiFi.RSSI()) + " dBm" : "--", LEFT);

  // Section 3: Location (with gap)
  drawString(col1X, 364, TXT_CITY, LEFT);
  drawString(col2X, 364, String(config.city), LEFT);

  drawString(col1X, 392, TXT_COORDINATES, LEFT);
  drawString(col2X, 392, String(config.latitude) + ", " + String(config.longitude), LEFT);

  drawString(col1X, 420, TXT_TIMEZONE, LEFT);
  drawString(col2X, 420, String(config.timezone), LEFT);

  // === RIGHT COLUMN - Explicit Y coordinates ===

  // Section 1: Config
  drawString(col3X, 110, TXT_LANGUAGE, LEFT);
  String langStr = strcmp(config.language, "ES") == 0 ? "Espanol" :
                   strcmp(config.language, "EN") == 0 ? "English" : "Francais";
  drawString(col4X, 110, langStr, LEFT);

  drawString(col3X, 138, TXT_UNITS, LEFT);
  drawString(col4X, 138, strcmp(config.units, "M") == 0 ? TXT_METRIC : TXT_IMPERIAL, LEFT);

  drawString(col3X, 166, TXT_HEMISPHERE, LEFT);
  drawString(col4X, 166, strcmp(config.hemisphere, "north") == 0 ? TXT_NORTH : TXT_SOUTH, LEFT);

  drawString(col3X, 194, TXT_FORECAST, LEFT);
  drawString(col4X, 194, String(config.forecast_days) + " " + TXT_DAYS, LEFT);

  // Section 2: System Config (with gap)
  drawString(col3X, 237, TXT_UPDATE_INTERVAL, LEFT);
  drawString(col4X, 237, String(config.update_interval) + " min", LEFT);

  drawString(col3X, 265, "Timeout:", LEFT);
  drawString(col4X, 265, String(config.sleep_timeout) + " " + TXT_SEC, LEFT);

  drawString(col3X, 293, TXT_ON_SLEEP, LEFT);
  drawString(col4X, 293, config.keep_screen_on_sleep ? TXT_KEEP_SCREEN : TXT_GO_MAIN, LEFT);

  // Section 3: API Info (with gap)
  drawString(col3X, 336, "API Key:", LEFT);
  String apiKeyMasked = String(config.api_key).substring(0, 8) + "...";
  drawString(col4X, 336, apiKeyMasked, LEFT);

  // Web Config URL
  drawString(col3X, 379, TXT_WEB_CONFIG, LEFT);
  if (WiFi.isConnected()) {
    drawString(col4X, 379, "http://" + WiFi.localIP().toString(), LEFT);
  } else {
    drawString(col4X, 379, TXT_WIFI_DISCONNECTED, LEFT);
  }

  // Clean display button at bottom center
  setFont(OpenSans12B);
  int btnX = SCREEN_WIDTH/2 - 120;
  int btnY = 451;
  // Draw thick border (5px) like toggle buttons
  for (int i = 0; i < 5; i++) {
    drawRect(btnX + i, btnY + i, 240 - i*2, 45 - i*2, Black);
  }
  drawString(SCREEN_WIDTH/2, btnY + 8, TXT_CLEAN_SCREEN, CENTER);

  // Footer - Uptime
  setFont(OpenSans8B);
  unsigned long uptime = millis() / 1000;
  int uptimeMin = (uptime / 60) % 60;
  int uptimeSec = uptime % 60;
  drawString(SCREEN_WIDTH / 2, 515, TXT_UPTIME + " " + String(uptimeMin) + "m " + String(uptimeSec) + "s", CENTER);
}

// Screen 6: Weather History (real recorded data)
void DisplayHistoryScreen() {
  // Date/Time same format and position as main screen
  drawScreenHeader();

  // Title
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 50, TXT_HISTORY, CENTER);
  drawFastHLine(150, 85, SCREEN_WIDTH - 300, Grey);

  // Toggle button top right - text with thick border
  setFont(OpenSans12B);
  String toggleText = historyShowWeek ? TXT_48H : TXT_1WEEK;
  int btnX = SCREEN_WIDTH - 105;
  int btnY = 35;
  // Draw thick border (5px)
  for (int i = 0; i < 5; i++) {
    drawRect(btnX + i, btnY + i, 90 - i*2, 32 - i*2, Black);
  }
  drawString(btnX + 45, btnY + 6, toggleText, CENTER);

  if (historyCount < 2) {
    setFont(OpenSans12B);
    drawString(SCREEN_WIDTH / 2, 300, TXT_NOT_ENOUGH_DATA, CENTER);
    drawString(SCREEN_WIDTH / 2, 340, TXT_HISTORY_FILLS, CENTER);
    return;
  }

  // Prepare data arrays
  #define HISTORY_GRAPH_READINGS 100
  float hist_temp[HISTORY_GRAPH_READINGS];
  float hist_hum[HISTORY_GRAPH_READINGS];
  float hist_press[HISTORY_GRAPH_READINGS];
  float hist_rain[HISTORY_GRAPH_READINGS];

  // Get readings based on selected view (48h or 168h = 1 week)
  int requestedHours = historyShowWeek ? 168 : 48;
  int readings = getReadingsForHours(requestedHours, hist_temp, hist_hum, hist_press, hist_rain, HISTORY_GRAPH_READINGS);

  // Get actual hours span from timestamps (not assuming 10-min intervals)
  int actualHours = (int)getHistoryHoursSpan(requestedHours);
  if (actualHours < 1) actualHours = 1;

  // Get start hour from oldest data timestamp
  int startHour = getHistoryStartHour(requestedHours);

  if (readings < 2) {
    setFont(OpenSans12B);
    drawString(SCREEN_WIDTH / 2, 300, TXT_INSUFFICIENT_DATA, CENTER);
    return;
  }

  // Draw 4 graphs (2x2 layout)
  int gwidth = 390;
  int gheight = 140;
  int gx1 = 65, gx2 = 530;
  int gy1 = 120, gy2 = 320;

  // Temperature - top left
  DrawHistoryGraph(gx1, gy1, gwidth, gheight, 0, 40, TXT_GRAPH_TEMP, Units == "M" ? "°C" : "°F", hist_temp, readings, autoscale_on, barchart_off, actualHours, historyShowWeek, startHour);

  // Pressure - top right
  DrawHistoryGraph(gx2, gy1, gwidth, gheight, 900, 1050, TXT_GRAPH_PRESSURE, Units == "M" ? "hPa" : "in", hist_press, readings, autoscale_on, barchart_off, actualHours, historyShowWeek, startHour);

  // Humidity - bottom left
  DrawHistoryGraph(gx1, gy2, gwidth, gheight, 0, 100, TXT_GRAPH_HUMIDITY, "%", hist_hum, readings, autoscale_off, barchart_off, actualHours, historyShowWeek, startHour);

  // Rainfall - bottom right
  DrawHistoryGraph(gx2, gy2, gwidth, gheight, 0, 10, TXT_GRAPH_RAIN, Units == "M" ? "mm" : "in", hist_rain, readings, autoscale_on, barchart_on, actualHours, historyShowWeek, startHour);

  // Footer - readings count
  setFont(OpenSans10B);
  String histInfo = String(historyCount) + " " + TXT_READINGS + " / " + String(getHistoryDays(), 1) + " " + TXT_DAYS;
  drawString(SCREEN_WIDTH / 2, 505, histInfo, CENTER);
}

