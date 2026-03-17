#ifndef WEATHER_HISTORY_H
#define WEATHER_HISTORY_H

#include <FFat.h>
#include <SD.h>
#include <esp_heap_caps.h>

// Maximum readings in PSRAM buffer (for display/graphs)
#define MAX_HISTORY_READINGS 1000
// Maximum readings on SD card (at 10 min intervals = ~1 year)
#define MAX_SD_READINGS 52560
#define HISTORY_FILE "/weather_history.dat"
#define SD_HISTORY_FILE "/weather_history.csv"

// External reference to SD availability (set in main .ino)
extern bool sdCardAvailable;

// Structure for a single weather reading (16 bytes)
struct WeatherReading {
  uint32_t timestamp;    // Unix timestamp
  int16_t temperature;   // Temperature * 10 (e.g., 25.5 = 255)
  uint16_t humidity;     // Humidity %
  uint16_t pressure;     // Pressure hPa
  uint16_t rainfall;     // Rainfall * 100 (e.g., 1.25mm = 125)
  int16_t feelslike;     // Feels like * 10
};

// History buffer - allocated in PSRAM to save regular RAM
WeatherReading* historyBuffer = nullptr;
int historyCount = 0;
int historyIndex = 0;  // Next write position (circular buffer)
bool historyInitialized = false;

// Forward declarations for SD functions (defined below)
int countSDHistoryReadings();
int loadFromSDHistory(int maxToLoad);
bool appendToSDHistory(const WeatherReading& reading);
void trimSDHistory();

// Initialize FFat and load history (from SD if available, otherwise FFat)
bool initWeatherHistory() {
  // Allocate buffer in PSRAM
  if (historyBuffer == nullptr) {
    historyBuffer = (WeatherReading*)heap_caps_calloc(MAX_HISTORY_READINGS, sizeof(WeatherReading), MALLOC_CAP_SPIRAM);
    if (historyBuffer == nullptr) {
      Serial.println("ERROR: Failed to allocate history buffer!");
      return false;
    }
  }

  // Always mount FFat (used as backup storage)
  if (!FFat.begin(true)) {
    Serial.println("FFat mount failed!");
    return false;
  }

  // Count readings in each storage
  int sdCount = 0;
  int ffatCount = 0;

  if (sdCardAvailable) {
    sdCount = countSDHistoryReadings();
  }

  if (FFat.exists(HISTORY_FILE)) {
    File file = FFat.open(HISTORY_FILE, "r");
    if (file) {
      file.read((uint8_t*)&ffatCount, sizeof(ffatCount));
      file.close();
      if (ffatCount < 0 || ffatCount > MAX_HISTORY_READINGS) ffatCount = 0;
    }
  }

  Serial.printf("History storage: SD=%d, FFat=%d readings\n", sdCount, ffatCount);

  // Load from whichever has more data
  bool loadedFromSD = false;
  if (sdCardAvailable && sdCount > ffatCount) {
    if (loadFromSDHistory(MAX_HISTORY_READINGS) > 0) {
      loadedFromSD = true;
    }
  }

  // Load from FFat if not loaded from SD
  if (!loadedFromSD && ffatCount > 0) {
    File file = FFat.open(HISTORY_FILE, "r");
    if (file) {
      file.read((uint8_t*)&historyCount, sizeof(historyCount));
      file.read((uint8_t*)&historyIndex, sizeof(historyIndex));

      if (historyCount < 0 || historyCount > MAX_HISTORY_READINGS) {
        historyCount = 0;
        historyIndex = 0;
      } else if (historyCount > 0) {
        int toRead = min(historyCount, MAX_HISTORY_READINGS);
        file.read((uint8_t*)historyBuffer, toRead * sizeof(WeatherReading));
      }
      file.close();

      // Migrate FFat data to SD if SD has less
      if (sdCardAvailable && sdCount < historyCount) {
        if (SD.exists(SD_HISTORY_FILE)) {
          SD.remove(SD_HISTORY_FILE);
        }
        for (int i = 0; i < historyCount; i++) {
          appendToSDHistory(historyBuffer[i]);
        }
        Serial.printf("Migrated %d readings to SD\n", historyCount);
      }
    }
  }

  if (historyCount == 0) {
    historyIndex = 0;
  }

  historyInitialized = true;
  Serial.printf("History ready: %d readings (%s)\n",
    historyCount, sdCardAvailable ? "SD+FFat" : "FFat");
  return true;
}

