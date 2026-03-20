// Radio Reference Screens for Amateur Radio
// Phonetic alphabet, Q codes, Morse code, DXCC prefixes, propagation, contests
// Style: Similar to Info screens with consistent layout
//
// MODULAR: Este archivo es independiente y puede usarse en otros proyectos
// Solo requiere las funciones de dibujo del EPD47 (setFont, drawString, etc.)

#ifndef RADIO_SCREENS_H
#define RADIO_SCREENS_H

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ============== PROPAGATION DATA STRUCTURE ==============

struct PropagationData {
  int solarFlux;
  int aIndex;
  int kIndex;
  int sunspots;
  String xray;
  String geomagField;
  // Band conditions (day/night)
  String band80_40_day, band80_40_night;
  String band30_20_day, band30_20_night;
  String band17_15_day, band17_15_night;
  String band12_10_day, band12_10_night;
  // VHF
  String vhfAurora;
  String vhfEskip;
  String updated;
  bool valid;
};

PropagationData propData = {0, 0, 0, 0, "", "", "", "", "", "", "", "", "", "", "", "", "", false};

// Helper to extract value between XML tags
String extractXMLValue(const String& xml, const String& tag) {
  String startTag = "<" + tag + ">";
  String endTag = "</" + tag + ">";
  int start = xml.indexOf(startTag);
  if (start == -1) return "";
  start += startTag.length();
  int end = xml.indexOf(endTag, start);
  if (end == -1) return "";
  return xml.substring(start, end);
}

// Extract band condition with time attribute
// Format: <band name="80m-40m" time="day">Good</band>
String extractBandCondition(const String& xml, const String& bandName, const String& timeOfDay) {
  String searchStr = "name=\"" + bandName + "\" time=\"" + timeOfDay + "\">";
  int start = xml.indexOf(searchStr);
  if (start == -1) return "N/A";
  start += searchStr.length();
  int end = xml.indexOf("</band>", start);
  if (end == -1) return "N/A";
  return xml.substring(start, end);
}

// Fetch propagation data from HamQSL
bool fetchPropagationData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Propagation: WiFi not connected");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, "https://www.hamqsl.com/solarxml.php");
  http.setTimeout(10000);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Propagation HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  String xml = http.getString();
  http.end();

  // Parse solar indices
  propData.solarFlux = extractXMLValue(xml, "solarflux").toInt();
  propData.aIndex = extractXMLValue(xml, "aindex").toInt();
  propData.kIndex = extractXMLValue(xml, "kindex").toInt();
  propData.sunspots = extractXMLValue(xml, "sunspots").toInt();
  propData.xray = extractXMLValue(xml, "xray");
  propData.geomagField = extractXMLValue(xml, "geomagfield");
  propData.updated = extractXMLValue(xml, "updated");

  // Band conditions
  propData.band80_40_day = extractBandCondition(xml, "80m-40m", "day");
  propData.band80_40_night = extractBandCondition(xml, "80m-40m", "night");
  propData.band30_20_day = extractBandCondition(xml, "30m-20m", "day");
  propData.band30_20_night = extractBandCondition(xml, "30m-20m", "night");
  propData.band17_15_day = extractBandCondition(xml, "17m-15m", "day");
  propData.band17_15_night = extractBandCondition(xml, "17m-15m", "night");
  propData.band12_10_day = extractBandCondition(xml, "12m-10m", "day");
  propData.band12_10_night = extractBandCondition(xml, "12m-10m", "night");

  // VHF conditions
  propData.vhfAurora = extractBandCondition(xml, "vhf-aurora", "");
  if (propData.vhfAurora == "N/A") {
    propData.vhfAurora = extractXMLValue(xml, "aurora");
  }
  propData.vhfEskip = extractXMLValue(xml, "e-skip");

  propData.valid = (propData.solarFlux > 0);
  Serial.printf("Propagation: SFI=%d A=%d K=%d\n", propData.solarFlux, propData.aIndex, propData.kIndex);
  return propData.valid;
}

