// BLE Configuration for Weather Station
// Allows WiFi and settings configuration via Bluetooth Low Energy
// Activated by pressing BOOT button during startup or from deep sleep

#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <driver/gpio.h>

// External declarations from main sketch
extern uint8_t *framebuffer;
extern void setFont(GFXfont const &font);
extern void drawString(int x, int y, String text, alignment align);
extern void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
extern void drawCircle(int x0, int y0, int r, uint8_t color);

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID              "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

// WiFi Characteristics (3 networks)
#define CHAR_WIFI1_UUID           "beb5483e-36e1-4688-b7f5-ea07361b2601"
#define CHAR_WIFI2_UUID           "beb5483e-36e1-4688-b7f5-ea07361b2602"
#define CHAR_WIFI3_UUID           "beb5483e-36e1-4688-b7f5-ea07361b2603"

// API & Location
#define CHAR_API_KEY_UUID         "beb5483e-36e1-4688-b7f5-ea07361b2610"
#define CHAR_CITY_UUID            "beb5483e-36e1-4688-b7f5-ea07361b2611"
#define CHAR_LOCATION_UUID        "beb5483e-36e1-4688-b7f5-ea07361b2612"

// Regional Settings
#define CHAR_LANGUAGE_UUID        "beb5483e-36e1-4688-b7f5-ea07361b2620"
#define CHAR_UNITS_UUID           "beb5483e-36e1-4688-b7f5-ea07361b2621"
#define CHAR_TIMEZONE_UUID        "beb5483e-36e1-4688-b7f5-ea07361b2622"

// Behavior Settings
#define CHAR_INTERVALS_UUID       "beb5483e-36e1-4688-b7f5-ea07361b2630"
#define CHAR_SLEEP_UUID           "beb5483e-36e1-4688-b7f5-ea07361b2631"

// Control
#define CHAR_STATUS_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26f0"
#define CHAR_COMMAND_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26f1"
#define CHAR_CONFIG_READ_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26f2"

// BLE Device name
#define BLE_DEVICE_NAME "WeatherStation-BLE"

// Status codes
#define BLE_STATUS_READY       "READY"
#define BLE_STATUS_SAVED       "SAVED"
#define BLE_STATUS_ERROR       "ERROR"
#define BLE_STATUS_CONNECTING  "CONNECTING"
#define BLE_STATUS_CONNECTED   "WIFI_OK"
#define BLE_STATUS_WIFI_FAIL   "WIFI_FAIL"

// Commands
#define BLE_CMD_SAVE           "SAVE"
#define BLE_CMD_RESTART        "RESTART"
#define BLE_CMD_WIFI_TEST      "TEST_WIFI"
#define BLE_CMD_READ_CONFIG    "READ_CONFIG"

// Global BLE objects
BLEServer* pServer = NULL;
BLECharacteristic* pCharStatus = NULL;
BLECharacteristic* pCharConfigRead = NULL;
bool bleDeviceConnected = false;
bool bleOldDeviceConnected = false;
bool bleConfigMode = false;
bool bleConfigSaved = false;

// Temporary storage for BLE-received values
struct BLEConfig {
  // WiFi (3 networks)
  String wifi_ssid1, wifi_pass1;
  String wifi_ssid2, wifi_pass2;
  String wifi_ssid3, wifi_pass3;

  // API & Location
  String api_key;
  String city;
  String latitude, longitude;

  // Regional
  String language;      // "es", "en", "fr"
  String hemisphere;    // "north", "south"
  String units;         // "M" or "I"
  String timezone;
  int gmt_offset;
  int dst_offset;

  // Behavior
  int forecast_days;
  int update_interval;
  int sleep_timeout;
  bool keep_screen;
  int wakeup_hour;
  int sleep_hour;

  // Flags
  bool wifi1_set, wifi2_set, wifi3_set;
  bool api_set, city_set, location_set;
  bool regional_set, intervals_set, sleep_set;
} bleConfig;

// Forward declarations
void saveBLEConfig();
void testWiFiConnection();
void sendCurrentConfig();
void DisplayBLEConfigScreen();

// Server callbacks
class WeatherServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    bleDeviceConnected = true;
    Serial.println("BLE: Device connected");
    if (pCharStatus) {
      pCharStatus->setValue(BLE_STATUS_READY);
      pCharStatus->notify();
    }
  }

  void onDisconnect(BLEServer* pServer) {
    bleDeviceConnected = false;
    Serial.println("BLE: Device disconnected");
    delay(500);
    pServer->startAdvertising();
  }
};

