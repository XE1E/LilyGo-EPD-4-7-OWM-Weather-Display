// Weather Narrative Screen
// Generates natural language weather description using Groq/Llama AI
// Access: Main screen -> touch weather icon area

#ifndef WEATHER_NARRATIVE_H
#define WEATHER_NARRATIVE_H

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Narrative data - stored in RTC memory to survive deep sleep
struct NarrativeData {
  char text[800];
  bool valid;
  unsigned long timestamp;
};

RTC_DATA_ATTR NarrativeData weatherNarrative = {"", false, 0};

// External declarations
extern void setFont(GFXfont const &font);
extern void drawString(int x, int y, String text, alignment align);
extern void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color);
extern void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern uint8_t *framebuffer;
extern String groq_apikey;
extern String City;

// Weather data structures (from main sketch)
extern Forecast_record_type WxConditions[];
extern Forecast_record_type WxForecast[];
extern String Units;

// Config for narrative style (defined in wifi_manager.h)
extern ConfigData config;

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

// Language setting (from lang.h)
extern int currentLang;

// Get language name for prompt
const char* getLanguageName() {
  switch(currentLang) {
    case 1: return "English";
    case 2: return "French";
    default: return "Spanish";
  }
}

// Narrative style prompts - multilingual
String getNarrativeStylePrompt(int style) {
  String lang = getLanguageName();
  String prompt;

  switch(style) {
    case 1:  // Formal
      prompt = "Generate a formal weather report in " + lang + " of 4-5 sentences, "
               "as a professional news presenter would say it. ";
      break;
    case 2:  // Poetic
      prompt = "Generate a poetic weather description in " + lang + " of 4-5 sentences, "
               "using metaphors and evocative literary language. ";
      break;
    case 3:  // Technical
      prompt = "Generate a technical weather report in " + lang + " of 4-5 sentences, "
               "precise and with scientific terminology. ";
      break;
    case 4:  // Humorous
      prompt = "Generate a weather description in " + lang + " of 4-5 sentences, "
               "with humor and funny comments about the weather. ";
      break;
    case 5:  // Grandma
      prompt = "Generate a weather description in " + lang + " of 4-5 sentences, "
               "as a caring grandmother would say it, with practical advice. ";
      break;
    default: // Radio (0)
      prompt = "Generate a weather report in " + lang + " of 4-5 sentences, "
               "radio bulletin style, concise and professional. ";
      break;
  }
  return prompt;
}

// Build weather data prompt for AI
String buildWeatherPrompt() {
  String prompt = getNarrativeStylePrompt(config.narrative_style);
  prompt += "Datos:\n";
  prompt += "Ciudad: " + City + "\n";
  prompt += "Temperatura: " + String(WxConditions[0].Temperature, 1);
  prompt += (Units == "M") ? " C\n" : " F\n";
  prompt += "Sensacion: " + String(WxConditions[0].Feelslike, 1);
  prompt += (Units == "M") ? " C\n" : " F\n";
  prompt += "Humedad: " + String(WxConditions[0].Humidity) + "%\n";
  prompt += "Condicion: " + String(WxConditions[0].Forecast0) + "\n";
  prompt += "Viento: " + String(WxConditions[0].Windspeed, 1);
  prompt += (Units == "M") ? " km/h\n" : " mph\n";
  prompt += "Presion: " + String(WxConditions[0].Pressure, 0) + " hPa\n";

  // Add forecast info for next days
  prompt += "Pronostico:\n";
  for (int i = 0; i < 3; i++) {
    int idx = i * 8;  // Each day has 8 readings (3-hour intervals)
    if (WxForecast[idx].Temperature > -100) {
      prompt += "Dia " + String(i + 1) + ": ";
      prompt += String(WxForecast[idx].High, 0) + "/" + String(WxForecast[idx].Low, 0);
      prompt += (Units == "M") ? " C, " : " F, ";
      prompt += String(WxForecast[idx].Forecast0) + "\n";
    }
  }

  prompt += "\nRespond ONLY with the description text, no introduction or explanation. Do not use phrases like 'weather bulletin' or 'weather report'.";
  return prompt;
}

// Fetch weather narrative from Groq API
bool fetchWeatherNarrative() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Narrative: WiFi not connected");
    return false;
  }

  if (groq_apikey.length() < 10) {
    Serial.println("Narrative: No Groq API key configured");
    return false;
  }

  Serial.println("Narrative: Generating with Groq...");

  String prompt = buildWeatherPrompt();
  Serial.println("Prompt: " + prompt);

  // Build Groq API request (OpenAI-compatible format)
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(15000);

  String url = "https://api.groq.com/openai/v1/chat/completions";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + groq_apikey);

  // Build JSON request body (OpenAI format)
  StaticJsonDocument<1024> requestDoc;
  requestDoc["model"] = "llama-3.1-8b-instant";
  JsonArray messages = requestDoc.createNestedArray("messages");
  JsonObject message = messages.createNestedObject();
  message["role"] = "user";
  message["content"] = prompt;

  String requestBody;
  serializeJson(requestDoc, requestBody);
  Serial.println("Request: " + requestBody);

  int httpCode = http.POST(requestBody);

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Narrative: HTTP error %d\n", httpCode);
    String response = http.getString();
    Serial.println("Response: " + response);
    http.end();
    // Store error message for display
    snprintf(weatherNarrative.text, sizeof(weatherNarrative.text),
             "Error API Groq (codigo %d). Intenta de nuevo en 1 minuto.", httpCode);
    weatherNarrative.valid = true;  // Show error message
    return false;
  }

  String payload = http.getString();
  http.end();

  Serial.printf("Narrative: Received %d bytes\n", payload.length());
  Serial.println(payload);

  // Parse response
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.printf("Narrative: JSON error: %s\n", error.c_str());
    snprintf(weatherNarrative.text, sizeof(weatherNarrative.text),
             "Error procesando respuesta. Intenta de nuevo.");
    weatherNarrative.valid = true;
    return false;
  }

  // Extract text from Groq response (OpenAI format)
  // Format: {"choices":[{"message":{"content":"..."}}]}
  const char* text = doc["choices"][0]["message"]["content"];

  if (text && strlen(text) > 0) {
    strncpy(weatherNarrative.text, text, sizeof(weatherNarrative.text) - 1);
    weatherNarrative.valid = true;
    weatherNarrative.timestamp = millis();
    Serial.printf("Narrative: \"%s\"\n", weatherNarrative.text);
    return true;
  }

  Serial.println("Narrative: No text in response");
  snprintf(weatherNarrative.text, sizeof(weatherNarrative.text),
           "No se recibio texto de la IA. Intenta de nuevo.");
  weatherNarrative.valid = true;
  return false;
}