// External dependencies
extern void setFont(GFXfont const &font);
extern void drawString(int x, int y, String text, alignment align);
extern void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color);
extern void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern void fillCircle(int x, int y, int r, uint8_t color);

// Draw rounded rectangle border
void drawRoundedRect(int x, int y, int w, int h, int r, int thickness, uint8_t color) {
  // Draw horizontal lines (top and bottom)
  for (int t = 0; t < thickness; t++) {
    drawFastHLine(x + r, y + t, w - 2*r, color);           // Top
    drawFastHLine(x + r, y + h - 1 - t, w - 2*r, color);   // Bottom
  }
  // Draw vertical lines (left and right)
  for (int t = 0; t < thickness; t++) {
    for (int i = r; i < h - r; i++) {
      fillRect(x + t, y + i, 1, 1, color);                 // Left
      fillRect(x + w - 1 - t, y + i, 1, 1, color);         // Right
    }
  }
  // Draw corners using circles
  for (int t = 0; t < thickness; t++) {
    int cr = r - t;
    if (cr > 0) {
      // Top-left
      for (int i = 0; i <= cr; i++) {
        for (int j = 0; j <= cr; j++) {
          if (i*i + j*j <= cr*cr && i*i + j*j >= (cr-1)*(cr-1)) {
            fillRect(x + r - i, y + r - j, 1, 1, color);
          }
        }
      }
      // Top-right
      for (int i = 0; i <= cr; i++) {
        for (int j = 0; j <= cr; j++) {
          if (i*i + j*j <= cr*cr && i*i + j*j >= (cr-1)*(cr-1)) {
            fillRect(x + w - r - 1 + i, y + r - j, 1, 1, color);
          }
        }
      }
      // Bottom-left
      for (int i = 0; i <= cr; i++) {
        for (int j = 0; j <= cr; j++) {
          if (i*i + j*j <= cr*cr && i*i + j*j >= (cr-1)*(cr-1)) {
            fillRect(x + r - i, y + h - r - 1 + j, 1, 1, color);
          }
        }
      }
      // Bottom-right
      for (int i = 0; i <= cr; i++) {
        for (int j = 0; j <= cr; j++) {
          if (i*i + j*j <= cr*cr && i*i + j*j >= (cr-1)*(cr-1)) {
            fillRect(x + w - r - 1 + i, y + h - r - 1 + j, 1, 1, color);
          }
        }
      }
    }
  }
}

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

// ============== RADIO MENU ==============

void DisplayRadioMenuScreen() {
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "RADIOAFICIONADO", CENTER);
  drawFastHLine(100, 50, SCREEN_WIDTH - 200, Black);

  // Grid 2x3 buttons
  const char* options1[] = {"CODIGO", "CODIGO Q", "CLAVE MORSE", "DXCC", "PROPAGACION", "CONCURSOS"};
  const char* options2[] = {"FONETICO", "", "", "", "", ""};

  const int btnW = 280, btnH = 130, gapX = 40, gapY = 20;
  int startX = (SCREEN_WIDTH - (btnW * 3 + gapX * 2)) / 2;

  setFont(OpenSans16B);
  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < 3; col++) {
      int idx = row * 3 + col;
      int x = startX + col * (btnW + gapX);
      int y = 70 + row * (btnH + gapY);

      drawRoundedRect(x, y, btnW, btnH, 12, 6, Black);

      if (strlen(options2[idx]) > 0) {
        drawString(x + btnW/2, y + btnH/2 - 28, options1[idx], CENTER);
        drawString(x + btnW/2, y + btnH/2 + 8, options2[idx], CENTER);
      } else {
        drawString(x + btnW/2, y + btnH/2 - 12, options1[idx], CENTER);
      }
    }
  }

  setFont(OpenSans12B);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 45, "Toca una opcion para ver detalles", CENTER);
}

// ============== PHONETIC ALPHABET ==============