// Save history to FFat
bool saveWeatherHistory() {
  if (!historyInitialized || historyBuffer == nullptr) return false;

  File file = FFat.open(HISTORY_FILE, "w");
  if (!file) return false;

  file.write((uint8_t*)&historyCount, sizeof(historyCount));
  file.write((uint8_t*)&historyIndex, sizeof(historyIndex));
  int toWrite = min(historyCount, MAX_HISTORY_READINGS);
  file.write((uint8_t*)historyBuffer, toWrite * sizeof(WeatherReading));
  file.close();
  return true;
}

// Add a new reading to history
void addWeatherReading(float temp, float humidity, float pressure, float rainfall, float feelslike) {
  if (!historyInitialized || historyBuffer == nullptr) return;

  WeatherReading reading;
  reading.timestamp = time(nullptr);
  reading.temperature = (int16_t)(temp * 10);
  reading.humidity = (uint16_t)humidity;
  reading.pressure = (uint16_t)pressure;
  reading.rainfall = (uint16_t)(rainfall * 100);
  reading.feelslike = (int16_t)(feelslike * 10);

  // Add to PSRAM buffer
  historyBuffer[historyIndex] = reading;
  historyIndex = (historyIndex + 1) % MAX_HISTORY_READINGS;

  if (historyCount < MAX_HISTORY_READINGS) {
    historyCount++;
  }

  // Save to FFat (backup) and SD (extended history)
  saveWeatherHistory();

  if (sdCardAvailable) {
    appendToSDHistory(reading);
    // Trim SD file periodically
    static int appendCount = 0;
    if (++appendCount >= 100) {
      appendCount = 0;
      trimSDHistory();
    }
  }
}

// Get reading at index (0 = oldest, historyCount-1 = newest)
bool getWeatherReading(int index, WeatherReading &reading) {
  if (historyBuffer == nullptr || index < 0 || index >= historyCount) return false;

  // Calculate actual buffer position
  int bufferPos;
  if (historyCount < MAX_HISTORY_READINGS) {
    bufferPos = index;
  } else {
    bufferPos = (historyIndex + index) % MAX_HISTORY_READINGS;
  }

  reading = historyBuffer[bufferPos];
  return true;
}

// Get readings for last N hours (for graphing)
int getReadingsForHours(int hours, float* tempArray, float* humArray, float* pressArray, float* rainArray, int maxReadings) {
  if (!historyInitialized || historyCount == 0) return 0;

  uint32_t now = time(nullptr);
  uint32_t cutoff = now - (hours * 3600);

  int count = 0;

  // Start from newest and go back
  for (int i = historyCount - 1; i >= 0 && count < maxReadings; i--) {
    WeatherReading reading;
    if (getWeatherReading(i, reading)) {
      if (reading.timestamp >= cutoff) {
        int idx = count;
        tempArray[idx] = reading.temperature / 10.0;
        humArray[idx] = reading.humidity;
        pressArray[idx] = reading.pressure;
        rainArray[idx] = reading.rainfall / 100.0;
        count++;
      } else {
        break;  // Older than cutoff, stop
      }
    }
  }

  // Reverse arrays so oldest is first
  for (int i = 0; i < count / 2; i++) {
    int j = count - 1 - i;
    float tmp;
    tmp = tempArray[i]; tempArray[i] = tempArray[j]; tempArray[j] = tmp;
    tmp = humArray[i]; humArray[i] = humArray[j]; humArray[j] = tmp;
    tmp = pressArray[i]; pressArray[i] = pressArray[j]; pressArray[j] = tmp;
    tmp = rainArray[i]; rainArray[i] = rainArray[j]; rainArray[j] = tmp;
  }

  return count;
}

