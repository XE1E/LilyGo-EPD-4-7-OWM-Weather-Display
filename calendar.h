#ifndef CALENDAR_H
#define CALENDAR_H

// Calendar module for LilyGo EPD 4.7" Weather Station
// Provides monthly and yearly calendar views with navigation

// External dependencies (defined in main sketch)
extern void setFont(GFXfont const &font);
extern void drawString(int x, int y, String text, alignment align);
extern void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color);
extern void drawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color);
extern void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
extern void epd_draw_pixel(int x, int y, uint8_t color, uint8_t *buffer);
extern uint8_t *framebuffer;

// Navigation offsets (defined in touch_handler.h)
extern int calendarMonthOffset;
extern int calendarYearOffset;

// Language functions (defined in lang.h)
extern const char* getMonthFull(int month);
extern const char* getMonthUpper(int month);
extern const char* getWeekdayShort(int day);
extern const char* getWeekdayMini(int day);

// Days in each month (updated for leap year when needed)
static const int baseDaysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Check if year is leap year
inline bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

// Get days in month (handles leap year)
inline int getDaysInMonth(int month, int year) {
  if (month == 1 && isLeapYear(year)) return 29;
  return baseDaysInMonth[month];
}

// Screen: Monthly Calendar
void DisplayCalendarScreen() {
  // Get current time
  time_t now = time(NULL);
  struct tm *timeinfo = localtime(&now);
  int todayDay = timeinfo->tm_mday;
  int todayMonth = timeinfo->tm_mon;
  int todayYear = timeinfo->tm_year + 1900;

  // Apply month offset for navigation
  int displayMonth = todayMonth + calendarMonthOffset;
  int displayYear = todayYear;
  while (displayMonth < 0) { displayMonth += 12; displayYear--; }
  while (displayMonth > 11) { displayMonth -= 12; displayYear++; }

  // === HEADER ===
  String title = String(getMonthFull(displayMonth)) + " " + String(displayYear);

  // Draw decorative header line
  drawFastHLine(100, 65, SCREEN_WIDTH - 200, Black);
  drawFastHLine(100, 67, SCREEN_WIDTH - 200, Black);

  // Month/year title - large and centered
  setFont(OpenSans24B);
  drawString(SCREEN_WIDTH / 2, 25, title, CENTER);

  // === NAVIGATION ARROWS (near title at top) ===
  int arrowY = 42;
  int arrowSize = 30;

  // Left arrow (previous month) - double triangle pointing left
  int leftArrowX = 80;
  fillTriangle(leftArrowX, arrowY,
               leftArrowX + arrowSize, arrowY - arrowSize/2,
               leftArrowX + arrowSize, arrowY + arrowSize/2, Black);
  fillTriangle(leftArrowX + 25, arrowY,
               leftArrowX + arrowSize + 25, arrowY - arrowSize/2,
               leftArrowX + arrowSize + 25, arrowY + arrowSize/2, Black);

  // Right arrow (next month) - double triangle pointing right
  int rightArrowX = SCREEN_WIDTH - 80;
  fillTriangle(rightArrowX, arrowY,
               rightArrowX - arrowSize, arrowY - arrowSize/2,
               rightArrowX - arrowSize, arrowY + arrowSize/2, Black);
  fillTriangle(rightArrowX - 25, arrowY,
               rightArrowX - arrowSize - 25, arrowY - arrowSize/2,
               rightArrowX - arrowSize - 25, arrowY + arrowSize/2, Black);

  // === DAY HEADERS (Week starts Monday) ===
  setFont(OpenSans16B);
  int cellWidth = 125;
  int cellHeight = 65;
  int gridWidth = 7 * cellWidth;
  int startX = (SCREEN_WIDTH - gridWidth) / 2;
  int startY = 85;

  for (int i = 0; i < 7; i++) {
    int x = startX + i * cellWidth + cellWidth / 2;
    drawString(x, startY + 5, getWeekdayShort(i), CENTER);
  }

  // === GRID LINES ===
  // Horizontal lines
  for (int i = 0; i <= 6; i++) {
    int y = startY + 30 + i * cellHeight;
    drawFastHLine(startX, y, gridWidth, Grey);
  }
  // Vertical lines (dashed)
  for (int i = 0; i <= 7; i++) {
    int x = startX + i * cellWidth;
    for (int y = startY; y < startY + 30 + 6 * cellHeight; y++) {
      if ((y - startY) % 3 == 0)
        epd_draw_pixel(x, y, Grey, framebuffer);
    }
  }

  // === CALCULATE CALENDAR ===
  struct tm firstDay = {0};
  firstDay.tm_year = displayYear - 1900;
  firstDay.tm_mon = displayMonth;
  firstDay.tm_mday = 1;
  firstDay.tm_hour = 12;
  mktime(&firstDay);
  // Convert from Sunday=0 to Monday=0 system
  int firstDayOfWeek = (firstDay.tm_wday + 6) % 7;
  int numDays = getDaysInMonth(displayMonth, displayYear);

  // === DRAW DAYS ===
  int day = 1;
  bool isCurrentMonth = (displayMonth == todayMonth && displayYear == todayYear);

  setFont(OpenSans24B);
  for (int week = 0; week < 6 && day <= numDays; week++) {
    int cellY = startY + 30 + week * cellHeight;

    for (int dow = 0; dow < 7; dow++) {
      if (week == 0 && dow < firstDayOfWeek) continue;
      if (day > numDays) break;

      int x = startX + dow * cellWidth + cellWidth / 2;
      drawString(x, cellY + 8, String(day), CENTER);

      // Highlight today with thick underline
      if (isCurrentMonth && day == todayDay) {
        int lineY = cellY + 45;
        int lineW = (day >= 10) ? 34 : 22;
        for (int i = 0; i < 7; i++) {
          drawFastHLine(x - lineW/2, lineY + i, lineW, Black);
        }
      }
      day++;
    }
  }

}