void DisplayRadioPhoneticScreen() {
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "CODIGO FONETICO", CENTER);
  drawFastHLine(100, 50, SCREEN_WIDTH - 200, Black);
  setFont(OpenSans12B);

  const char* phonetic[] = {
    "A - Alfa", "B - Bravo", "C - Charlie", "D - Delta",
    "E - Echo", "F - Foxtrot", "G - Golf", "H - Hotel",
    "I - India", "J - Juliet", "K - Kilo", "L - Lima",
    "M - Mike", "N - November", "O - Oscar", "P - Papa",
    "Q - Quebec", "R - Romeo", "S - Sierra", "T - Tango",
    "U - Uniform", "V - Victor", "W - Whiskey", "X - X-ray",
    "Y - Yankee", "Z - Zulu"
  };

  const int col1 = 80, col2 = 380, col3 = 680;
  const int startY = 75, lineH = 38;

  for (int i = 0; i < 9; i++) {
    drawString(col1, startY + i * lineH, phonetic[i], LEFT);
    drawString(col2, startY + i * lineH, phonetic[i + 9], LEFT);
    if (i + 18 < 26) {
      drawString(col3, startY + i * lineH, phonetic[i + 18], LEFT);
    }
  }

  // Numbers
  int numY = startY + 9 * lineH + 20;
  drawFastHLine(50, numY - 10, SCREEN_WIDTH - 100, Grey);
  setFont(OpenSans10B);
  drawString(col1, numY, "0-Zero  1-One  2-Two  3-Three  4-Four", LEFT);
  drawString(col1, numY + 25, "5-Five  6-Six  7-Seven  8-Eight  9-Niner", LEFT);

  setFont(OpenSans8B);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, "Toca para regresar al menu", CENTER);
}

// ============== Q CODES ==============

void DisplayRadioQCodesScreen() {
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "CODIGOS Q", CENTER);
  drawFastHLine(100, 60, SCREEN_WIDTH - 200, Black);
  setFont(OpenSans10B);

  const char* qcodes[][2] = {
    {"QRA", "Nombre de estacion"},
    {"QRG", "Frecuencia exacta"},
    {"QRH", "Frecuencia varia"},
    {"QRI", "Tono de emision (1-5)"},
    {"QRK", "Inteligibilidad (1-5)"},
    {"QRL", "Frecuencia ocupada?"},
    {"QRM", "Interferencia artificial"},
    {"QRN", "Interferencia atmosferica"},
    {"QRO", "Aumentar potencia"},
    {"QRP", "Reducir potencia"},
    {"QRQ", "Transmitir mas rapido"},
    {"QRS", "Transmitir mas lento"},
    {"QRT", "Dejar de transmitir"},
    {"QRU", "No tengo nada para ti"},
    {"QRV", "Estoy listo"},
    {"QRX", "Espere, llamare de nuevo"},
    {"QRZ", "Quien me llama?"},
    {"QSA", "Intensidad (1-5)"},
    {"QSB", "Desvanecimiento (fading)"},
    {"QSL", "Confirmo recepcion"},
    {"QSO", "Comunicado"},
    {"QSY", "Cambiar frecuencia"},
    {"QTH", "Ubicacion"}
  };

  const int col1 = 60, col2 = 180, col3 = 500, col4 = 620;
  const int startY = 80, lineH = 32, half = 12;

  for (int i = 0; i < half; i++) {
    drawString(col1, startY + i * lineH, qcodes[i][0], LEFT);
    drawString(col2, startY + i * lineH, qcodes[i][1], LEFT);
    if (i + half < 23) {
      drawString(col3, startY + i * lineH, qcodes[i + half][0], LEFT);
      drawString(col4, startY + i * lineH, qcodes[i + half][1], LEFT);
    }
  }

  setFont(OpenSans8B);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, "Toca para regresar al menu", CENTER);
}

// ============== MORSE CODE ==============

void drawMorseCode(int x, int y, const char* code) {
  const int dotSize = 8, dashW = 24, dashH = 8, gap = 6;
  for (int i = 0; code[i] != '\0'; i++) {
    if (code[i] == '.') {
      // Draw dot (filled circle approximation with rect)
      fillRect(x, y - dotSize/2, dotSize, dotSize, Black);
      x += dotSize + gap;
    } else if (code[i] == '-') {
      // Draw dash
      fillRect(x, y - dashH/2, dashW, dashH, Black);
      x += dashW + gap;
    }
  }
}

