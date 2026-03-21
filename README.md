# LilyGo EPD 4.7" Weather Display - Touch Version

Weather station for LilyGo T5 4.7" e-paper display with full touch navigation.

## Features

- **Touch navigation** - 11+ screens navigable by touch
- **Current weather** - Temperature, humidity, pressure, wind, UV Index, Air Quality
- **3-day forecast** - Hourly and daily weather predictions
- **Weather graphs** - Temperature, pressure, humidity, precipitation trends
- **Data history** - Up to 1 year of recorded weather data (with SD card)
- **Moon phase** - Current lunar phase with icon
- **Sunrise/sunset** - Daily solar times
- **Calendar** - Monthly and yearly views
- **Weather narrative** - AI-generated weather descriptions in selected language (Groq/Llama)
- **Quote of the day** - Daily inspirational quotes
- **World clock** - Timezone map
- **Web configuration** - Configure via WiFi AP, no recompilation needed
- **Bluetooth configuration** - Android app for BLE setup
- **Multi-language** - Spanish, English, French
- **Multi-WiFi** - Connects to strongest available network from up to 3 configured
- **Deep sleep** - Battery-friendly operation with configurable update interval
- **SD card support** - Extended history storage (~52,000 readings)

## Hardware

**Required:** LilyGo T5 4.7" S3 Touch (ESP32-S3, 960x540 e-paper, GT911 touch)

Optional: MicroSD card for extended weather history

## Quick Start

### 1. Upload Firmware

**Arduino IDE Settings:**
| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enable |
| USB DFU On Boot | Disable |
| Flash Size | 16MB (128Mb) |
| Flash Mode | QIO 80MHz |
| Partition Scheme | 16M Flash (3M APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |
| Upload Mode | UART0/Hardware CDC |
| USB Mode | Hardware CDC and JTAG |

**Required Libraries:**
- Board Manager: esp32 by Espressif Systems 2.0.17
- EPD47-master: https://github.com/DFRobotdl/EPD47/archive/refs/heads/master.zip
- ArduinoJson: by Benoit Blanchon 6.19.0

### 2. First Boot Configuration

On first power-up (or when no WiFi available), the device enters configuration mode:

1. Connect to WiFi network: `WeatherStation-Setup`
2. Password: `weather123`
3. Open browser: `http://192.168.4.1`
4. Enter your settings (API keys, location, etc.)
5. Click Save - device restarts and displays weather

### 3. Normal Operation

After configuration, the device:
1. Connects to WiFi
2. Fetches weather from OpenWeatherMap
3. Displays weather on screen
4. Allows 30 seconds of touch navigation
5. Enters deep sleep (configurable interval)
6. Wakes up and repeats

## Touch Navigation

| Screen Zone | Action |
|-------------|--------|
| Large weather icon (center) | Weather Narrative (AI) |
| Battery/WiFi area (top-right) | System Info |
| Moon/Sun area (top-left) | Calendar |
| Temperature/Weather (upper) | Current Conditions |
| Hourly forecast (middle) | Extended Forecast |
| Graphs left half | Weather History |
| Graphs right half | Forecast Trends |

## Configuration Options

| Field | Description | Example |
|-------|-------------|---------|
| WiFi (up to 3) | Network SSID and password | MyWiFi / password123 |
| OWM API Key | OpenWeatherMap API key | abc123... |
| Groq API Key | For AI narrative (free) | gsk_... |
| City | City name for display | Mexico City |
| Latitude | Location latitude | 19.4326 |
| Longitude | Location longitude | -99.1332 |
| Timezone | POSIX timezone string | CST6 |
| Update interval | Minutes between updates | 30 |
| Language | Interface language | ES / EN / FR |
| Units | Metric or Imperial | M / I |
| Narrative style | AI text style | Radio, Formal, Poetic... |

## API Keys

### OpenWeatherMap (required)
1. Sign up at https://openweathermap.org/
2. Go to API Keys section
3. Copy your key

### Groq (optional, for Weather Narrative)
1. Sign up at https://console.groq.com/
2. Create an API key
3. Free tier: 30 requests/minute

## Screens

| # | Screen | Access |
|---|--------|--------|
| 1 | Main Weather | Default |
| 2 | Current Conditions | Touch upper area |
| 3 | Extended Forecast | Touch hourly icons |
| 4 | Weather Trends | Touch right graphs |
| 5 | Weather History | Touch left graphs |
| 6 | System Info (5 pages) | Touch battery/WiFi |
| 7 | Air Quality | From Current Conditions |
| 8 | Calendar (Monthly/Yearly) | Touch moon/sun area |
| 9 | Weather Narrative | Touch large icon |
| 10 | Quote of the Day | Hidden: Info > Credits > ... |
| 11 | World Clock | Hidden navigation |

## Troubleshooting

### Upload fails
1. Press and hold BOOT button
2. While holding BOOT, press RST
3. Release RST, then release BOOT
4. Upload should now work

### Touch not responding
- Clean the screen surface
- Check GT911 detection in Serial Monitor
- Touch firmly for at least 50ms

### No weather data
- Check your OpenWeatherMap API key is valid
- Verify latitude/longitude are correct
- Check WiFi credentials

### Ghosting on screen
- Use "Clean Screen" button in System Info
- Or restart the device

## Files

| File | Description |
|------|-------------|
| `LilyGo-EPD-4-7-OWM-Weather-Display.ino` | Main sketch |
| `wifi_manager.h` | AP mode and web server |
| `owm_credentials.h` | Default configuration |
| `lang.h` | Multi-language strings |
| `touch_handler.h` | Touch navigation |
| `weather_narrative.h` | AI weather descriptions |
| `weather_history.h` | Data storage system |
| `calendar.h` | Calendar views |
| `quote_screen.h` | Quote of the day |
| `forecast_record.h` | Weather data structure |
| `opensans*.h` | Font files |

## Technical Details

- **Display:** 960x540 pixels, 4-bit grayscale
- **Touch:** GT911 capacitive controller
- **MCU:** ESP32-S3 with 8MB PSRAM
- **Power:** Deep sleep between updates (~10uA)
- **Storage:** NVS + FFat (~7 days) + SD card (~1 year)
- **Wake source:** Timer

## Documentation

See detailed manuals in three languages:
- [MANUAL.md](MANUAL.md) - Español
- [MANUAL_EN.md](MANUAL_EN.md) - English
- [MANUAL_FR.md](MANUAL_FR.md) - Français

## License

Based on original work by David Bird - See Licence.txt

## Credits

- Original code by David Bird 2021
- ESP32 port by Xinyuan-LilyGO
- Modified by Stefan Maetschke 2025
- Touch version with extensions by XE1E 2026
- EPD47 library by Vroland/DFRobot
- Weather data from OpenWeatherMap
- AI narrative by Groq/Llama

---

73 de XE1E