// Get total days of history available
float getHistoryDays() {
  if (historyCount < 2) return 0;

  WeatherReading oldest, newest;
  getWeatherReading(0, oldest);
  getWeatherReading(historyCount - 1, newest);

  return (newest.timestamp - oldest.timestamp) / 86400.0;
}

// Get actual hours span from oldest to newest reading in the requested range
float getHistoryHoursSpan(int requestedHours) {
  if (!historyInitialized || historyCount < 2) return 0;

  uint32_t now = time(nullptr);
  uint32_t cutoff = now - (requestedHours * 3600);

  uint32_t oldestTimestamp = 0;
  uint32_t newestTimestamp = 0;

  // Find oldest and newest readings within range
  for (int i = 0; i < historyCount; i++) {
    WeatherReading reading;
    if (getWeatherReading(i, reading)) {
      if (reading.timestamp >= cutoff) {
        if (oldestTimestamp == 0) oldestTimestamp = reading.timestamp;
        newestTimestamp = reading.timestamp;
      }
    }
  }

  if (oldestTimestamp == 0 || newestTimestamp == 0) return 0;
  return (newestTimestamp - oldestTimestamp) / 3600.0;
}

// Get start hour of oldest data point in range (returns hour 0-23)
int getHistoryStartHour(int requestedHours) {
  if (!historyInitialized || historyCount == 0) return 0;

  uint32_t now = time(nullptr);
  uint32_t cutoff = now - (requestedHours * 3600);

  // Find oldest reading within the requested range
  for (int i = 0; i < historyCount; i++) {
    WeatherReading reading;
    if (getWeatherReading(i, reading)) {
      if (reading.timestamp >= cutoff) {
        struct tm* tm_info = localtime((time_t*)&reading.timestamp);
        return tm_info->tm_hour;
      }
    }
  }
  return 0;
}

// Clear all history
void clearWeatherHistory() {
  historyCount = 0;
  historyIndex = 0;
  if (FFat.exists(HISTORY_FILE)) {
    FFat.remove(HISTORY_FILE);
  }
  // Also clear SD history if available
  if (sdCardAvailable && SD.exists(SD_HISTORY_FILE)) {
    SD.remove(SD_HISTORY_FILE);
  }
}

// ============== SD Card Extended History Functions ==============

// Append a single reading to SD card CSV
bool appendToSDHistory(const WeatherReading& reading) {
  if (!sdCardAvailable) return false;

  bool fileExists = SD.exists(SD_HISTORY_FILE);
  File file = SD.open(SD_HISTORY_FILE, FILE_APPEND);
  if (!file) {
    Serial.println("SD: Write error");
    return false;
  }

  // Write CSV header if new file
  if (!fileExists) {
    file.println("timestamp,temperature,humidity,pressure,rainfall,feelslike");
  }

  // Write reading as CSV line
  file.printf("%lu,%.1f,%u,%u,%.2f,%.1f\n",
    reading.timestamp,
    reading.temperature / 10.0,
    reading.humidity,
    reading.pressure,
    reading.rainfall / 100.0,
    reading.feelslike / 10.0);

  file.close();
  return true;
}

// Count lines in SD history file
int countSDHistoryReadings() {
  if (!sdCardAvailable || !SD.exists(SD_HISTORY_FILE)) return 0;

  File file = SD.open(SD_HISTORY_FILE, FILE_READ);
  if (!file) return 0;

  int count = -1;  // Start at -1 to skip header line
  while (file.available()) {
    if (file.read() == '\n') count++;
  }
  file.close();

  return max(0, count);
}