void DisplayRadioMorseScreen() {
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "CLAVE MORSE", CENTER);
  drawFastHLine(100, 50, SCREEN_WIDTH - 200, Black);

  // Morse codes for letters
  const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char* morse[] = {
    ".-", "-...", "-.-.", "-..", ".",
    "..-.", "--.", "....", "..", ".---",
    "-.-", ".-..", "--", "-.", "---",
    ".--.", "--.-", ".-.", "...", "-",
    "..-", "...-", ".--", "-..-", "-.--",
    "--.."
  };

  const int cols[] = {50, 200, 350, 500, 650, 800};
  const int startY = 65, lineH = 38;

  setFont(OpenSans14B);

  // Draw letters with graphical morse
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 6; col++) {
      int idx = row + col * 5;
      if (idx < 26) {
        int x = cols[col];
        int y = startY + row * lineH;
        // Draw letter
        char letterStr[2] = {letters[idx], '\0'};
        drawString(x, y, letterStr, LEFT);
        // Draw morse code graphically
        drawMorseCode(x + 30, y + 12, morse[idx]);
      }
    }
  }

  // Numbers section
  int numY = startY + 5 * lineH + 20;
  drawFastHLine(50, numY - 10, SCREEN_WIDTH - 100, Grey);

  const char* numMorse[] = {
    "-----", ".----", "..---", "...--", "....-",
    ".....", "-....", "--...", "---..", "----."
  };

  setFont(OpenSans14B);
  for (int i = 0; i < 10; i++) {
    int x = 50 + (i % 5) * 180;
    int y = numY + (i / 5) * 35;
    char numStr[2] = {'0' + i, '\0'};
    drawString(x, y, numStr, LEFT);
    drawMorseCode(x + 30, y + 12, numMorse[i]);
  }

  // SOS - separate line
  int sosY = numY + 85;
  drawFastHLine(50, sosY - 10, SCREEN_WIDTH - 100, Grey);
  setFont(OpenSans14B);
  drawString(50, sosY, "SOS", LEFT);
  drawMorseCode(110, sosY + 12, "...---...");

  // Prosigns section
  int proY = sosY + 45;
  drawFastHLine(50, proY - 10, SCREEN_WIDTH - 100, Grey);
  setFont(OpenSans12B);
  drawString(50, proY, "Prosigns:", LEFT);

  setFont(OpenSans10B);
  // AR (end)
  drawString(200, proY, "AR", LEFT);
  drawMorseCode(240, proY + 12, ".-.-.");

  // SK (end QSO)
  drawString(380, proY, "SK", LEFT);
  drawMorseCode(420, proY + 12, "...-.-");

  // BT (pause)
  drawString(560, proY, "BT", LEFT);
  drawMorseCode(600, proY + 12, "-...-");

  // KN (go ahead)
  drawString(740, proY, "KN", LEFT);
  drawMorseCode(780, proY + 12, "-.--.");

  setFont(OpenSans8B);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, "Toca para regresar al menu", CENTER);
}

// ============== DXCC SCREENS ==============

