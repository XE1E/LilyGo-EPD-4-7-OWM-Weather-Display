# LilyGo EPD 4.7" Weather Station Manual

## Table of Contents

1. [Introduction](#1-introduction)
2. [Technical Specifications](#2-technical-specifications)
3. [System Architecture](#3-system-architecture)
4. [Installation and Compilation](#4-installation-and-compilation)
5. [Configuration](#5-configuration)
6. [Device Usage](#6-device-usage)
7. [Navigation Screens](#7-navigation-screens)
8. [OpenWeatherMap API](#8-openweathermap-api)
9. [Power Management](#9-power-management)
10. [3D Printed Case](#10-3d-printed-case)
11. [Troubleshooting](#11-troubleshooting)
12. [Special Functions](#12-special-functions)
    - [12.1 Calendar](#121-calendar)
    - [12.2 SD Card](#122-sd-card)
    - [12.3 Web Access](#123-web-access)
    - [12.4 Bluetooth Configuration](#124-bluetooth-configuration)
13. [Appendix](#13-appendix)

---

## 1. Introduction

### 1.1 General Description

The LilyGo EPD 4.7" Weather Station is an ESP32-S3 based device that displays real-time weather information from OpenWeatherMap. It uses a 4.7-inch e-paper (electronic ink) display that offers excellent visibility under any lighting condition and low power consumption.

### 1.2 Main Features

- **4.7-inch e-paper display** - 960x540 pixels, grayscale
- **Touch navigation** - GT911 controller with 11 navigable screens
- **Multi-WiFi** - Support for up to 3 configurable WiFi networks
- **AP Mode (Access Point)** - Captive portal for initial configuration
- **Built-in web server** - Configuration from any browser on the local network
- **Bluetooth configuration** - Android app for BLE setup
- **Deep Sleep** - Low consumption for battery operation
- **Multi-language** - Spanish, English, and French
- **Automatic update** - Configurable interval (5-120 minutes)
- **Data history** - Stores real readings for trends (SD: ~1 year, internal: ~7 days)

---

## 2. Technical Specifications

### 2.1 Hardware

#### Microcontroller
| Parameter | Specification |
|-----------|---------------|
| Chip | ESP32-S3 |
| CPU | Dual-core Xtensa LX7 @ 240MHz |
| RAM | 512KB SRAM + 8MB PSRAM (OPI) |
| Flash | 16MB |
| WiFi | 802.11 b/g/n (2.4GHz) |

#### E-Paper Display
| Parameter | Specification |
|-----------|---------------|
| Type | E-Ink (electronic ink) |
| Size | 4.7 inches diagonal |
| Resolution | 960 x 540 pixels |
| Colors | 16 grayscale levels |
| Technology | ED047TC1 |
| Controller | IT8951 |
| Refresh time | ~0.5 seconds |
| Viewing angle | ~180 degrees |
| Standby consumption | ~0 mW (static image) |

#### Touch Panel
| Parameter | Specification |
|-----------|---------------|
| Controller | GT911 |
| Interface | I2C |
| Pins | SDA=18, SCL=17, INT=47 |
| I2C Address | 0x5D or 0x14 |
| Touch points | Up to 5 simultaneous |

#### Power
| Parameter | Specification |
|-----------|---------------|
| USB input voltage | 5V |
| Battery voltage | 3.7V LiPo (3.2V-4.2V) |
| Active consumption | ~150mA |
| Deep sleep consumption | ~10uA |
| Battery ADC pin | GPIO 14 |

#### Buttons
| Parameter | Specification |
|-----------|---------------|
| BOOT button | GPIO 0 (bootloader mode) |
| RST button | Hardware reset |

#### SD Card Reader
| Parameter | Specification |
|-----------|---------------|
| Interface | SPI |
| CS (Chip Select) | GPIO 42 |
| MOSI | GPIO 15 |
| MISO | GPIO 16 |
| CLK | GPIO 11 |
| Supported formats | FAT32, exFAT |
| Maximum size | No limit (tested up to 64GB) |

### 2.2 E-Paper Technology Principles

#### How It Works

The e-paper display uses **bicolor microspheres** suspended in a fluid:

```
+------------------+
|  @@  @@  @@  @@ |  <-- White particles (charged +)
|  oo  oo  oo  oo |  <-- Black particles (charged -)
|==================|  <-- Upper electrode (transparent)
|  Fluid           |
|==================|  <-- Lower electrode
+------------------+
```

- **Positive voltage**: White particles rise, appears white
- **Negative voltage**: Black particles rise, appears black
- **No voltage**: Image stays indefinitely (bistable memory)

#### Advantages
1. **Visibility** - Perfect under direct sunlight
2. **Viewing angle** - Almost 180 degrees
3. **Consumption** - Only consumes energy when changing the image
4. **Visual comfort** - No backlight, doesn't strain eyes

#### Limitations
1. **Speed** - Slower refresh than LCD (~0.5s)
2. **Ghosting** - Residual images may remain
3. **Color** - Only grayscale (no color)
4. **Temperature** - Optimal operation 0-50C

### 2.3 Grayscale

The display supports 16 gray levels defined in code:

```cpp
#define White      0xFF   // Pure white
#define LightGrey  0xBB   // Light gray
#define Grey       0x88   // Medium gray
#define DarkGrey   0x44   // Dark gray
#define Black      0x00   // Pure black
```

---

## 3. System Architecture

### 3.1 Main Flow Diagram

```
                    +----------------+
                    |     START      |
                    +----------------+
                           |
                           v
                    +----------------+
                    | InitialiseSystem|
                    | - Serial.begin  |
                    | - epd_init      |
                    | - framebuffer   |
                    +----------------+
                           |
                           v
                    +----------------+
                    | InitializeTouch |
                    | - GT911 I2C     |
                    +----------------+
                           |
                           v
                    +----------------+
                    | loadConfig()    |
                    | - Preferences   |
                    | - applyConfig   |
                    +----------------+
                           |
                           v
                    +----------------+
                    | FORCE_AP_MODE? |----Yes----> AP Mode
                    +----------------+             |
                           |No                     |
                           v                       |
                    +----------------+             |
                    | StartWiFi()    |             |
                    | - Scan networks|             |
                    | - Best signal  |             |
                    +----------------+             |
                           |                       |
                      Connected?                   |
                      /        \                   |
                    Yes         No-----------------+
                    |                              |
                    v                              v
              +----------------+           +----------------+
              | SetupTime()    |           | startAPMode()  |
              | - NTP sync     |           | - DNS server   |
              +----------------+           | - Web server   |
                    |                      +----------------+
                    v                              |
              +----------------+                   |
              | obtainWeather  |                   |
              | - API weather  |                   |
              | - API forecast |                   |
              +----------------+                   |
                    |                              |
                    v                              |
              +----------------+                   |
              | DisplayWeather |                   |
              | - Render       |                   |
              +----------------+                   |
                    |                              |
                    v                              v
              +----------------+           +----------------+
              |   LOOP()       |           |   LOOP()       |
              | - Touch nav    |           | - handleAPMode |
              | - Web server   |           | - Retry WiFi   |
              | - 30s timeout  |           |   every 60s    |
              +----------------+           +----------------+
                    |
                    v (timeout)
              +----------------+
              | BeginSleep()   |
              | - deep_sleep   |
              +----------------+
```

### 3.2 File Structure

```
LilyGo-EPD-4-7-OWM-Weather-Display/
|
+-- LilyGo-EPD-4-7-OWM-Weather-Display.ino  # Main sketch
|
+-- owm_credentials.h     # WiFi and API credentials (defaults)
|
+-- wifi_manager.h        # AP mode, web portal, NVS storage
|
+-- forecast_record.h     # Weather data structure
|
+-- lang.h                # Multi-language system (ES/EN/FR)
|
+-- touch_handler.h       # GT911 touch controller
|
+-- weather_history.h     # Data history system (SD + FFat + PSRAM)
|
+-- opensans*.h           # Fonts (6, 8B, 9B, 10B, 12B, 14B, 16B, 18B, 24B, 28B)
|
+-- moon.h                # Moon image
+-- sunrise.h             # Sunrise icon
+-- sunset.h              # Sunset icon
```

### 3.3 Weather Data Structure

```cpp
typedef struct {
  int      Dt;           // Unix timestamp
  String   Period;       // Time period
  String   Icon;         // Icon code (01d, 02n, etc.)
  String   Trend;        // Pressure trend (+, -, 0)
  String   Main0;        // Main condition
  String   Forecast0;    // Weather description
  String   Description;  // Detailed description
  float    Temperature;  // Current temperature
  float    Feelslike;    // Feels like temperature
  float    Humidity;     // Humidity %
  float    High;         // Max temperature
  float    Low;          // Min temperature
  float    Winddir;      // Wind direction (degrees)
  float    Windspeed;    // Wind speed
  float    Rainfall;     // Rain mm
  float    Snowfall;     // Snow mm
  float    Pop;          // Probability of precipitation
  float    Pressure;     // Atmospheric pressure hPa
  int      Cloudcover;   // Cloud cover %
  int      Visibility;   // Visibility meters
  int      Sunrise;      // Unix timestamp sunrise
  int      Sunset;       // Unix timestamp sunset
  int      Timezone;     // Timezone offset
} Forecast_record_type;
```

### 3.4 Configuration Storage (NVS)

Configuration is stored in ESP32 Preferences namespace "weather":

| Key | Type | Description |
|-----|------|-------------|
| ssid1, pass1 | String | Primary WiFi network |
| ssid2, pass2 | String | Secondary WiFi network |
| ssid3, pass3 | String | Tertiary WiFi network |
| apikey | String | OpenWeatherMap API Key |
| fcdays | Int | Forecast days (3 or 5) |
| city | String | City name |
| lat, lon | String | Geographic coordinates |
| lang | String | Language (ES, EN, FR) |
| hemi | String | Hemisphere (north, south) |
| units | String | Units (M=metric, I=imperial) |
| tz | String | POSIX timezone |
| gmt | Int | GMT offset in seconds |
| dst | Int | Daylight saving offset |
| updint | Int | Update interval (min) |
| sleept | Int | Time before sleep (sec) |
| keepscr | Bool | Keep current screen on sleep |
| wakeuph | Int | Activity start hour (0-23) |
| sleeph | Int | Activity end hour (0-23) |

---

## 4. Installation and Compilation

### 4.1 Software Requirements

#### Arduino IDE
- Version 1.8.x or 2.x

#### Board Manager
- **URL**: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
- **Package**: esp32 by Espressif Systems **version 2.0.17**

#### Required Libraries
Only install these two libraries in the `libraries` folder:

1. **EPD47-master**
   - URL: https://github.com/DFRobotdl/EPD47/archive/refs/heads/master.zip
   - Extract as `EPD47-master`

2. **ArduinoJson**
   - Author: Benoit Blanchon
   - Version: 6.19.0

> **IMPORTANT**: Do not install other libraries to avoid conflicts.

### 4.2 Arduino IDE Configuration

In `Tools`, configure:

| Parameter | Value |
|-----------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enable |
| USB DFU On Boot | Disable |
| Flash Size | 16MB (128Mb) |
| Flash Mode | QIO 80MHz |
| Partition Scheme | 16M Flash (3M APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |
| Upload Mode | UART0/Hardware CDC |
| USB Mode | Hardware CDC and JTAG |

### 4.3 Firmware Upload

1. Connect the device via USB
2. Select the correct COM port
3. Click Upload

#### Forced Bootloader Mode

If upload fails, enter bootloader mode:

1. **Press and hold** the BOOT button (IO0)
2. **While holding** BOOT, press RST
3. **Release** RST
4. **Release** BOOT
5. Try uploading again

---

## 5. Configuration

### 5.1 File Configuration (owm_credentials.h)

Default values that can be overwritten via web:

```cpp
// WiFi networks (up to 3)
const WiFiCredentials wifiNetworks[] = {
  {"MyMainNetwork", "password123"},
  {"SecondaryNetwork", "anotherpassword"},
  {"TertiaryNetwork", "thirdpass"},
};

// OpenWeatherMap API
String apikey = "your_api_key_here";

// Groq API (for Weather Narrative - free at console.groq.com)
String groq_apikey = "your_groq_api_key_here";

// Location
String City      = "New York";
String Latitude  = "40.7128";
String Longitude = "-74.0060";

// Preferences
String Language   = "EN";        // ES, EN, FR
String Hemisphere = "north";     // north, south
String Units      = "M";         // M=metric, I=imperial

// Timezone
const char* Timezone    = "EST5EDT";
int gmtOffset_sec       = -18000;  // -5 hours
int daylightOffset_sec  = 3600;    // 1 hour DST
```

### 5.2 Web Portal Configuration (AP Mode)

#### Accessing AP Mode

AP mode activates automatically when:
- No configured WiFi networks are available
- `FORCE_AP_MODE = true` is set in code

#### Connection Data
| Parameter | Value |
|-----------|-------|
| SSID | WeatherStation-Setup |
| Password | weather123 |
| URL | http://192.168.4.1 |

#### AP Mode Screen

When AP mode is activated, the screen shows:

```
    WiFi Setup Mode

    Connect to WiFi network:
    WeatherStation-Setup

    Password: weather123

    Then open in browser:
    http://192.168.4.1
```

### 5.3 Web Configuration Page

The web page is organized in 4 tabs:

#### Tab 1: WiFi
- **Primary Network**: SSID and password
- **Secondary Network**: SSID and password (optional)
- **Tertiary Network**: SSID and password (optional)

#### Tab 2: Weather
- **API Key**: OpenWeatherMap key
- **Forecast Days**: 3 or 5 days
- **City**: Display name
- **Latitude/Longitude**: Exact coordinates
- **Hemisphere**: North or South (affects moon phases)

#### Tab 3: Display
- **Language**: Spanish, English, French
- **Units**: Metric (C, m/s, hPa) or Imperial (F, mph, inHg)
- **Timezone**: POSIX timezone (e.g., EST5EDT)
- **GMT Offset**: Offset in seconds
- **DST Offset**: Daylight saving in seconds

#### Tab 4: System
- **Update Interval**: 5-120 minutes
- **Time before Sleep**: 10-300 seconds
- **On sleep**: Return to main screen or keep current screen
- **Start Hour (wake)**: Hour from which to update weather (0-23)
- **End Hour (sleep)**: Hour from which to stop updating (0-23)
- **Narrative Style**: AI-generated text style (see section 12.5.5)
- **Save**: Only saves changes
- **Save and Restart**: Saves and applies changes
- **Factory Reset**: Erases all configuration

### 5.4 Common Timezones

| City | Timezone | GMT Offset |
|------|----------|------------|
| Mexico City | CST6 | -21600 |
| New York | EST5EDT | -18000 |
| Los Angeles | PST8PDT | -28800 |
| Madrid | CET-1CEST | 3600 |
| London | GMT0BST | 0 |
| Tokyo | JST-9 | 32400 |
| Sydney | AEST-10AEDT | 36000 |

---

## 6. Device Usage

### 6.1 Normal Startup

1. **Power on** - Device starts automatically
2. **WiFi connection** - Searches for configured networks
3. **Synchronization** - Gets time via NTP
4. **Weather data** - Downloads from OpenWeatherMap
5. **History** - Saves reading to local history
6. **Display** - Shows configured screen (main or last visited)
7. **Web server** - Available for configuration at http://[LOCAL_IP]
8. **Navigation** - 30 seconds to interact
9. **Sleep** - Enters low power mode

### 6.2 Update Cycle

```
[WAKE UP] --> [WiFi] --> [NTP] --> [API] --> [Display] --> [Sleep]
    ^                                                          |
    |                                                          |
    +------ (configured interval, e.g., 30 minutes) -----------+
```

### 6.3 Screen Indicators

#### Status Bar (upper right corner)

```
  [SD]  [Battery]  [WiFi]
        85% 4.1v   -65dB
```

| Indicator | Description |
|-----------|-------------|
| SD | SD card icon (only visible if SD inserted) |
| Battery | Icon with charge level, percentage and voltage below |
| WiFi | Signal bars (1-5) with RSSI value in dB |

---

## 7. Navigation Screens

### 7.1 Touch Navigation System

The device has **11 navigable screens** by touching different zones:

```
Main Screen (SCREEN_MAIN)
+-------------------------------------+
| [City]         [SD][Bat][WiFi]<-|  Info Zone (upper right corner)
| [Date] @ [Time]                 |  -> Screen 4: System Info
+==================================+
|                                  |
| [Wind Rose]    [Temp] [Hum]     |  Upper zone
|                [Max/Min]        |  -> Screen 1: Current
| [Moon][Sun] <- [Description]    |     Conditions
|  Calendar      [ICON]           |  Moon/Sun Zone -> Calendar
|                                  |
+==================================+
| [+3h] [+6h] [+9h] ... [+21h]    |  Middle zone (hourly forecast)
|  Hourly forecast with icons     |  -> Screen 2: Forecast
+================+=================+
| [Temp] [Press] | [Hum] [Rain]   |  Lower zone DIVIDED:
|   Graphs       |    Graphs      |  Left -> Screen 5: History
|   (3 days)     |    (3 days)    |  Right -> Screen 3: Trends
+----------------+-----------------+
```

**Return**: Touch any part of a secondary screen to return to main.

### 7.2 Main Screen

```
+--[City]---------------[SD][Battery][WiFi]--+
|                            85% 4.1v        |
|  [Date]  @ [Time]                          |
+----------------------------------------------+
|                                              |
| [Wind Rose]      [Temperature] [Humidity]    |
|     [Direction]  [Max/Min]                   |
|     [Speed]      [Weather description]       |
|                  [Feels like]                |
|                                              |
| [Moon]  [Sunrise]                [ICON]      |
| [Phase] [Sunset]                 [Large]     |
|                                              |
+----------------------------------------------+
| [+3h] [+6h] [+9h] [+12h] [+15h] [+18h] [+21h]|
|  Hourly forecast (7 columns)                 |
+----------------------------------------------+
| [Temperature] [Pressure] [Humidity] [Rain]   |
|    4 mini-graphs of trends (3 days)          |
+----------------------------------------------+
```

### 7.3 Screen 1: Current Conditions

**Access**: Touch the upper zone of main screen (temperature/weather area).

### 7.4 Screen 2: Extended Forecast

**Access**: Touch the middle zone of main screen (hourly forecast area).

### 7.5 Screen 3: Weather Trends

**Access**: Touch the RIGHT half of the graphs in main screen.

**Note**: These graphs show TRENDS (future predictions) from OpenWeatherMap API, not actual historical data.

### 7.6 Screen 4: System Information

**Access**: Touch the upper right corner (battery/WiFi zone).

The Information section consists of **5 navigable sub-screens** (0/4 to 4/4).

### 7.7 Screen 5: Data History

**Access**: Touch the LEFT half of the graphs in main screen.

Shows actual historical data recorded by the device.

### 7.8 Screen 6: Air Quality

**Access**: Touch the **AQI** zone in Current Conditions screen.

---

## 8. OpenWeatherMap API

### 8.1 Getting an API Key

1. Go to https://openweathermap.org/
2. Create a free account
3. Go to "API Keys" in profile
4. Copy or generate a new key

### 8.2 Free Plan Limits

| Feature | Limit |
|---------|-------|
| Calls/minute | 60 |
| Calls/month | 1,000,000 |
| Historical data | No |
| Forecast | 5 days / 3 hours |

### 8.3 Endpoints Used

#### Current Weather
```
GET /data/2.5/weather
Parameters:
  lat={latitude}
  lon={longitude}
  appid={api_key}
  units=metric|imperial
  lang={language_code}
```

#### Forecast
```
GET /data/2.5/forecast
Parameters:
  lat={latitude}
  lon={longitude}
  appid={api_key}
  units=metric|imperial
  lang={language_code}
  cnt=40  (8 readings/day x 5 days)
```

---

## 9. Power Management

### 9.1 Operating Modes

| Mode | Consumption | Description |
|------|-------------|-------------|
| Active | ~150mA | WiFi + Display + CPU |
| Display Off | ~80mA | WiFi + CPU |
| Light Sleep | ~2mA | CPU paused |
| Deep Sleep | ~10uA | RTC only |

### 9.2 Typical Power Cycle

```
         150mA        0mA (e-paper)      10uA
           |            |                  |
[Active]---+--[Display]+---[Deep Sleep]----+
  ~15s        ~0.5s       ~30 minutes

Average: ~0.1mA (with 2000mAh battery = ~20,000 hours)
```

### 9.3 Battery Monitoring

- **ADC Pin**: GPIO 14
- **Formula**: `voltage = analogRead(14) / 4096.0 * 6.566 * (vref / 1000.0)`
- **Range**: 3.2V (0%) to 4.2V (100%)

---

## 10. 3D Printed Case

### 10.1 Recommended Model

**Simple case for LilyGO T5 e-Paper 4.7" ESP32-S3 Board [H716]**
- **Author**: n602
- **URL**: https://www.thingiverse.com/thing:6897183
- **License**: Creative Commons BY-NC-SA 4.0

---

## 11. Troubleshooting

### 11.1 Won't Connect to WiFi

**Symptoms**: Enters AP mode every time

**Solutions**:
1. Verify SSID is written exactly the same
2. Verify password
3. Ensure network is 2.4GHz (not 5GHz)
4. Move device closer to router

### 11.2 No Weather Data

**Symptoms**: Blank screen or "?"

**Solutions**:
1. Verify OpenWeatherMap API Key
2. Verify location coordinates
3. Verify internet connection
4. Check Serial Monitor for errors

### 11.3 Screen Ghosting

**Symptoms**: Previous images visible

**Solutions**:
1. Use [CLEAN SCREEN] button in System Info screen
2. Next full refresh will clean it
3. Restart device to force cleaning

### 11.4 Touch Not Responding

**Symptoms**: Won't navigate between screens

**Solutions**:
1. Verify in Serial that GT911 was detected
2. Clean screen of dust/grease
3. Touch firmly for at least 50ms
4. Wait 500ms between touches (debounce)

### 11.5 Won't Upload Firmware

**Symptoms**: Connection error in Arduino IDE

**Solutions**:
1. Use forced bootloader mode (section 4.3)
2. Try another USB cable
3. Verify USB drivers installed
4. Close Serial Monitor before uploading

---

## 12. Special Functions

### 12.1 Calendar

The device includes a complete calendar system with monthly and yearly views.

#### 12.1.1 Calendar Access

From the main screen:

1. **Locate activation zone**: Moon, sunrise, and sunset icon area (upper left corner)
2. **Touch the zone**: Normal touch activates calendar
3. **Activation**: Screen changes to monthly calendar

#### 12.1.2 Monthly Calendar

Shows a complete month with:

| Element | Description |
|---------|-------------|
| Title | Month name and year (e.g., "March 2026") |
| Double arrows | Month navigation (left/right) |
| Headers | MON, TUE, WED, THU, FRI, SAT, SUN |
| Days | Numbers 1 to 28/29/30/31 depending on month |
| Current day | Marked with double underline |
| Week | Starts on Monday |

#### 12.1.3 Yearly Calendar

Shows 12 months in a 3 columns x 4 rows grid.

### 12.2 SD Card

The device supports extended storage via MicroSD card.

#### 12.2.1 Features

| Feature | Description |
|---------|-------------|
| Format | FAT32 (up to 32GB) or exFAT (64GB+) |
| File | `/weather_history.csv` |
| Capacity | ~52,560 readings (~1 year at 10 min) |
| Fallback | If no SD, uses internal FFat (~7 days) |

#### 12.2.2 CSV File Format

```csv
timestamp,temp,humidity,pressure,wind_speed,wind_dir,rain,description
1710000000,22.5,65,1013,5.2,180,0.0,Partly cloudy
```

### 12.3 Web Access

The device includes a web server for configuration and data access.

#### 12.3.1 Available Endpoints

| Endpoint | Description |
|----------|-------------|
| `http://[IP]/` | Configuration page (4 tabs) |
| `http://[IP]/history` | Last 50 readings in HTML table |
| `http://[IP]/history.csv` | Download complete CSV (requires SD) |

### 12.4 Bluetooth Configuration

The device supports configuration via Bluetooth Low Energy (BLE) using an Android app.

#### 12.4.1 BLE Mode Activation

1. Navigate to **System Configuration** screen (Info)
2. Touch **Bluetooth** button (lower left corner)
3. Screen will show "BLUETOOTH CONFIGURATION"
4. Device will appear as **WeatherStation-BLE**

### 12.5 Hidden Screens

#### 12.5.1 Access Path

```
Main Screen ---------> Weather Narrative (AI)
      |                (touch large center icon)
      v
Info (battery/WiFi) -----> Features 1 -----> Features 2
                                                  |
                                                  v
                                             Help -----> Credits (QR)
                                                              |
                                                              v
                                                         Callsign (XE1E)
                                                              |
                                                              v
                                                         World Clock
                                                              |
                                                              v
                                                    Thought of the Day
```

#### 12.5.2 Callsign Screen

Access: Credits (QR) -> touch lower zone

Shows the ham radio callsign of the author.

#### 12.5.3 World Clock Screen

Access: Callsign -> touch lower zone

Shows a world map with timezone zones.

#### 12.5.4 Thought of the Day Screen

Access: World Clock -> touch right side

Shows an inspirational daily quote from the internet:

| Element | Description |
|---------|-------------|
| Frame | Decorative with elegant corners and borders |
| Title | "~ Thought of the Day ~" |
| Quote | Text of the quote |
| Author | Name of the quote author |
| Footer | "Touch to return" |

Features:
- Daily quote (API: frasedeldia.azurewebsites.net)
- Same quote shown all day, changes every 24 hours
- Persists in deep sleep
- Elegant format with decorative quotation marks

#### 12.5.5 Weather Narrative Screen (AI)

Access: Main screen -> touch large weather icon (center of screen)

Generates a natural weather description using artificial intelligence (Groq/Llama):

| Element | Description |
|---------|-------------|
| Frame | Decorative with elegant corners and borders |
| Title | "~ Today's Weather ~" |
| City | Configured location name |
| Narrative | Weather description in 4-5 sentences |
| Footer | "Touch to return" |

##### Narrative Styles

Style is configured from web interface (System Section -> Weather Narrative):

| Style | Description | Example |
|-------|-------------|---------|
| Radio | Radio bulletin, concise | "Good morning listeners, today we have 22 degrees..." |
| Formal | Like TV news | "Current meteorological conditions indicate..." |
| Poetic | Literary metaphors | "The sun bathes the city in its warm embrace..." |
| Technical | Meteorological style | "High pressure system. Temperature: 22C..." |
| Humorous | With humor touches | "Neither too cold nor too hot, the weather is undecided..." |
| Grandma | Grandma's advice | "Honey, put on a sweater, it's 22 degrees out there..." |

##### How to Configure

1. Access web interface:
   - AP Network: `WeatherStation-Setup` (pass: `weather123`) -> `http://192.168.4.1`
   - Or use device IP if already connected to your network
2. Go to **System** tab
3. Find **"Weather Narrative (AI)"** section
4. Select desired style from dropdown menu
5. Save configuration

##### Data Used

AI generates text using real OpenWeatherMap data (doesn't invent numbers):
- Current temperature and feels like
- Humidity and atmospheric pressure
- Sky condition (cloudy, sunny, etc.)
- Wind speed
- Next hours forecast

##### Requirements

- Active WiFi connection (to call Groq API)
- Groq API key configured in `owm_credentials.h` or via web
- Get free key at: https://console.groq.com

##### Features

- Text is generated each time you enter the screen
- **Multilingual**: Generates in configured language (ES/EN/FR)
- Persists in deep sleep
- Elegant format with decorative frame
- OpenSans10B font for better readability
- Up to 9 lines of ~69 characters each
- AI Model: Llama 3.1 8B (fast and free)

Navigation:
- Touch any zone -> Returns to main screen

---

## 13. Appendix

### 13.1 Available Fonts

| Name | Size | Use |
|------|------|-----|
| OpenSans6 | 6pt | Very small text |
| OpenSans8B | 8pt | Small text, hints, uptime |
| OpenSans9B | 9pt | Graph labels |
| OpenSans10B | 10pt | Date/time, details, system info |
| OpenSans12B | 12pt | Medium text, buttons |
| OpenSans14B | 14pt | City, descriptions |
| OpenSans18B | 18pt | Screen titles |
| OpenSans24B | 24pt | Temperature, main data |
| OpenSans28B | 28pt | Extra large numbers |

### 13.2 Screen Coordinates

```
(0,0)--------------------------(960,0)
  |                               |
  |        960 x 540 pixels       |
  |                               |
(0,540)-----------------------(960,540)
```

### 13.3 Wind Directions

Abbreviations vary by configured language:

| Degrees | Spanish | English | French |
|---------|---------|---------|--------|
| 0 / 360 | N | N | N |
| 45 | NE | NE | NE |
| 90 | E | E | E |
| 135 | SE | SE | SE |
| 180 | S | S | S |
| 225 | SO | SW | SO |
| 270 | O | W | O |
| 315 | NO | NW | NO |

### 13.4 Moon Phases

| Phase | Illumination | Name EN | Name ES |
|-------|--------------|---------|---------|
| 0 | 0% | New Moon | Luna Nueva |
| 1 | 25% | Waxing Crescent | Luna Creciente |
| 2 | 50% | First Quarter | Cuarto Creciente |
| 3 | 75% | Waxing Gibbous | Gibosa Creciente |
| 4 | 100% | Full Moon | Luna Llena |
| 5 | 75% | Waning Gibbous | Gibosa Menguante |
| 6 | 50% | Third Quarter | Cuarto Menguante |
| 7 | 25% | Waning Crescent | Luna Menguante |

### 13.5 Multi-language Texts

The system supports 3 configurable languages:

| Text | Spanish | English | French |
|------|---------|---------|--------|
| Conditions | Condiciones Actuales | Current Conditions | Conditions Actuelles |
| Forecast | Pronostico 5 Dias | 5-Day Forecast | Previsions 5 Jours |
| Trends | Tendencias del Clima | Weather Trends | Tendances Meteo |
| History | Historial | History | Historique |
| System Info | Info del Sistema | System Info | Info Systeme |
| Sunrise | Amanecer | Sunrise | Lever |
| Sunset | Anochecer | Sunset | Coucher |
| Humidity | Humedad | Humidity | Humidite |
| Pressure | Presion | Pressure | Pression |
| Wind | Viento | Wind | Vent |
| Clean | Limpiar Pantalla | Clean Screen | Nettoyer Ecran |
| 48 Hours | 48H | 48H | 48H |
| 1 Week | 1 Semana | 1 Week | 1 Semaine |

### 13.6 License and Credits

**Original Software**: David Bird 2021
**Modifications**: XE1E 2024

This project is open source. Check repository for license details.

---

*Manual Version 2.1 - March 2026*
