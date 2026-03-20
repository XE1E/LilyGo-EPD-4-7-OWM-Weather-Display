// Callsign Display - Large letters for amateur radio callsign
// Configurable: 4-6 characters, auto-sizing, full alphabet + numbers
// NOTA: Esta pantalla es privada/oculta - NO DOCUMENTAR en manual

#ifndef CALLSIGN_H
#define CALLSIGN_H

// ============== CONFIGURATION ==============
// Edit these values to customize your callsign display

#define CALLSIGN_TEXT     "XE1E"                    // 4-6 characters (A-Z, 0-9)
#define CALLSIGN_LINE1    "Enrique"                 // First line of subtitle (max 50 chars)
#define CALLSIGN_LINE2    "Ciudad de Mexico - EK09" // Second line of subtitle (max 50 chars)

// ============== END CONFIGURATION ==============

// Draw rounded rectangle helper
void fillRoundRect(int x, int y, int w, int h, int r, uint8_t color) {
  if (r > w/2) r = w/2;
  if (r > h/2) r = h/2;
  fillRect(x + r, y, w - 2*r, h, color);
  fillRect(x, y + r, r, h - 2*r, color);
  fillRect(x + w - r, y + r, r, h - 2*r, color);
  fillCircle(x + r, y + r, r, color);
  fillCircle(x + w - r - 1, y + r, r, color);
  fillCircle(x + r, y + h - r - 1, r, color);
  fillCircle(x + w - r - 1, y + h - r - 1, r, color);
}

// Draw thick diagonal line with rounded ends
void drawThickDiagonalRound(int x1, int y1, int x2, int y2, int thickness, uint8_t color) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  float len = sqrt(dx*dx + dy*dy);
  float nx = -dy / len * thickness / 2;
  float ny = dx / len * thickness / 2;
  fillTriangle(x1 + nx, y1 + ny, x1 - nx, y1 - ny, x2 + nx, y2 + ny, color);
  fillTriangle(x1 - nx, y1 - ny, x2 - nx, y2 - ny, x2 + nx, y2 + ny, color);
  int r = thickness / 2;
  fillCircle(x1, y1, r, color);
  fillCircle(x2, y2, r, color);
}

// ============== LETTER DRAWING FUNCTIONS ==============
// All letters use: x, y (top-left), w (width), h (height), t (thickness), c (color)

void drawChar_A(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  // Left diagonal
  drawThickDiagonalRound(x, y + h, x + w/2, y, t, c);
  // Right diagonal
  drawThickDiagonalRound(x + w/2, y, x + w, y + h, t, c);
  // Middle bar
  fillRoundRect(x + w/5, y + h*3/5, w*3/5, t, r, c);
}

void drawChar_B(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x, y, w*3/4, t, r, c);
  fillRoundRect(x, y + h/2 - t/2, w*3/4, t, r, c);
  fillRoundRect(x, y + h - t, w*3/4, t, r, c);
  fillRoundRect(x + w - t, y, t, h/2, r, c);
  fillRoundRect(x + w - t, y + h/2, t, h/2, r, c);
}

void drawChar_C(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_D(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x, y, w*2/3, t, r, c);
  fillRoundRect(x, y + h - t, w*2/3, t, r, c);
  fillRoundRect(x + w - t, y + t, t, h - 2*t, r, c);
  drawThickDiagonalRound(x + w*2/3, y, x + w, y + t*2, t, c);
  drawThickDiagonalRound(x + w*2/3, y + h, x + w, y + h - t*2, t, c);
}

void drawChar_E(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h/2 - t/2, w*3/4, t, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_F(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h/2 - t/2, w*3/4, t, r, c);
}

void drawChar_G(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
  fillRoundRect(x + w - t, y + h/2, t, h/2, r, c);
  fillRoundRect(x + w/2, y + h/2, w/2, t, r, c);
}

void drawChar_H(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  fillRoundRect(x, y + h/2 - t/2, w, t, r, c);
}