void DisplayRadioDXCCScreen() {
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "PREFIJOS DXCC - Americas", CENTER);
  drawFastHLine(100, 60, SCREEN_WIDTH - 200, Black);
  setFont(OpenSans12B);

  const char* dxcc[][2] = {
    {"CE", "Chile"}, {"CE0", "Isla Pascua"}, {"CM/CO", "Cuba"},
    {"CP", "Bolivia"}, {"CX", "Uruguay"}, {"HC", "Ecuador"},
    {"HI", "Rep. Dominicana"}, {"HK", "Colombia"}, {"HP", "Panama"},
    {"HR", "Honduras"}, {"HH", "Haiti"}, {"J3", "Granada"},
    {"K/W/N", "USA"}, {"KG4", "Guantanamo"}, {"KH6", "Hawaii"},
    {"KL7", "Alaska"}, {"KP4", "Puerto Rico"}, {"LU", "Argentina"},
    {"OA", "Peru"}, {"PY", "Brasil"}, {"PZ", "Surinam"},
    {"TG", "Guatemala"}, {"TI", "Costa Rica"}, {"V3", "Belice"},
    {"VE", "Canada"}, {"VP2", "Antillas"}, {"VP5", "Turks/Caicos"},
    {"XE", "Mexico"}, {"YN", "Nicaragua"}, {"YS", "El Salvador"},
    {"YV", "Venezuela"}, {"ZP", "Paraguay"}, {"8P", "Barbados"}
  };

  const int col1 = 35, col2 = 125, col3 = 350, col4 = 440, col5 = 650, col6 = 740;
  const int startY = 90, lineH = 32, perCol = 9;

  for (int i = 0; i < perCol; i++) {
    if (i < 27) {
      drawString(col1, startY + i * lineH, dxcc[i][0], LEFT);
      drawString(col2, startY + i * lineH, dxcc[i][1], LEFT);
    }
    if (i + perCol < 27) {
      drawString(col3, startY + i * lineH, dxcc[i + perCol][0], LEFT);
      drawString(col4, startY + i * lineH, dxcc[i + perCol][1], LEFT);
    }
    if (i + perCol * 2 < 27) {
      drawString(col5, startY + i * lineH, dxcc[i + perCol * 2][0], LEFT);
      drawString(col6, startY + i * lineH, dxcc[i + perCol * 2][1], LEFT);
    }
  }

  setFont(OpenSans8B);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50, "Pagina 1/3", CENTER);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, "Toca para siguiente pagina >>", CENTER);
}

void DisplayRadioDXCC2Screen() {
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "PREFIJOS DXCC - Europa", CENTER);
  drawFastHLine(100, 60, SCREEN_WIDTH - 200, Black);
  setFont(OpenSans12B);

  const char* dxcc[][2] = {
    {"CT", "Portugal"}, {"DL", "Alemania"}, {"EA", "Espana"},
    {"EA6", "Baleares"}, {"EA8", "Canarias"}, {"EI", "Irlanda"},
    {"F", "Francia"}, {"G", "Inglaterra"}, {"GD", "Isla Man"},
    {"GI", "Irlanda Norte"}, {"GM", "Escocia"}, {"GW", "Gales"},
    {"HA", "Hungria"}, {"HB", "Suiza"}, {"I", "Italia"},
    {"LA", "Noruega"}, {"LX", "Luxemburgo"}, {"LZ", "Bulgaria"},
    {"OE", "Austria"}, {"OH", "Finlandia"}, {"OK", "Chequia"},
    {"ON", "Belgica"}, {"OZ", "Dinamarca"}, {"PA", "Holanda"},
    {"S5", "Eslovenia"}, {"SM", "Suecia"}, {"SP", "Polonia"},
    {"SV", "Grecia"}, {"UA", "Rusia Euro"}, {"UR", "Ucrania"},
    {"YO", "Rumania"}, {"YU", "Serbia"}, {"9A", "Croacia"}
  };

  const int col1 = 50, col2 = 140, col3 = 350, col4 = 440, col5 = 650, col6 = 740;
  const int startY = 90, lineH = 32, perCol = 9;

  for (int i = 0; i < perCol; i++) {
    if (i < 27) {
      drawString(col1, startY + i * lineH, dxcc[i][0], LEFT);
      drawString(col2, startY + i * lineH, dxcc[i][1], LEFT);
    }
    if (i + perCol < 27) {
      drawString(col3, startY + i * lineH, dxcc[i + perCol][0], LEFT);
      drawString(col4, startY + i * lineH, dxcc[i + perCol][1], LEFT);
    }
    if (i + perCol * 2 < 27) {
      drawString(col5, startY + i * lineH, dxcc[i + perCol * 2][0], LEFT);
      drawString(col6, startY + i * lineH, dxcc[i + perCol * 2][1], LEFT);
    }
  }

  setFont(OpenSans8B);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50, "Pagina 2/3", CENTER);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, "Toca para siguiente pagina >>", CENTER);
}