// Draw decorative frame (similar to quote screen)
void drawNarrativeFrame(int x, int y, int w, int h) {
  // Outer thick border
  for (int i = 0; i < 4; i++) {
    drawRect(x + i, y + i, w - i*2, h - i*2, Black);
  }

  // Inner border with gap
  int inset = 12;
  for (int i = 0; i < 2; i++) {
    drawRect(x + inset + i, y + inset + i, w - (inset + i)*2, h - (inset + i)*2, DarkGrey);
  }

  // Corner squares
  int cornerSize = 8;
  int cornerInset = 6;
  fillRect(x + cornerInset, y + cornerInset, cornerSize, cornerSize, Black);
  fillRect(x + w - cornerInset - cornerSize, y + cornerInset, cornerSize, cornerSize, Black);
  fillRect(x + cornerInset, y + h - cornerInset - cornerSize, cornerSize, cornerSize, Black);
  fillRect(x + w - cornerInset - cornerSize, y + h - cornerInset - cornerSize, cornerSize, cornerSize, Black);
}

// Word wrap for narrative text
int drawNarrativeText(int centerX, int startY, int maxWidth, int lineHeight, const char* text, int maxLines) {
  String str = text;
  int lines = 0;
  int currentY = startY;

  setFont(OpenSans10B);
  int charWidth = 9;  // Smaller font for more text

  int charsPerLine = maxWidth / charWidth;
  if (charsPerLine < 10) charsPerLine = 10;

  while (str.length() > 0 && lines < maxLines) {
    if ((int)str.length() <= charsPerLine) {
      drawString(centerX, currentY, str, CENTER);
      lines++;
      break;
    }

    int breakPoint = charsPerLine;
    while (breakPoint > 0 && str.charAt(breakPoint) != ' ') {
      breakPoint--;
    }
    if (breakPoint == 0) breakPoint = charsPerLine;

    String line = str.substring(0, breakPoint);
    drawString(centerX, currentY, line, CENTER);

    str = str.substring(breakPoint + 1);
    currentY += lineHeight;
    lines++;
  }

  return lines;
}

// Main display function
void DisplayWeatherNarrativeScreen() {
  Serial.println("Narrative: Displaying weather narrative screen");

  // Auto-fetch if no valid narrative or if previous was an error
  bool isError = strncmp(weatherNarrative.text, "Error", 5) == 0 ||
                 strncmp(weatherNarrative.text, "No se recibio", 13) == 0;
  if (!weatherNarrative.valid || isError) {
    Serial.println("Narrative: Fetching from Groq...");
    fetchWeatherNarrative();
  }

  // Frame dimensions
  int frameX = 35;
  int frameY = 35;
  int frameW = SCREEN_WIDTH - 70;
  int frameH = SCREEN_HEIGHT - 70;

  // Draw decorative frame
  drawNarrativeFrame(frameX, frameY, frameW, frameH);

  // Title - multilingual
  const char* titles[] = {"~ El Clima de Hoy ~", "~ Today's Weather ~", "~ Meteo du Jour ~"};
  const char* loading[] = {"Generando descripcion...", "Generating description...", "Generation en cours..."};
  const char* wait[] = {"Por favor espera", "Please wait", "Veuillez patienter"};

  setFont(OpenSans14B);
  drawString(SCREEN_WIDTH / 2, frameY + 25, titles[currentLang], CENTER);

  // Decorative line under title
  int lineY = frameY + 70;
  for (int i = 0; i < 3; i++) {
    drawFastHLine(frameX + 120, lineY + i, frameW - 240, DarkGrey);
  }

  if (!weatherNarrative.valid) {
    setFont(OpenSans16B);
    drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, loading[currentLang], CENTER);
    setFont(OpenSans10B);
    drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 40, wait[currentLang], CENTER);
    return;
  }

  // Narrative text
  int textStartY = frameY + 100;
  int textMaxWidth = frameW - 214;  // ~69 chars per line
  drawNarrativeText(SCREEN_WIDTH / 2, textStartY, textMaxWidth, 36, weatherNarrative.text, 9);

  // Footer hint - multilingual
  const char* footer[] = {"Toca para volver", "Touch to return", "Touchez pour revenir"};
  setFont(OpenSans10B);
  drawString(SCREEN_WIDTH / 2, frameY + frameH, footer[currentLang], CENTER);
}

#endif
