// Quote of the Day Screen
// Fetches random inspirational quotes in Spanish
// Hidden feature: QR -> World Clock -> Quote

#ifndef QUOTE_SCREEN_H
#define QUOTE_SCREEN_H

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Quote data structure - stored in RTC memory to survive deep sleep
struct QuoteData {
  char text[500];
  char author[100];
  bool valid;
};

RTC_DATA_ATTR QuoteData currentQuote = {"", "", false};

// External declarations
extern void setFont(GFXfont const &font);
extern void drawString(int x, int y, String text, alignment align);
extern void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color);
extern void drawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color);
extern void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
extern uint8_t *framebuffer;

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

// Fetch random quote - tries Spanish API first, then English fallback
bool fetchQuote() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Quote: WiFi not connected");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);

  // Try Spanish quotes API first
  Serial.println("Quote: Trying Spanish API...");
  http.begin(client, "https://frasedeldia.azurewebsites.net/api/phrase");

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    http.end();
    Serial.printf("Quote ES: Received %d bytes\n", payload.length());
    Serial.println(payload);

    // Parse: {"phrase":"...","author":"..."}
    StaticJsonDocument<512> doc;
    if (!deserializeJson(doc, payload)) {
      const char* content = doc["phrase"];
      const char* author = doc["author"];
      if (content) {
        strncpy(currentQuote.text, content, sizeof(currentQuote.text) - 1);
        if (author && strlen(author) > 0) {
          strncpy(currentQuote.author, author, sizeof(currentQuote.author) - 1);
        } else {
          strcpy(currentQuote.author, "Anonimo");
        }
        currentQuote.valid = true;
        Serial.printf("Quote: \"%s\" - %s\n", currentQuote.text, currentQuote.author);
        return true;
      }
    }
  }
  http.end();

  // Fallback to English API
  Serial.println("Quote: Trying English fallback...");
  http.begin(client, "https://api.quotable.io/random?maxLength=180");

  httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Quote: HTTP error %d\n", httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  Serial.printf("Quote EN: Received %d bytes\n", payload.length());

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, payload)) {
    Serial.println("Quote: JSON error");
    return false;
  }

  const char* content = doc["content"];
  const char* author = doc["author"];

  if (content && author) {
    strncpy(currentQuote.text, content, sizeof(currentQuote.text) - 1);
    strncpy(currentQuote.author, author, sizeof(currentQuote.author) - 1);
    currentQuote.valid = true;
    Serial.printf("Quote: \"%s\" - %s\n", currentQuote.text, currentQuote.author);
    return true;
  }

  return false;
}

// Draw thick decorative frame
void drawDecorativeFrame(int x, int y, int w, int h) {
  // Outer thick border (4 pixels)
  for (int i = 0; i < 4; i++) {
    drawRect(x + i, y + i, w - i*2, h - i*2, Black);
  }

  // Inner border with gap
  int inset = 12;
  for (int i = 0; i < 2; i++) {
    drawRect(x + inset + i, y + inset + i, w - (inset + i)*2, h - (inset + i)*2, DarkGrey);
  }

  // Corner decorations (small squares)
  int cornerSize = 8;
  int cornerInset = 6;
  // Top-left
  fillRect(x + cornerInset, y + cornerInset, cornerSize, cornerSize, Black);
  // Top-right
  fillRect(x + w - cornerInset - cornerSize, y + cornerInset, cornerSize, cornerSize, Black);
  // Bottom-left
  fillRect(x + cornerInset, y + h - cornerInset - cornerSize, cornerSize, cornerSize, Black);
  // Bottom-right
  fillRect(x + w - cornerInset - cornerSize, y + h - cornerInset - cornerSize, cornerSize, cornerSize, Black);
}

// Draw elegant opening quotation mark (large double quotes)
void drawOpenQuote(int x, int y) {
  // Simple elegant double quote marks «
  int h = 35;  // Height
  int w = 8;   // Width of each stroke
  int gap = 14; // Gap between the two marks

  // First quote mark - thick vertical line
  fillRect(x, y, w, h, DarkGrey);
  // Second quote mark
  fillRect(x + gap + w, y, w, h, DarkGrey);
}