void DisplayRadioDXCC3Screen() {
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "PREFIJOS DXCC - Asia/Africa/Oceania", CENTER);
  drawFastHLine(100, 60, SCREEN_WIDTH - 200, Black);
  setFont(OpenSans12B);

  const char* dxcc[][2] = {
    {"A4", "Oman"}, {"A6", "EAU"}, {"A7", "Qatar"},
    {"BV", "Taiwan"}, {"BY", "China"}, {"DU", "Filipinas"},
    {"HL", "Corea Sur"}, {"HS", "Tailandia"}, {"JA", "Japon"},
    {"VK", "Australia"}, {"VR", "Hong Kong"}, {"VS6", "Hong Kong"},
    {"YB", "Indonesia"}, {"ZL", "Nueva Zelanda"}, {"4S", "Sri Lanka"},
    {"4X", "Israel"}, {"5B", "Chipre"}, {"9K", "Kuwait"},
    {"9M2", "Malasia Oeste"}, {"9V", "Singapur"}, {"UA0", "Rusia Asia"},
    {"CN", "Marruecos"}, {"EA9", "Ceuta/Melilla"}, {"SU", "Egipto"},
    {"ZS", "Sudafrica"}, {"5H", "Tanzania"}, {"5Z", "Kenia"},
    {"7X", "Argelia"}, {"9J", "Zambia"}, {"V5", "Namibia"}
  };

  const int col1 = 50, col2 = 140, col3 = 350, col4 = 440, col5 = 650, col6 = 740;
  const int startY = 90, lineH = 32, perCol = 9;

  for (int i = 0; i < perCol; i++) {
    if (i < 27) {
      drawString(col1, startY + i * lineH, dxcc[i][0], LEFT);
      drawString(col2, startY + i * lineH, dxcc[i][1], LEFT);
    }
    if (i + perCol < 27) {
      drawString(col3, startY + i * lineH, dxcc[i + perCol][0], LEFT);
      drawString(col4, startY + i * lineH, dxcc[i + perCol][1], LEFT);
    }
    if (i + perCol * 2 < 27) {
      drawString(col5, startY + i * lineH, dxcc[i + perCol * 2][0], LEFT);
      drawString(col6, startY + i * lineH, dxcc[i + perCol * 2][1], LEFT);
    }
  }

  // Zonas info
  int noteY = startY + perCol * lineH + 15;
  drawFastHLine(50, noteY - 5, SCREEN_WIDTH - 100, Grey);
  setFont(OpenSans12B);
  drawString(50, noteY, "Regiones ITU: 1=Europa/Africa  2=Americas  3=Asia/Pacifico", LEFT);
  drawString(50, noteY + 37, "Zonas CQ: 1-40 (Mexico=6, USA=3-5, Europa=14-16, Japon=25)", LEFT);

  setFont(OpenSans8B);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50, "Pagina 3/3", CENTER);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, "Toca para regresar al menu", CENTER);
}

// ============== PROPAGATION ==============

const char* getConditionText(const String& condition) {
  if (condition == "Good" || condition == "Excellent") return "BUENA";
  if (condition == "Fair") return "REGULAR";
  if (condition == "Poor") return "MALA";
  return condition.c_str();
}

String translateGeomagField(const String& field) {
  if (field == "quiet" || field == "Quiet") return "TRANQUILO";
  if (field == "unsettled" || field == "Unsettled") return "INESTABLE";
  if (field == "active" || field == "Active") return "ACTIVO";
  if (field == "storm" || field == "Storm") return "TORMENTA";
  if (field == "minor storm" || field == "Minor storm") return "TORMENTA MENOR";
  if (field == "major storm" || field == "Major storm") return "TORMENTA MAYOR";
  return field;
}

