#ifndef WEATHER_HISTORY_H
#define WEATHER_HISTORY_H

#include <FFat.h>
#include <esp_heap_caps.h>

// Maximum readings to store (at 10 min intervals = ~7 days)
#define MAX_HISTORY_READINGS 1000
#define HISTORY_FILE "/weather_history.dat"

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

// Initialize FFat and load history
bool initWeatherHistory() {
  // Allocate buffer in PSRAM
  if (historyBuffer == nullptr) {
    historyBuffer = (WeatherReading*)heap_caps_calloc(MAX_HISTORY_READINGS, sizeof(WeatherReading), MALLOC_CAP_SPIRAM);
    if (historyBuffer == nullptr) {
      Serial.println("ERROR: Failed to allocate history buffer!");
      return false;
    }
  }

  if (!FFat.begin(true)) {
    Serial.println("FFat mount failed!");
    return false;
  }

  // Load existing history
  if (FFat.exists(HISTORY_FILE)) {
    File file = FFat.open(HISTORY_FILE, "r");
    if (file) {
      file.read((uint8_t*)&historyCount, sizeof(historyCount));
      file.read((uint8_t*)&historyIndex, sizeof(historyIndex));

      // Validate count
      if (historyCount < 0 || historyCount > MAX_HISTORY_READINGS) {
        historyCount = 0;
        historyIndex = 0;
      } else if (historyCount > 0) {
        int toRead = min(historyCount, MAX_HISTORY_READINGS);
        file.read((uint8_t*)historyBuffer, toRead * sizeof(WeatherReading));
      }
      file.close();
    }
  } else {
    historyCount = 0;
    historyIndex = 0;
  }

  historyInitialized = true;
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

  historyBuffer[historyIndex] = reading;
  historyIndex = (historyIndex + 1) % MAX_HISTORY_READINGS;

  if (historyCount < MAX_HISTORY_READINGS) {
    historyCount++;
  }

  saveWeatherHistory();
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

// Clear all history
void clearWeatherHistory() {
  historyCount = 0;
  historyIndex = 0;
  if (FFat.exists(HISTORY_FILE)) {
    FFat.remove(HISTORY_FILE);
  }
}

#endif // WEATHER_HISTORY_H