// Load last N readings from SD into PSRAM buffer
int loadFromSDHistory(int maxToLoad) {
  if (!sdCardAvailable || !SD.exists(SD_HISTORY_FILE)) return 0;

  File file = SD.open(SD_HISTORY_FILE, FILE_READ);
  if (!file) return 0;

  // Skip header
  file.readStringUntil('\n');

  // Count total lines first
  int totalLines = 0;
  long startPos = file.position();
  while (file.available()) {
    if (file.read() == '\n') totalLines++;
  }

  // Calculate how many to skip
  int toSkip = max(0, totalLines - maxToLoad);

  // Reset and skip header again
  file.seek(0);
  file.readStringUntil('\n');

  // Skip lines we don't need
  for (int i = 0; i < toSkip && file.available(); i++) {
    file.readStringUntil('\n');
  }

  // Read remaining lines into buffer
  historyCount = 0;
  historyIndex = 0;

  while (file.available() && historyCount < MAX_HISTORY_READINGS) {
    String line = file.readStringUntil('\n');
    if (line.length() < 10) continue;  // Skip empty or invalid lines

    // Parse CSV: timestamp,temperature,humidity,pressure,rainfall,feelslike
    int idx = 0;
    int lastComma = -1;
    String fields[6];

    for (int i = 0; i <= line.length(); i++) {
      if (i == line.length() || line[i] == ',') {
        fields[idx++] = line.substring(lastComma + 1, i);
        lastComma = i;
        if (idx >= 6) break;
      }
    }

    if (idx >= 6) {
      WeatherReading reading;
      reading.timestamp = fields[0].toInt();
      reading.temperature = (int16_t)(fields[1].toFloat() * 10);
      reading.humidity = (uint16_t)fields[2].toInt();
      reading.pressure = (uint16_t)fields[3].toInt();
      reading.rainfall = (uint16_t)(fields[4].toFloat() * 100);
      reading.feelslike = (int16_t)(fields[5].toFloat() * 10);

      historyBuffer[historyCount++] = reading;
    }
  }

  historyIndex = historyCount % MAX_HISTORY_READINGS;
  file.close();
  return historyCount;
}

// Trim SD history file if it exceeds max readings
void trimSDHistory() {
  if (!sdCardAvailable) return;

  int currentCount = countSDHistoryReadings();
  if (currentCount <= MAX_SD_READINGS) return;

  Serial.println("SD: Trimming old readings...");

  // Read file, keeping only last MAX_SD_READINGS
  File readFile = SD.open(SD_HISTORY_FILE, FILE_READ);
  if (!readFile) return;

  // Create temp file
  File writeFile = SD.open("/weather_temp.csv", FILE_WRITE);
  if (!writeFile) {
    readFile.close();
    return;
  }

  // Copy header
  String header = readFile.readStringUntil('\n');
  writeFile.println(header);

  // Skip old entries
  int toSkip = currentCount - MAX_SD_READINGS;
  for (int i = 0; i < toSkip && readFile.available(); i++) {
    readFile.readStringUntil('\n');
  }

  // Copy remaining entries
  while (readFile.available()) {
    String line = readFile.readStringUntil('\n');
    if (line.length() > 0) {
      writeFile.println(line);
    }
  }

  readFile.close();
  writeFile.close();

  // Replace original with temp
  SD.remove(SD_HISTORY_FILE);
  SD.rename("/weather_temp.csv", SD_HISTORY_FILE);
}

// Get total readings available (SD + FFat fallback info)
int getTotalHistoryReadings() {
  if (sdCardAvailable && SD.exists(SD_HISTORY_FILE)) {
    return countSDHistoryReadings();
  }
  return historyCount;
}

// Get history storage info string
String getHistoryStorageInfo() {
  if (sdCardAvailable) {
    int sdReadings = countSDHistoryReadings();
    float days = sdReadings * 10.0 / 60.0 / 24.0;  // Assuming 10 min intervals
    return String("SD: ") + String(sdReadings) + " (" + String(days, 1) + " dias)";
  } else {
    float days = historyCount * 10.0 / 60.0 / 24.0;
    return String("FFat: ") + String(historyCount) + " (" + String(days, 1) + " dias)";
  }
}

#endif // WEATHER_HISTORY_H