void DisplayRadioPropagationScreen() {
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "PROPAGACION HF - Tiempo Real", CENTER);
  drawFastHLine(100, 65, SCREEN_WIDTH - 200, Black);

  bool hasData = fetchPropagationData();
  int y = 105;

  if (hasData) {
    // === LEFT SECTION: Solar Indices ===
    setFont(OpenSans14B);
    drawString(50, y, "Indices Solares", LEFT);
    y += 45;

    // SFI with interpretation
    String sfiText = "SFI: " + String(propData.solarFlux);
    if (propData.solarFlux < 100) sfiText += " (Bajo)";
    else if (propData.solarFlux >= 150) sfiText += " (Bueno)";
    else sfiText += " (Moderado)";
    drawString(70, y, sfiText, LEFT);
    y += 32;

    // A-Index
    String aText = "Indice A: " + String(propData.aIndex);
    if (propData.aIndex < 10) aText += " (Tranquilo)";
    else if (propData.aIndex >= 50) aText += " (Tormentoso)";
    else aText += " (Inestable)";
    drawString(70, y, aText, LEFT);
    y += 32;

    // K-Index
    String kText = "Indice K: " + String(propData.kIndex);
    if (propData.kIndex <= 2) kText += " (Tranquilo)";
    else if (propData.kIndex >= 5) kText += " (Tormentoso)";
    else kText += " (Activo)";
    drawString(70, y, kText, LEFT);
    y += 42;

    drawString(70, y, "Manchas Solares: " + String(propData.sunspots), LEFT);
    y += 32;

    if (propData.xray.length() > 0) {
      drawString(70, y, "X-Ray: " + propData.xray, LEFT);
      y += 30;
    }

    y += 30;
    if (propData.geomagField.length() > 0) {
      drawString(70, y, "Campo Geomagnetico: " + translateGeomagField(propData.geomagField), LEFT);
      y += 42;
    }

    if (propData.vhfAurora.length() > 0 && propData.vhfAurora != "N/A") {
      drawString(70, y, "VHF Aurora: " + propData.vhfAurora, LEFT);
      y += 32;
    }
    if (propData.vhfEskip.length() > 0) {
      drawString(70, y, "VHF E-Skip: " + propData.vhfEskip, LEFT);
    }

    // === RIGHT SECTION: Band Conditions ===
    int rightCol = 520;
    int yRight = 105;

    setFont(OpenSans14B);
    drawString(rightCol, yRight, "Condiciones", LEFT);
    yRight += 45;

    setFont(OpenSans12B);
    drawString(rightCol, yRight, "Banda", LEFT);
    drawString(rightCol + 135, yRight, "Dia", LEFT);
    drawString(rightCol + 260, yRight, "Noche", LEFT);
    yRight += 28;
    drawFastHLine(rightCol, yRight - 5, 340, Grey);

    setFont(OpenSans12B);
    // Band conditions from API
    drawString(rightCol, yRight, "80m-40m:", LEFT);
    drawString(rightCol + 135, yRight, getConditionText(propData.band80_40_day), LEFT);
    drawString(rightCol + 260, yRight, getConditionText(propData.band80_40_night), LEFT);
    yRight += 30;

    drawString(rightCol, yRight, "30m-20m:", LEFT);
    drawString(rightCol + 135, yRight, getConditionText(propData.band30_20_day), LEFT);
    drawString(rightCol + 260, yRight, getConditionText(propData.band30_20_night), LEFT);
    yRight += 30;

    drawString(rightCol, yRight, "17m-15m:", LEFT);
    drawString(rightCol + 135, yRight, getConditionText(propData.band17_15_day), LEFT);
    drawString(rightCol + 260, yRight, getConditionText(propData.band17_15_night), LEFT);
    yRight += 30;

    drawString(rightCol, yRight, "12m-10m:", LEFT);
    drawString(rightCol + 135, yRight, getConditionText(propData.band12_10_day), LEFT);
    drawString(rightCol + 260, yRight, getConditionText(propData.band12_10_night), LEFT);

    // Update timestamp
    if (propData.updated.length() > 0) {
      setFont(OpenSans10B);
      drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50, "Actualizado: " + propData.updated, CENTER);
    }

  } else {
    // === FALLBACK: Static information ===
    setFont(OpenSans12B);
    drawString(50, y, "Sin conexion - Info de referencia:", LEFT);
    y += 35;

    setFont(OpenSans10B);
    drawString(70, y, "SFI (Solar Flux): 70-300  |  Bajo <100  |  Bueno >150", LEFT);
    y += 28;
    drawString(70, y, "Indice A: 0-400  |  Tranquilo <10  |  Tormentoso >50", LEFT);
    y += 28;
    drawString(70, y, "Indice K: 0-9  |  Tranquilo 0-2  |  Tormentoso >5", LEFT);

    y += 45;
    setFont(OpenSans12B);
    drawString(50, y, "Propagacion por Banda:", LEFT);
    y += 35;

    setFont(OpenSans10B);
    drawString(70, y, "10m: Mejor con SFI alto, ciclo solar maximo", LEFT);
    y += 26;
    drawString(70, y, "12m-15m: Buenos con SFI moderado-alto", LEFT);
    y += 26;
    drawString(70, y, "17m-20m: Bandas de DX diurnas principales", LEFT);
    y += 26;
    drawString(70, y, "30m-40m: DX nocturno y regional diurno", LEFT);
    y += 26;
    drawString(70, y, "80m-160m: Bandas nocturnas, ruido alto", LEFT);

    y += 40;
    setFont(OpenSans12B);
    drawString(50, y, "Modos de Propagacion:", LEFT);
    y += 35;
    setFont(OpenSans10B);
    drawString(70, y, "Ionosferica (F2): DX, 3000+ km, depende de sol", LEFT);
    y += 26;
    drawString(70, y, "Esporadica (Es): 10m/6m, impredecible, verano", LEFT);
    y += 26;
    drawString(70, y, "Troposferica: VHF/UHF, inversiones, <500 km", LEFT);
  }

  setFont(OpenSans8B);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, "Toca para regresar al menu", CENTER);
}