// Screen: Yearly Calendar (12 mini months)
void DisplayCalendarYearScreen() {
  // Get current year and apply offset
  time_t now = time(NULL);
  struct tm *timeinfo = localtime(&now);
  int displayYear = timeinfo->tm_year + 1900 + calendarYearOffset;

  // === HEADER with year and navigation arrows ===
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 8, String(displayYear), CENTER);
  drawFastHLine(100, 40, SCREEN_WIDTH - 200, Grey);

  // === NAVIGATION ARROWS ===
  int arrowY = 30;
  int arrowSize = 25;

  // Left arrow (previous year)
  int leftArrowX = 80;
  fillTriangle(leftArrowX, arrowY,
               leftArrowX + arrowSize, arrowY - arrowSize/2,
               leftArrowX + arrowSize, arrowY + arrowSize/2, Black);
  fillTriangle(leftArrowX + 20, arrowY,
               leftArrowX + arrowSize + 20, arrowY - arrowSize/2,
               leftArrowX + arrowSize + 20, arrowY + arrowSize/2, Black);

  // Right arrow (next year)
  int rightArrowX = SCREEN_WIDTH - 80;
  fillTriangle(rightArrowX, arrowY,
               rightArrowX - arrowSize, arrowY - arrowSize/2,
               rightArrowX - arrowSize, arrowY + arrowSize/2, Black);
  fillTriangle(rightArrowX - 20, arrowY,
               rightArrowX - arrowSize - 20, arrowY - arrowSize/2,
               rightArrowX - arrowSize - 20, arrowY + arrowSize/2, Black);

  // === 12 MINI CALENDARS (3 columns x 4 rows) ===
  const int cols = 3;
  const int rows = 4;
  int monthWidth = SCREEN_WIDTH / cols;
  int monthHeight = (SCREEN_HEIGHT - 50) / rows;
  int startY = 42;

  // Draw vertical separator lines
  for (int c = 1; c < cols; c++) {
    int lineX = c * monthWidth;
    drawFastVLine(lineX, startY, SCREEN_HEIGHT - startY, Grey);
  }

  // Draw horizontal separator lines
  for (int r = 1; r < rows; r++) {
    int lineY = startY + r * monthHeight;
    drawFastHLine(0, lineY, SCREEN_WIDTH, Grey);
  }

  for (int month = 0; month < 12; month++) {
    int col = month % cols;
    int row = month / cols;
    int boxX = col * monthWidth + 5;
    int boxY = startY + row * monthHeight + 2;
    int boxW = monthWidth - 10;

    // Month name
    setFont(OpenSans10B);
    drawString(boxX + boxW / 2, boxY, getMonthUpper(month), CENTER);

    // Calculate first day of month (Monday = 0)
    struct tm firstDay = {0};
    firstDay.tm_year = displayYear - 1900;
    firstDay.tm_mon = month;
    firstDay.tm_mday = 1;
    firstDay.tm_hour = 12;
    mktime(&firstDay);
    int firstDayOfWeek = (firstDay.tm_wday + 6) % 7;

    // Mini day grid (tighter spacing)
    int cellW = 32;
    int gridW = cellW * 7;
    int gridX = boxX + (boxW - gridW) / 2;
    int cellH = 15;
    int gridY = boxY + 16;
    int numDays = getDaysInMonth(month, displayYear);

    // Day headers
    setFont(OpenSans8B);
    for (int d = 0; d < 7; d++) {
      drawString(gridX + d * cellW + cellW / 2, gridY, getWeekdayMini(d), CENTER);
    }

    // Thin line under day headers
    drawFastHLine(gridX, gridY + 12, gridW, Grey);

    // Days
    setFont(OpenSans8B);
    int day = 1;
    for (int week = 0; week < 6 && day <= numDays; week++) {
      int y = gridY + 15 + week * cellH;
      for (int dow = 0; dow < 7; dow++) {
        if (week == 0 && dow < firstDayOfWeek) continue;
        if (day > numDays) break;

        int x = gridX + dow * cellW + cellW / 2;

        // Draw number
        drawString(x, y, String(day), CENTER);
        day++;
      }
    }
  }
}

#endif // CALENDAR_H