// WiFi 1 callback - format: "ssid|password"
class WiFi1Callbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String value = String(pChar->getValue().c_str());
    int sep = value.indexOf('|');
    if (sep > 0) {
      bleConfig.wifi_ssid1 = value.substring(0, sep);
      bleConfig.wifi_pass1 = value.substring(sep + 1);
      bleConfig.wifi1_set = true;
      Serial.println("BLE: WiFi1 received: " + bleConfig.wifi_ssid1);
    }
  }
};

// WiFi 2 callback
class WiFi2Callbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String value = String(pChar->getValue().c_str());
    int sep = value.indexOf('|');
    if (sep > 0) {
      bleConfig.wifi_ssid2 = value.substring(0, sep);
      bleConfig.wifi_pass2 = value.substring(sep + 1);
      bleConfig.wifi2_set = true;
      Serial.println("BLE: WiFi2 received: " + bleConfig.wifi_ssid2);
    }
  }
};

// WiFi 3 callback
class WiFi3Callbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String value = String(pChar->getValue().c_str());
    int sep = value.indexOf('|');
    if (sep > 0) {
      bleConfig.wifi_ssid3 = value.substring(0, sep);
      bleConfig.wifi_pass3 = value.substring(sep + 1);
      bleConfig.wifi3_set = true;
      Serial.println("BLE: WiFi3 received: " + bleConfig.wifi_ssid3);
    }
  }
};

// API Key callback
class ApiKeyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    bleConfig.api_key = String(pChar->getValue().c_str());
    bleConfig.api_set = true;
    Serial.println("BLE: API Key received");
  }
};

// City callback
class CityCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    bleConfig.city = String(pChar->getValue().c_str());
    bleConfig.city_set = true;
    Serial.println("BLE: City received: " + bleConfig.city);
  }
};

// Location callback - format: "lat|lon"
class LocationCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String value = String(pChar->getValue().c_str());
    int sep = value.indexOf('|');
    if (sep > 0) {
      bleConfig.latitude = value.substring(0, sep);
      bleConfig.longitude = value.substring(sep + 1);
      bleConfig.location_set = true;
      Serial.println("BLE: Location received: " + bleConfig.latitude + ", " + bleConfig.longitude);
    }
  }
};

// Language callback - format: "lang|hemi" (e.g., "es|north")
class LanguageCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String value = String(pChar->getValue().c_str());
    int sep = value.indexOf('|');
    if (sep > 0) {
      bleConfig.language = value.substring(0, sep);
      bleConfig.hemisphere = value.substring(sep + 1);
      Serial.println("BLE: Language received: " + bleConfig.language + ", " + bleConfig.hemisphere);
    } else {
      bleConfig.language = value;
    }
  }
};

// Units callback
class UnitsCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    bleConfig.units = String(pChar->getValue().c_str());
    Serial.println("BLE: Units received: " + bleConfig.units);
  }
};

// Timezone callback - format: "tz|gmt|dst" (e.g., "CST6CDT|-21600|3600")
class TimezoneCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String value = String(pChar->getValue().c_str());
    int sep1 = value.indexOf('|');
    int sep2 = value.indexOf('|', sep1 + 1);
    if (sep1 > 0 && sep2 > 0) {
      bleConfig.timezone = value.substring(0, sep1);
      bleConfig.gmt_offset = value.substring(sep1 + 1, sep2).toInt();
      bleConfig.dst_offset = value.substring(sep2 + 1).toInt();
      bleConfig.regional_set = true;
      Serial.println("BLE: Timezone received: " + bleConfig.timezone);
    }
  }
};

// Intervals callback - format: "update|forecast_days" (e.g., "30|3")
class IntervalsCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String value = String(pChar->getValue().c_str());
    int sep = value.indexOf('|');
    if (sep > 0) {
      bleConfig.update_interval = value.substring(0, sep).toInt();
      bleConfig.forecast_days = value.substring(sep + 1).toInt();
      bleConfig.intervals_set = true;
      Serial.println("BLE: Intervals received: " + String(bleConfig.update_interval) + "min, " + String(bleConfig.forecast_days) + " days");
    }
  }
};