// Draw elegant closing quotation mark
void drawCloseQuote(int x, int y) {
  // Simple elegant double quote marks »
  int h = 35;
  int w = 8;
  int gap = 14;

  // First quote mark
  fillRect(x, y, w, h, DarkGrey);
  // Second quote mark
  fillRect(x + gap + w, y, w, h, DarkGrey);
}

// Word wrap helper - returns number of lines used
int drawQuoteText(int centerX, int startY, int maxWidth, int lineHeight, const char* text, int maxLines) {
  String str = text;
  int lines = 0;
  int currentY = startY;

  setFont(OpenSans16B);   // Bigger font
  int charWidth = 18;     // More margin to avoid overlap with frame/quotes

  int charsPerLine = maxWidth / charWidth;
  if (charsPerLine < 10) charsPerLine = 10;

  Serial.printf("Quote wrap: text len=%d, maxWidth=%d, charsPerLine=%d\n", str.length(), maxWidth, charsPerLine);

  while (str.length() > 0 && lines < maxLines) {
    Serial.printf("  Line %d: remaining=%d chars\n", lines, str.length());

    if ((int)str.length() <= charsPerLine) {
      // Last line - draw centered
      Serial.printf("  -> Final line: %s\n", str.c_str());
      drawString(centerX, currentY, str, CENTER);
      lines++;
      break;
    }

    // Find last space within limit
    int breakPoint = charsPerLine;
    while (breakPoint > 0 && str.charAt(breakPoint) != ' ') {
      breakPoint--;
    }
    if (breakPoint == 0) breakPoint = charsPerLine;  // No space found, force break

    String line = str.substring(0, breakPoint);
    Serial.printf("  -> Draw: '%s' (break at %d)\n", line.c_str(), breakPoint);
    drawString(centerX, currentY, line, CENTER);

    str = str.substring(breakPoint + 1);  // +1 to skip the space
    currentY += lineHeight;
    lines++;
  }

  Serial.printf("Quote wrap: drew %d lines\n", lines);
  return lines;
}

// Main display function
void DisplayQuoteScreen() {
  Serial.println("Quote: Displaying quote screen");

  // Auto-fetch if no valid quote
  if (!currentQuote.valid) {
    Serial.println("Quote: No valid quote, fetching...");
    fetchQuote();
  }

  // Frame dimensions - larger, using more screen space
  int frameX = 35;
  int frameY = 35;
  int frameW = SCREEN_WIDTH - 70;
  int frameH = SCREEN_HEIGHT - 70;

  // Draw decorative frame
  drawDecorativeFrame(frameX, frameY, frameW, frameH);

  // Title - moved up
  setFont(OpenSans14B);
  drawString(SCREEN_WIDTH / 2, frameY + 25, "~ Pensamiento del Dia ~", CENTER);

  // Decorative line under title (separate from title position)
  int lineY = frameY + 65;
  for (int i = 0; i < 3; i++) {
    drawFastHLine(frameX + 120, lineY + i, frameW - 240, DarkGrey);
  }

  if (!currentQuote.valid) {
    setFont(OpenSans16B);
    drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, "Cargando sabiduria...", CENTER);
    return;
  }

  // Opening quote mark
  drawOpenQuote(frameX + 55, frameY + 80);

  // Quote text - centered, wrapped
  int textStartY = frameY + 120;
  int textMaxWidth = frameW - 200;  // More margin for safety
  int linesUsed = drawQuoteText(SCREEN_WIDTH / 2, textStartY, textMaxWidth, 45, currentQuote.text, 6);

  // Closing quote mark
  int closeQuoteY = textStartY + (linesUsed * 45) - 15;
  if (closeQuoteY < textStartY + 100) closeQuoteY = textStartY + 100;
  drawCloseQuote(frameX + frameW - 100, closeQuoteY);

  // Author line with decorative dash
  int authorY = closeQuoteY + 80;
  if (authorY > frameY + frameH - 90) authorY = frameY + frameH - 90;

  setFont(OpenSans14B);
  String authorLine = "- " + String(currentQuote.author) + " -";
  drawString(SCREEN_WIDTH / 2, authorY, authorLine, CENTER);

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
