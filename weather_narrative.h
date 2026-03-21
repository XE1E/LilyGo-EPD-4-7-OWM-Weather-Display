// Weather Narrative Screen
// Generates natural language weather description using Gemini AI
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
extern String gemini_apikey;
extern String City;

// Weather data structures (from main sketch)
extern Forecast_record_type WxConditions[];
extern Forecast_record_type WxForecast[];
extern String Units;

// Config for narrative style (defined in wifi_manager.h)
extern ConfigData config;

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

// Narrative style prompts
const char* getNarrativeStylePrompt(int style) {
  switch(style) {
    case 1:  // Formal
      return "Genera un reporte meteorologico formal en espanol de 2-3 oraciones, "
             "como lo diria un presentador de noticias profesional. ";
    case 2:  // Poético
      return "Genera una descripcion poetica del clima en espanol de 2-3 oraciones, "
             "usando metaforas y lenguaje literario evocador. ";
    case 3:  // Técnico
      return "Genera un informe meteorologico tecnico en espanol de 2-3 oraciones, "
             "preciso y con terminologia cientifica. ";
    case 4:  // Humorístico
      return "Genera una descripcion del clima en espanol de 2-3 oraciones, "
             "con humor y comentarios graciosos sobre el tiempo. ";
    case 5:  // Abuelita
      return "Genera una descripcion del clima en espanol de 2-3 oraciones, "
             "como lo diria una abuelita mexicana carinosa, con consejos practicos. ";
    default: // Casual (0)
      return "Genera una descripcion del clima en espanol de 2-3 oraciones, "
             "natural y amigable, como platica entre amigos. ";
  }
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

  // Add forecast info
  if (WxForecast[0].Temperature > -100) {
    prompt += "Pronostico proximas horas: ";
    prompt += String(WxForecast[0].High, 0) + "/" + String(WxForecast[0].Low, 0);
    prompt += (Units == "M") ? " C, " : " F, ";
    prompt += String(WxForecast[0].Forecast0) + "\n";
  }

  prompt += "\nResponde SOLO con el texto de la descripcion, sin introduccion ni explicacion.";
  return prompt;
}

// Fetch weather narrative from Gemini API
bool fetchWeatherNarrative() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Narrative: WiFi not connected");
    return false;
  }

  if (gemini_apikey.length() < 10) {
    Serial.println("Narrative: No Gemini API key configured");
    return false;
  }

  Serial.println("Narrative: Generating with Gemini...");

  String prompt = buildWeatherPrompt();
  Serial.println("Prompt: " + prompt);

  // Build Gemini API request
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(15000);

  String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + gemini_apikey;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  // Build JSON request body
  StaticJsonDocument<1024> requestDoc;
  JsonArray contents = requestDoc.createNestedArray("contents");
  JsonObject content = contents.createNestedObject();
  JsonArray parts = content.createNestedArray("parts");
  JsonObject part = parts.createNestedObject();
  part["text"] = prompt;

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
             "Error API Gemini (codigo %d). Intenta de nuevo en 1 minuto.", httpCode);
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

  // Extract text from Gemini response
  // Format: {"candidates":[{"content":{"parts":[{"text":"..."}]}}]}
  const char* text = doc["candidates"][0]["content"]["parts"][0]["text"];

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

  setFont(OpenSans14B);
  int charWidth = 12;

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
    Serial.println("Narrative: Fetching from Gemini...");
    fetchWeatherNarrative();
  }

  // Frame dimensions
  int frameX = 35;
  int frameY = 35;
  int frameW = SCREEN_WIDTH - 70;
  int frameH = SCREEN_HEIGHT - 70;

  // Draw decorative frame
  drawNarrativeFrame(frameX, frameY, frameW, frameH);

  // Title with weather icon indication
  setFont(OpenSans14B);
  drawString(SCREEN_WIDTH / 2, frameY + 25, "~ El Clima de Hoy ~", CENTER);

  // Decorative line under title
  int lineY = frameY + 50;
  for (int i = 0; i < 3; i++) {
    drawFastHLine(frameX + 120, lineY + i, frameW - 240, DarkGrey);
  }

  // City name
  setFont(OpenSans12B);
  drawString(SCREEN_WIDTH / 2, frameY + 75, City, CENTER);

  if (!weatherNarrative.valid) {
    setFont(OpenSans16B);
    drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, "Generando descripcion...", CENTER);
    setFont(OpenSans10B);
    drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 40, "Por favor espera", CENTER);
    return;
  }

  // Narrative text
  int textStartY = frameY + 120;
  int textMaxWidth = frameW - 100;
  drawNarrativeText(SCREEN_WIDTH / 2, textStartY, textMaxWidth, 38, weatherNarrative.text, 8);

  // Bottom decorative line
  int bottomLineY = frameY + frameH - 55;
  for (int i = 0; i < 3; i++) {
    drawFastHLine(frameX + 120, bottomLineY + i, frameW - 240, DarkGrey);
  }

  // Footer hint
  setFont(OpenSans10B);
  drawString(SCREEN_WIDTH / 2, frameY + frameH, "Toca para volver", CENTER);
}

#endif