// Sleep callback - format: "timeout|keepscreen|wakeup|sleep" (e.g., "30|0|7|23")
class SleepCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String value = String(pChar->getValue().c_str());
    int sep1 = value.indexOf('|');
    int sep2 = value.indexOf('|', sep1 + 1);
    int sep3 = value.indexOf('|', sep2 + 1);
    if (sep1 > 0 && sep2 > 0 && sep3 > 0) {
      bleConfig.sleep_timeout = value.substring(0, sep1).toInt();
      bleConfig.keep_screen = value.substring(sep1 + 1, sep2).toInt() == 1;
      bleConfig.wakeup_hour = value.substring(sep2 + 1, sep3).toInt();
      bleConfig.sleep_hour = value.substring(sep3 + 1).toInt();
      bleConfig.sleep_set = true;
      Serial.println("BLE: Sleep config received");
    }
  }
};

// Command callback
class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String cmd = String(pChar->getValue().c_str());
    Serial.println("BLE: Command received: " + cmd);

    if (cmd == BLE_CMD_SAVE) {
      saveBLEConfig();
    } else if (cmd == BLE_CMD_RESTART) {
      if (pCharStatus) {
        pCharStatus->setValue("RESTARTING");
        pCharStatus->notify();
      }
      delay(1000);
      ESP.restart();
    } else if (cmd == BLE_CMD_WIFI_TEST) {
      testWiFiConnection();
    } else if (cmd == BLE_CMD_READ_CONFIG) {
      sendCurrentConfig();
    }
  }
};