// ============== CONTESTS ==============

void DisplayRadioContestsScreen() {
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "CONCURSOS PRINCIPALES", CENTER);
  drawFastHLine(100, 50, SCREEN_WIDTH - 200, Black);

  int y = 70;
  const int col1 = 50, col2 = 280, col3 = 550, col4 = 750;

  setFont(OpenSans12B);
  drawString(col1, y, "Concurso", LEFT);
  drawString(col2, y, "Fecha", LEFT);
  drawString(col3, y, "Modo", LEFT);
  drawString(col4, y, "Duracion", LEFT);
  y += 30;
  drawFastHLine(50, y - 5, SCREEN_WIDTH - 100, Grey);

  setFont(OpenSans10B);

  const char* contests[][4] = {
    {"CQ WW DX CW", "Nov (ultimo fin)", "CW", "48h"},
    {"CQ WW DX SSB", "Oct (ultimo fin)", "SSB", "48h"},
    {"CQ WPX CW", "Mayo (ultimo fin)", "CW", "48h"},
    {"CQ WPX SSB", "Marzo (ultimo fin)", "SSB", "48h"},
    {"ARRL DX CW", "Feb (3er fin)", "CW", "48h"},
    {"ARRL DX SSB", "Mar (1er fin)", "SSB", "48h"},
    {"ARRL Field Day", "Jun (4to fin)", "Todos", "24h"},
    {"IARU HF World", "Jul (2do fin)", "CW/SSB", "24h"},
    {"WAE DX CW", "Ago (2do fin)", "CW", "48h"},
    {"WAE DX SSB", "Sep (2do fin)", "SSB", "48h"},
    {"JIDX CW", "Abr (2do fin)", "CW", "48h"},
    {"All Asian DX", "Jun/Sep", "CW/SSB", "48h"}
  };

  for (int i = 0; i < 12; i++) {
    drawString(col1, y, contests[i][0], LEFT);
    drawString(col2, y, contests[i][1], LEFT);
    drawString(col3, y, contests[i][2], LEFT);
    drawString(col4, y, contests[i][3], LEFT);
    y += 28;
  }

  // Notes
  y += 15;
  drawFastHLine(50, y - 10, SCREEN_WIDTH - 100, Grey);
  drawString(50, y, "Horarios: UTC. Consulta reglas actualizadas en cada organizador.", LEFT);

  setFont(OpenSans8B);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, "Toca para regresar al menu", CENTER);
}

#endif // RADIO_SCREENS_H