void drawChar_I(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  int cx = x + w/2 - t/2;
  fillRoundRect(cx, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_J(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x + w - t, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
  fillRoundRect(x, y + h*2/3, t, h/3, r, c);
}

void drawChar_K(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  drawThickDiagonalRound(x + t, y + h/2, x + w, y, t, c);
  drawThickDiagonalRound(x + t, y + h/2, x + w, y + h, t, c);
}

void drawChar_L(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_M(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  drawThickDiagonalRound(x, y, x + w/2, y + h/3, t, c);
  drawThickDiagonalRound(x + w/2, y + h/3, x + w, y, t, c);
}

void drawChar_N(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  drawThickDiagonalRound(x, y, x + w, y + h, t, c);
}

void drawChar_O(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_P(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h/2 - t/2, w, t, r, c);
  fillRoundRect(x + w - t, y, t, h/2, r, c);
}

void drawChar_Q(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
  drawThickDiagonalRound(x + w/2, y + h*2/3, x + w, y + h, t*2/3, c);
}

void drawChar_R(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h/2 - t/2, w*3/4, t, r, c);
  fillRoundRect(x + w - t, y, t, h/2, r, c);
  drawThickDiagonalRound(x + w*2/3, y + h/2, x + w, y + h, t, c);
}

void drawChar_S(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y, t, h/2, r, c);
  fillRoundRect(x, y + h/2 - t/2, w, t, r, c);
  fillRoundRect(x + w - t, y + h/2, t, h/2, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_T(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x + w/2 - t/2, y, t, h, r, c);
}

void drawChar_U(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_V(int x, int y, int w, int h, int t, uint8_t c) {
  drawThickDiagonalRound(x, y, x + w/2, y + h, t, c);
  drawThickDiagonalRound(x + w/2, y + h, x + w, y, t, c);
}

void drawChar_W(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
  fillRoundRect(x + w/2 - t/2, y + h/3, t, h*2/3, r, c);
}

void drawChar_X(int x, int y, int w, int h, int t, uint8_t c) {
  drawThickDiagonalRound(x, y, x + w, y + h, t, c);
  drawThickDiagonalRound(x + w, y, x, y + h, t, c);
}

void drawChar_Y(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  drawThickDiagonalRound(x, y, x + w/2, y + h/2, t, c);
  drawThickDiagonalRound(x + w, y, x + w/2, y + h/2, t, c);
  fillRoundRect(x + w/2 - t/2, y + h/2, t, h/2, r, c);
}

void drawChar_Z(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, w, t, r, c);
  drawThickDiagonalRound(x + w, y, x, y + h, t, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

// ============== NUMBER DRAWING FUNCTIONS ==============

void drawChar_0(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
  drawThickDiagonalRound(x + w*3/4, y + h/4, x + w/4, y + h*3/4, t*2/3, c);
}

void drawChar_1(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  int cx = x + w/2 - t/2;
  fillRoundRect(cx, y, t, h, r, c);
  drawThickDiagonalRound(cx + t/2, y + t/2, cx - w/4, y + h/5, t*2/3, c);
  fillRoundRect(x + w/4, y + h - t, w/2, t, r, c);
}

void drawChar_2(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x + w - t, y, t, h/2, r, c);
  fillRoundRect(x, y + h/2 - t/2, w, t, r, c);
  fillRoundRect(x, y + h/2, t, h/2, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_3(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  fillRoundRect(x + w/3, y + h/2 - t/2, w*2/3, t, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_4(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h*2/3, r, c);
  fillRoundRect(x, y + h*2/3 - t, w, t, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
}

void drawChar_5(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y, t, h/2, r, c);
  fillRoundRect(x, y + h/2 - t/2, w, t, r, c);
  fillRoundRect(x + w - t, y + h/2, t, h/2, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_6(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x, y + h/2 - t/2, w, t, r, c);
  fillRoundRect(x + w - t, y + h/2, t, h/2, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_7(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
}

void drawChar_8(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, t, h, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y + h/2 - t/2, w, t, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

void drawChar_9(int x, int y, int w, int h, int t, uint8_t c) {
  int r = t / 2;
  fillRoundRect(x, y, w, t, r, c);
  fillRoundRect(x, y, t, h/2, r, c);
  fillRoundRect(x, y + h/2 - t/2, w, t, r, c);
  fillRoundRect(x + w - t, y, t, h, r, c);
  fillRoundRect(x, y + h - t, w, t, r, c);
}

// ============== CHARACTER DISPATCHER ==============

void drawLargeChar(char ch, int x, int y, int w, int h, int t, uint8_t c) {
  switch(ch) {
    case 'A': drawChar_A(x, y, w, h, t, c); break;
    case 'B': drawChar_B(x, y, w, h, t, c); break;
    case 'C': drawChar_C(x, y, w, h, t, c); break;
    case 'D': drawChar_D(x, y, w, h, t, c); break;
    case 'E': drawChar_E(x, y, w, h, t, c); break;
    case 'F': drawChar_F(x, y, w, h, t, c); break;
    case 'G': drawChar_G(x, y, w, h, t, c); break;
    case 'H': drawChar_H(x, y, w, h, t, c); break;
    case 'I': drawChar_I(x, y, w, h, t, c); break;
    case 'J': drawChar_J(x, y, w, h, t, c); break;
    case 'K': drawChar_K(x, y, w, h, t, c); break;
    case 'L': drawChar_L(x, y, w, h, t, c); break;
    case 'M': drawChar_M(x, y, w, h, t, c); break;
    case 'N': drawChar_N(x, y, w, h, t, c); break;
    case 'O': drawChar_O(x, y, w, h, t, c); break;
    case 'P': drawChar_P(x, y, w, h, t, c); break;
    case 'Q': drawChar_Q(x, y, w, h, t, c); break;
    case 'R': drawChar_R(x, y, w, h, t, c); break;
    case 'S': drawChar_S(x, y, w, h, t, c); break;
    case 'T': drawChar_T(x, y, w, h, t, c); break;
    case 'U': drawChar_U(x, y, w, h, t, c); break;
    case 'V': drawChar_V(x, y, w, h, t, c); break;
    case 'W': drawChar_W(x, y, w, h, t, c); break;
    case 'X': drawChar_X(x, y, w, h, t, c); break;
    case 'Y': drawChar_Y(x, y, w, h, t, c); break;
    case 'Z': drawChar_Z(x, y, w, h, t, c); break;
    case '0': drawChar_0(x, y, w, h, t, c); break;
    case '1': drawChar_1(x, y, w, h, t, c); break;
    case '2': drawChar_2(x, y, w, h, t, c); break;
    case '3': drawChar_3(x, y, w, h, t, c); break;
    case '4': drawChar_4(x, y, w, h, t, c); break;
    case '5': drawChar_5(x, y, w, h, t, c); break;
    case '6': drawChar_6(x, y, w, h, t, c); break;
    case '7': drawChar_7(x, y, w, h, t, c); break;
    case '8': drawChar_8(x, y, w, h, t, c); break;
    case '9': drawChar_9(x, y, w, h, t, c); break;
    default: break; // Unknown character - skip
  }
}

// ============== MAIN CALLSIGN DRAWING ==============

void drawCallsignText(const char* text, int marginX, int marginY, int bottomSpace) {
  int screenW = 960;
  int screenH = 540;
  int len = strlen(text);

  // Clamp to 4-6 characters
  if (len < 4) len = 4;
  if (len > 6) len = 6;

  // Available space
  int availW = screenW - marginX * 2;
  int availH = screenH - marginY * 2 - bottomSpace;

  // Calculate letter size with spacing
  int spacing = availW / (len * 5);  // Dynamic spacing based on char count
  int totalSpacing = spacing * (len - 1);
  int letterW = (availW - totalSpacing) / len;
  int letterH = availH;

  // Limit height to maintain proportion (roughly 1.3:1 ratio)
  if (letterH > letterW * 1.4) {
    letterH = letterW * 1.4;
  }

  // Recalculate to center if height was limited
  int totalW = letterW * len + spacing * (len - 1);
  int startX = marginX + (availW - totalW) / 2;
  int startY = marginY + (availH - letterH) / 2;

  // Thickness proportional to letter size
  int thick = letterW / 5;
  if (thick < 12) thick = 12;
  if (thick > 40) thick = 40;

  // Draw each character
  for (int i = 0; i < len && text[i] != '\0'; i++) {
    char ch = toupper(text[i]);
    drawLargeChar(ch, startX, startY, letterW, letterH, thick, Black);
    startX += letterW + spacing;
  }
}

// Draw subtitle (one or two lines)
void drawCallsignSubtitle(const char* line1, const char* line2) {
  int y = 540 - 140;

  if (strlen(line2) > 0) {
    // Two lines - line1 larger
    setFont(OpenSans24B);
    drawString(960 / 2, y, String(line1), CENTER);
    y += 65;
    setFont(OpenSans18B);
    drawString(960 / 2, y, String(line2), CENTER);
  } else {
    // Single line
    setFont(OpenSans24B);
    drawString(960 / 2, y + 15, String(line1), CENTER);
  }
}

// Complete callsign screen
void DisplayCallsignScreen() {
  // Clear screen
  fillRect(0, 0, 960, 540, White);

  // Margins
  int marginX = 50;
  int marginY = 30;
  int bottomSpace = 120;

  // Draw large callsign text
  drawCallsignText(CALLSIGN_TEXT, marginX, marginY, bottomSpace);

  // Draw subtitle
  drawCallsignSubtitle(CALLSIGN_LINE1, CALLSIGN_LINE2);
}

#endif // CALLSIGN_H