// Initialize BLE
void initBLE() {
  Serial.println("BLE: Initializing...");

  // Reset config flags
  memset(&bleConfig, 0, sizeof(bleConfig));

  BLEDevice::init(BLE_DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new WeatherServerCallbacks());

  BLEService* pService = pServer->createService(BLEUUID(SERVICE_UUID), 30);

  // WiFi characteristics
  BLECharacteristic* pCharWifi1 = pService->createCharacteristic(CHAR_WIFI1_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharWifi1->setCallbacks(new WiFi1Callbacks());

  BLECharacteristic* pCharWifi2 = pService->createCharacteristic(CHAR_WIFI2_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharWifi2->setCallbacks(new WiFi2Callbacks());

  BLECharacteristic* pCharWifi3 = pService->createCharacteristic(CHAR_WIFI3_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharWifi3->setCallbacks(new WiFi3Callbacks());

  // API & Location
  BLECharacteristic* pCharApi = pService->createCharacteristic(CHAR_API_KEY_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharApi->setCallbacks(new ApiKeyCallbacks());

  BLECharacteristic* pCharCity = pService->createCharacteristic(CHAR_CITY_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharCity->setCallbacks(new CityCallbacks());

  BLECharacteristic* pCharLoc = pService->createCharacteristic(CHAR_LOCATION_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharLoc->setCallbacks(new LocationCallbacks());

  // Regional
  BLECharacteristic* pCharLang = pService->createCharacteristic(CHAR_LANGUAGE_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharLang->setCallbacks(new LanguageCallbacks());

  BLECharacteristic* pCharUnits = pService->createCharacteristic(CHAR_UNITS_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharUnits->setCallbacks(new UnitsCallbacks());

  BLECharacteristic* pCharTz = pService->createCharacteristic(CHAR_TIMEZONE_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharTz->setCallbacks(new TimezoneCallbacks());

  // Behavior
  BLECharacteristic* pCharIntervals = pService->createCharacteristic(CHAR_INTERVALS_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharIntervals->setCallbacks(new IntervalsCallbacks());

  BLECharacteristic* pCharSleep = pService->createCharacteristic(CHAR_SLEEP_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharSleep->setCallbacks(new SleepCallbacks());

  // Status (read + notify)
  pCharStatus = pService->createCharacteristic(CHAR_STATUS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharStatus->addDescriptor(new BLE2902());
  pCharStatus->setValue(BLE_STATUS_READY);

  // Config read (for sending current config to app)
  pCharConfigRead = pService->createCharacteristic(CHAR_CONFIG_READ_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharConfigRead->addDescriptor(new BLE2902());

  // Command
  BLECharacteristic* pCharCmd = pService->createCharacteristic(CHAR_COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharCmd->setCallbacks(new CommandCallbacks());

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  bleConfigMode = true;
  Serial.println("BLE: Advertising started - " BLE_DEVICE_NAME);
}

// Stop BLE
void stopBLE() {
  if (bleConfigMode) {
    BLEDevice::deinit(true);
    bleConfigMode = false;
    Serial.println("BLE: Stopped");
  }
}

// Save configuration received via BLE
void saveBLEConfig() {
  Serial.println("BLE: Saving configuration...");

  Preferences prefs;
  prefs.begin("weather", false);

  bool saved = false;

  // WiFi networks
  if (bleConfig.wifi1_set) {
    prefs.putString("ssid1", bleConfig.wifi_ssid1);
    prefs.putString("pass1", bleConfig.wifi_pass1);
    saved = true;
  }
  if (bleConfig.wifi2_set) {
    prefs.putString("ssid2", bleConfig.wifi_ssid2);
    prefs.putString("pass2", bleConfig.wifi_pass2);
    saved = true;
  }
  if (bleConfig.wifi3_set) {
    prefs.putString("ssid3", bleConfig.wifi_ssid3);
    prefs.putString("pass3", bleConfig.wifi_pass3);
    saved = true;
  }

  // API & Location
  if (bleConfig.api_set) {
    prefs.putString("apikey", bleConfig.api_key);
    saved = true;
  }
  if (bleConfig.city_set) {
    prefs.putString("city", bleConfig.city);
    saved = true;
  }
  if (bleConfig.location_set) {
    prefs.putString("lat", bleConfig.latitude);
    prefs.putString("lon", bleConfig.longitude);
    saved = true;
  }

  // Regional
  if (bleConfig.language.length() > 0) {
    prefs.putString("lang", bleConfig.language);
    saved = true;
  }
  if (bleConfig.hemisphere.length() > 0) {
    prefs.putString("hemi", bleConfig.hemisphere);
    saved = true;
  }
  if (bleConfig.units.length() > 0) {
    prefs.putString("units", bleConfig.units);
    saved = true;
  }
  if (bleConfig.regional_set) {
    prefs.putString("tz", bleConfig.timezone);
    prefs.putInt("gmt", bleConfig.gmt_offset);
    prefs.putInt("dst", bleConfig.dst_offset);
    saved = true;
  }

  // Behavior
  if (bleConfig.intervals_set) {
    prefs.putInt("updint", bleConfig.update_interval);
    prefs.putInt("fcdays", bleConfig.forecast_days);
    saved = true;
  }
  if (bleConfig.sleep_set) {
    prefs.putInt("sleept", bleConfig.sleep_timeout);
    prefs.putBool("keepscr", bleConfig.keep_screen);
    prefs.putInt("wakeuph", bleConfig.wakeup_hour);
    prefs.putInt("sleeph", bleConfig.sleep_hour);
    saved = true;
  }

  prefs.end();

  if (saved) {
    bleConfigSaved = true;
    if (pCharStatus) {
      pCharStatus->setValue(BLE_STATUS_SAVED);
      pCharStatus->notify();
    }
    Serial.println("BLE: Configuration saved");
  } else {
    if (pCharStatus) {
      pCharStatus->setValue(BLE_STATUS_ERROR);
      pCharStatus->notify();
    }
  }
}

// Send current configuration to app
void sendCurrentConfig() {
  Preferences prefs;
  prefs.begin("weather", true);

  // Create JSON with current config
  StaticJsonDocument<1024> doc;

  doc["wifi1"] = prefs.getString("ssid1", "");
  doc["wifi2"] = prefs.getString("ssid2", "");
  doc["wifi3"] = prefs.getString("ssid3", "");
  doc["city"] = prefs.getString("city", "");
  doc["lat"] = prefs.getString("lat", "");
  doc["lon"] = prefs.getString("lon", "");
  doc["lang"] = prefs.getString("lang", "es");
  doc["hemi"] = prefs.getString("hemi", "north");
  doc["units"] = prefs.getString("units", "M");
  doc["tz"] = prefs.getString("tz", "");
  doc["gmt"] = prefs.getInt("gmt", -21600);
  doc["dst"] = prefs.getInt("dst", 0);
  doc["updint"] = prefs.getInt("updint", 30);
  doc["fcdays"] = prefs.getInt("fcdays", 3);
  doc["sleept"] = prefs.getInt("sleept", 30);
  doc["keepscr"] = prefs.getBool("keepscr", false);
  doc["wakeuph"] = prefs.getInt("wakeuph", 7);
  doc["sleeph"] = prefs.getInt("sleeph", 23);

  prefs.end();

  String jsonStr;
  serializeJson(doc, jsonStr);

  if (pCharConfigRead) {
    pCharConfigRead->setValue(jsonStr.c_str());
    pCharConfigRead->notify();
  }

  Serial.println("BLE: Config sent: " + jsonStr);
}

// Test WiFi connection
void testWiFiConnection() {
  String ssid = bleConfig.wifi_ssid1;
  String pass = bleConfig.wifi_pass1;

  if (ssid.length() == 0) {
    // Try to use stored config
    Preferences prefs;
    prefs.begin("weather", true);
    ssid = prefs.getString("ssid1", "");
    pass = prefs.getString("pass1", "");
    prefs.end();
  }

  if (ssid.length() == 0) {
    if (pCharStatus) {
      pCharStatus->setValue("NO_SSID");
      pCharStatus->notify();
    }
    return;
  }

  if (pCharStatus) {
    pCharStatus->setValue(BLE_STATUS_CONNECTING);
    pCharStatus->notify();
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nBLE: WiFi test OK - " + WiFi.localIP().toString());
    if (pCharStatus) {
      String status = String(BLE_STATUS_CONNECTED) + "|" + WiFi.localIP().toString();
      pCharStatus->setValue(status.c_str());
      pCharStatus->notify();
    }
    WiFi.disconnect();
  } else {
    Serial.println("\nBLE: WiFi test failed");
    if (pCharStatus) {
      pCharStatus->setValue(BLE_STATUS_WIFI_FAIL);
      pCharStatus->notify();
    }
  }

  WiFi.mode(WIFI_OFF);
}

// Check BOOT button (GPIO 0, active LOW)
// Uses internal pullup, checks briefly, then restores
bool isBootButtonPressed() {
  // Save current state and enable pullup
  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask = (1ULL << 0);
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&io_conf);

  delay(50);  // Stabilize

  // Quick check - if HIGH, not pressed
  if (gpio_get_level(GPIO_NUM_0) == 1) {
    return false;
  }

  // Button might be pressed - verify with hold
  Serial.println("BOOT detectado - manten 2 seg para BLE...");

  int holdCount = 0;
  for (int i = 0; i < 20; i++) {
    delay(100);
    if (gpio_get_level(GPIO_NUM_0) == 0) holdCount++;
  }

  if (holdCount >= 15) {
    Serial.println("BLE mode activado");
    return true;
  }

  Serial.println("BOOT soltado - arrancando normal");
  return false;
}

// Display BLE config mode screen
void DisplayBLEConfigScreen() {
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  // Title
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 50, "CONFIGURACION BLUETOOTH", CENTER);

  // Bluetooth icon
  int cx = SCREEN_WIDTH / 2;
  int cy = 170;

  // Main vertical line
  fillRect(cx - 4, cy - 50, 8, 100, Black);

  // Top arrow
  fillTriangle(cx, cy - 50, cx + 30, cy - 20, cx, cy - 20, Black);
  fillTriangle(cx, cy - 50, cx - 30, cy - 20, cx, cy - 20, White);

  // Bottom arrow
  fillTriangle(cx, cy + 50, cx + 30, cy + 20, cx, cy + 20, Black);
  fillTriangle(cx, cy + 50, cx - 30, cy + 20, cx, cy + 20, White);

  // Center diamond
  fillTriangle(cx - 30, cy, cx, cy - 15, cx, cy + 15, Black);

  // Circle around icon
  drawCircle(cx, cy, 70, Black);

  // Instructions
  setFont(OpenSans12B);
  int y = 290;
  drawString(cx, y, "1. Abrir app 'Weather Station BLE'", CENTER);
  y += 35;
  drawString(cx, y, "2. Buscar y conectar: " BLE_DEVICE_NAME, CENTER);
  y += 35;
  drawString(cx, y, "3. Configurar parametros", CENTER);
  y += 35;
  drawString(cx, y, "4. Guardar y reiniciar", CENTER);

  // Status - with border box
  setFont(OpenSans12B);
  y = 465;
  if (bleDeviceConnected) {
    // Draw box with thick border for "CONECTADO"
    int boxW = 200, boxH = 35;
    for (int i = 0; i < 4; i++) {
      drawRect(cx - boxW/2 + i, y - 5 + i, boxW - i*2, boxH - i*2, Black);
    }
    drawString(cx, y + 2, "CONECTADO", CENTER);
  } else {
    drawString(cx, y + 2, "Esperando conexion...", CENTER);
  }

  // Footer
  y = 510;
  setFont(OpenSans8B);
  drawString(cx, y, "Presiona RST para cancelar y reiniciar normal", CENTER);

  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
}

#endif // BLE_CONFIG_H
