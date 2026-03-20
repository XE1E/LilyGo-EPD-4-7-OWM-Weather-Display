// News Reader - RSS Feed Display for e-paper
// Fetches and displays news with touch navigation and images
//
// MODULAR: Este archivo es independiente y puede usarse en otros proyectos
// REQUIRES: TJpg_Decoder library for image support

#ifndef NEWS_READER_H
#define NEWS_READER_H

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <TJpg_Decoder.h>

// ============== CONFIGURATION ==============

#define NEWS_MAX_ITEMS      10    // Max news items to store
#define NEWS_TITLE_LEN      120   // Max title length
#define NEWS_DESC_LEN       400   // Max description length
#define NEWS_SOURCE_LEN     30    // Max source name length
#define NEWS_IMAGE_URL_LEN  200   // Max image URL length

// Available RSS feeds
struct RSSFeed {
  const char* name;
  const char* url;
};

const RSSFeed availableFeeds[] = {
  {"BBC Mundo",      "https://feeds.bbci.co.uk/mundo/rss.xml"},
  {"Aristegui",      "https://editorial.aristeguinoticias.com/feed/"},
  {"DW Espanol",     "https://rss.dw.com/xml/rss-sp-all"},
  {"RT Espanol",     "https://actualidad.rt.com/feeds/all.rss"}
};
const int NUM_FEEDS = sizeof(availableFeeds) / sizeof(availableFeeds[0]);

int currentFeedIndex = 0;  // Default to BBC Mundo

// ============== DATA STRUCTURES ==============

struct NewsItem {
  char title[NEWS_TITLE_LEN];
  char description[NEWS_DESC_LEN];
  char source[NEWS_SOURCE_LEN];
  char imageUrl[NEWS_IMAGE_URL_LEN];
  char pubDate[32];
  bool valid;
  bool hasImage;
};

struct NewsData {
  NewsItem items[NEWS_MAX_ITEMS];
  int count;
  int currentPage;
  unsigned long lastUpdate;
  bool valid;
};

NewsData newsData = {{}, 0, 0, 0, false};

// Extract image URL from RSS item (checks multiple formats)
String extractImageUrl(const String& itemXml) {
  // Try <enclosure> tag (common in RSS 2.0)
  int encStart = itemXml.indexOf("<enclosure");
  if (encStart != -1) {
    int urlStart = itemXml.indexOf("url=\"", encStart);
    if (urlStart != -1) {
      urlStart += 5;
      int urlEnd = itemXml.indexOf("\"", urlStart);
      if (urlEnd != -1) {
        String url = itemXml.substring(urlStart, urlEnd);
        if (url.indexOf(".jpg") != -1 || url.indexOf(".jpeg") != -1 ||
            url.indexOf(".png") != -1 || url.indexOf(".webp") != -1) {
          return url;
        }
      }
    }
  }

  // Try <media:content> tag
  int mediaStart = itemXml.indexOf("<media:content");
  if (mediaStart != -1) {
    int urlStart = itemXml.indexOf("url=\"", mediaStart);
    if (urlStart != -1) {
      urlStart += 5;
      int urlEnd = itemXml.indexOf("\"", urlStart);
      if (urlEnd != -1) {
        return itemXml.substring(urlStart, urlEnd);
      }
    }
  }

  // Try <media:thumbnail> tag
  int thumbStart = itemXml.indexOf("<media:thumbnail");
  if (thumbStart != -1) {
    int urlStart = itemXml.indexOf("url=\"", thumbStart);
    if (urlStart != -1) {
      urlStart += 5;
      int urlEnd = itemXml.indexOf("\"", urlStart);
      if (urlEnd != -1) {
        return itemXml.substring(urlStart, urlEnd);
      }
    }
  }

  // Try <image> inside item
  int imgStart = itemXml.indexOf("<image>");
  if (imgStart != -1) {
    int urlStart = itemXml.indexOf("<url>", imgStart);
    if (urlStart != -1) {
      urlStart += 5;
      int urlEnd = itemXml.indexOf("</url>", urlStart);
      if (urlEnd != -1) {
        return itemXml.substring(urlStart, urlEnd);
      }
    }
  }

  return "";
}

// ============== EXTERNAL DEPENDENCIES ==============

extern void setFont(GFXfont const &font);
extern void drawString(int x, int y, String text, alignment align);
extern void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color);
extern void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern uint8_t *framebuffer;

// ============== IMAGE HANDLING ==============

// Image position for current decode
int newsImageX = 0;
int newsImageY = 0;
int newsImageMaxW = 400;
int newsImageMaxH = 280;

// JPEG decoder callback - writes to e-paper framebuffer
bool newsImageCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  // Offset by image position
  x += newsImageX;
  y += newsImageY;

  // Bounds check
  if (x >= 960 || y >= 540) return false;

  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      int16_t px = x + i;
      int16_t py = y + j;

      if (px < 960 && py < 540) {
        // Convert RGB565 to grayscale (4-bit) with brightness boost
        uint16_t rgb = bitmap[j * w + i];
        uint8_t r = (rgb >> 11) & 0x1F;
        uint8_t g = (rgb >> 5) & 0x3F;
        uint8_t b = rgb & 0x1F;

        // Weighted grayscale: Y = 0.299*R + 0.587*G + 0.114*B
        int gray = ((r * 8 * 299) + (g * 4 * 587) + (b * 8 * 114)) / 1000;
        // Lighten image: boost brightness by ~20%
        gray = (gray * 120) / 100;
        if (gray > 255) gray = 255;
        gray = gray >> 4;  // Convert to 4-bit (0-15)

        // Write to framebuffer (4-bit packed, 2 pixels per byte)
        int idx = (py * 960 + px) / 2;
        if (px % 2 == 0) {
          framebuffer[idx] = (framebuffer[idx] & 0x0F) | (gray << 4);
        } else {
          framebuffer[idx] = (framebuffer[idx] & 0xF0) | gray;
        }
      }
    }
  }
  return true;
}

// Download and display image from URL
bool displayNewsImage(const char* imageUrl, int x, int y, int maxW, int maxH) {
  if (!imageUrl || strlen(imageUrl) == 0) return false;

  Serial.printf("News: Fetching image %s\n", imageUrl);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, imageUrl);
  http.setTimeout(10000);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("News image HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  int len = http.getSize();
  if (len <= 0 || len > 100000) {  // Max 100KB image
    Serial.printf("News image size invalid: %d\n", len);
    http.end();
    return false;
  }

  // Allocate buffer in PSRAM if available
  uint8_t* imgBuffer = (uint8_t*)ps_malloc(len);
  if (!imgBuffer) {
    imgBuffer = (uint8_t*)malloc(len);
  }
  if (!imgBuffer) {
    Serial.println("News image: failed to allocate buffer");
    http.end();
    return false;
  }

  // Read image data
  WiFiClient* stream = http.getStreamPtr();
  int bytesRead = 0;
  while (http.connected() && bytesRead < len) {
    size_t available = stream->available();
    if (available) {
      int read = stream->readBytes(imgBuffer + bytesRead, available);
      bytesRead += read;
    }
    delay(1);
  }
  http.end();

  Serial.printf("News image: read %d bytes\n", bytesRead);

  // Set position for callback
  newsImageX = x;
  newsImageY = y;
  newsImageMaxW = maxW;
  newsImageMaxH = maxH;

  // Setup decoder
  TJpgDec.setJpgScale(1);  // No scaling initially
  TJpgDec.setCallback(newsImageCallback);

  // Get image dimensions
  uint16_t imgW, imgH;
  TJpgDec.getJpgSize(&imgW, &imgH, imgBuffer, bytesRead);
  Serial.printf("News image: %dx%d\n", imgW, imgH);

  // Calculate scale to fit in maxW x maxH (TJpgDec only supports 1,2,4,8)
  // Pick largest scale where image still fits
  int scale = 1;
  if (imgW > maxW * 4 || imgH > maxH * 4) scale = 8;
  else if (imgW > maxW * 2 || imgH > maxH * 2) scale = 4;
  else if (imgW > maxW || imgH > maxH) scale = 2;

  TJpgDec.setJpgScale(scale);
  Serial.printf("News image: using scale 1/%d\n", scale);

  // Draw image
  TJpgDec.drawJpg(0, 0, imgBuffer, bytesRead);

  free(imgBuffer);
  Serial.println("News image: displayed");
  return true;
}

// ============== XML PARSING HELPERS ==============

// Extract text between tags, decode HTML entities
String extractRSSValue(const String& xml, const String& tag, int startFrom = 0) {
  String startTag = "<" + tag;
  String endTag = "</" + tag + ">";

  int start = xml.indexOf(startTag, startFrom);
  if (start == -1) return "";

  // Find end of opening tag (handle attributes)
  int tagEnd = xml.indexOf(">", start);
  if (tagEnd == -1) return "";

  // Check for CDATA
  int contentStart = tagEnd + 1;
  String content;

  if (xml.substring(contentStart, contentStart + 9) == "<![CDATA[") {
    contentStart += 9;
    int cdataEnd = xml.indexOf("]]>", contentStart);
    if (cdataEnd == -1) return "";
    content = xml.substring(contentStart, cdataEnd);
  } else {
    int end = xml.indexOf(endTag, contentStart);
    if (end == -1) return "";
    content = xml.substring(contentStart, end);
  }

  // Decode common HTML entities
  content.replace("&amp;", "&");
  content.replace("&lt;", "<");
  content.replace("&gt;", ">");
  content.replace("&quot;", "\"");
  content.replace("&apos;", "'");
  content.replace("&#39;", "'");
  content.replace("&nbsp;", " ");

  // Remove HTML tags from description
  String clean = "";
  bool inTag = false;
  for (unsigned int i = 0; i < content.length(); i++) {
    if (content[i] == '<') inTag = true;
    else if (content[i] == '>') inTag = false;
    else if (!inTag) clean += content[i];
  }

  clean.trim();
  return clean;
}

// Find position of nth occurrence of <item>
int findItemPosition(const String& xml, int itemNum) {
  int pos = 0;
  for (int i = 0; i <= itemNum; i++) {
    pos = xml.indexOf("<item", pos);
    if (pos == -1) return -1;
    if (i < itemNum) pos++;
  }
  return pos;
}

// ============== RSS FETCHING ==============

bool fetchNewsRSS(int feedIndex = -1) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("News: WiFi not connected");
    return false;
  }

  // Use specified feed or current selection
  if (feedIndex >= 0 && feedIndex < NUM_FEEDS) {
    currentFeedIndex = feedIndex;
  }
  const char* rssUrl = availableFeeds[currentFeedIndex].url;
  Serial.printf("News: Fetching %s\n", rssUrl);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, rssUrl);
  http.setTimeout(15000);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("News HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  String xml = http.getString();
  http.end();

  Serial.printf("News: Received %d bytes\n", xml.length());

  // Extract channel title as source
  String channelTitle = extractRSSValue(xml, "title");
  if (channelTitle.length() > NEWS_SOURCE_LEN - 1) {
    channelTitle = channelTitle.substring(0, NEWS_SOURCE_LEN - 1);
  }

  // Parse items
  newsData.count = 0;
  for (int i = 0; i < NEWS_MAX_ITEMS; i++) {
    int itemPos = findItemPosition(xml, i);
    if (itemPos == -1) break;

    // Find end of this item
    int itemEnd = xml.indexOf("</item>", itemPos);
    if (itemEnd == -1) break;

    String itemXml = xml.substring(itemPos, itemEnd);

    // Extract fields
    String title = extractRSSValue(itemXml, "title");
    String desc = extractRSSValue(itemXml, "description");
    String pubDate = extractRSSValue(itemXml, "pubDate");

    if (title.length() == 0) continue;

    // Truncate if needed
    if (title.length() > NEWS_TITLE_LEN - 1) {
      title = title.substring(0, NEWS_TITLE_LEN - 4) + "...";
    }
    if (desc.length() > NEWS_DESC_LEN - 1) {
      desc = desc.substring(0, NEWS_DESC_LEN - 4) + "...";
    }

    // Extract image URL
    String imageUrl = extractImageUrl(itemXml);

    // Store
    strncpy(newsData.items[i].title, title.c_str(), NEWS_TITLE_LEN - 1);
    strncpy(newsData.items[i].description, desc.c_str(), NEWS_DESC_LEN - 1);
    strncpy(newsData.items[i].source, channelTitle.c_str(), NEWS_SOURCE_LEN - 1);
    strncpy(newsData.items[i].pubDate, pubDate.c_str(), 31);
    if (imageUrl.length() > 0 && imageUrl.length() < NEWS_IMAGE_URL_LEN) {
      strncpy(newsData.items[i].imageUrl, imageUrl.c_str(), NEWS_IMAGE_URL_LEN - 1);
      newsData.items[i].hasImage = true;
    } else {
      newsData.items[i].imageUrl[0] = '\0';
      newsData.items[i].hasImage = false;
    }
    newsData.items[i].valid = true;
    newsData.count++;
  }

  newsData.currentPage = 0;
  newsData.lastUpdate = millis();
  newsData.valid = (newsData.count > 0);

  Serial.printf("News: Parsed %d items\n", newsData.count);
  return newsData.valid;
}

// ============== TEXT WRAPPING ==============

// Draw text with word wrap
// charWidth: approximate pixels per character (16B~11, 14B~10, 12B~8, 10B~7)
int drawWrappedText(int x, int y, int maxWidth, int lineHeight, const char* text, int maxLines, int charWidth = 8) {
  String str = text;
  int lines = 0;
  int currentY = y;

  while (str.length() > 0 && lines < maxLines) {
    int charsPerLine = maxWidth / charWidth;
    if (charsPerLine < 10) charsPerLine = 10;

    if ((int)str.length() <= charsPerLine) {
      drawString(x, currentY, str, LEFT);
      lines++;
      break;
    }

    // Find last space within limit
    int breakPoint = charsPerLine;
    if (breakPoint >= (int)str.length()) breakPoint = str.length() - 1;
    while (breakPoint > 0 && str[breakPoint] != ' ') {
      breakPoint--;
    }
    if (breakPoint == 0) breakPoint = charsPerLine;

    String line = str.substring(0, breakPoint);
    drawString(x, currentY, line, LEFT);

    str = str.substring(breakPoint);
    str.trim();
    currentY += lineHeight;
    lines++;
  }

  return lines;
}

// ============== DISPLAY FUNCTIONS ==============

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

// Draw rounded rectangle (same as radio_screens.h)
void drawNewsRoundedRect(int x, int y, int w, int h, int r, int thickness, uint8_t color) {
  for (int t = 0; t < thickness; t++) {
    drawFastHLine(x + r, y + t, w - 2*r, color);
    drawFastHLine(x + r, y + h - 1 - t, w - 2*r, color);
  }
  for (int t = 0; t < thickness; t++) {
    for (int i = r; i < h - r; i++) {
      fillRect(x + t, y + i, 1, 1, color);
      fillRect(x + w - 1 - t, y + i, 1, 1, color);
    }
  }
  for (int t = 0; t < thickness; t++) {
    int cr = r - t;
    if (cr > 0) {
      for (int i = 0; i <= cr; i++) {
        for (int j = 0; j <= cr; j++) {
          if (i*i + j*j <= cr*cr && i*i + j*j >= (cr-1)*(cr-1)) {
            fillRect(x + r - i, y + r - j, 1, 1, color);
            fillRect(x + w - r - 1 + i, y + r - j, 1, 1, color);
            fillRect(x + r - i, y + h - r - 1 + j, 1, 1, color);
            fillRect(x + w - r - 1 + i, y + h - r - 1 + j, 1, 1, color);
          }
        }
      }
    }
  }
}

// Feed selection menu
void DisplayNewsFeedMenu() {
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "SELECCIONA FUENTE", CENTER);
  drawFastHLine(100, 55, SCREEN_WIDTH - 200, Black);

  // Grid of feed buttons (2x2 or adjust based on NUM_FEEDS)
  const int btnW = 400, btnH = 150, gapX = 60, gapY = 30;
  int startX = (SCREEN_WIDTH - (btnW * 2 + gapX)) / 2;
  int startY = 80;

  setFont(OpenSans16B);
  for (int i = 0; i < NUM_FEEDS; i++) {
    int row = i / 2;
    int col = i % 2;
    int x = startX + col * (btnW + gapX);
    int y = startY + row * (btnH + gapY);

    drawNewsRoundedRect(x, y, btnW, btnH, 12, 4, Black);
    drawString(x + btnW/2, y + btnH/2 - 12, availableFeeds[i].name, CENTER);
  }

  setFont(OpenSans10B);
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, "Toca una fuente para ver noticias | Centro: Salir", CENTER);
}

void DisplayNewsScreen() {
  // Title bar with feed name
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 15, "NOTICIAS", CENTER);

  // Feed name on left, page indicator on right
  setFont(OpenSans10B);
  drawString(100, 20, availableFeeds[currentFeedIndex].name, LEFT);

  setFont(OpenSans12B);
  String pageStr = String(newsData.currentPage + 1) + "/" + String(newsData.count);
  drawString(SCREEN_WIDTH - 50, 20, pageStr, RIGHT);

  drawFastHLine(50, 55, SCREEN_WIDTH - 100, Black);

  if (!newsData.valid || newsData.count == 0) {
    setFont(OpenSans14B);
    drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20, "Sin noticias disponibles", CENTER);
    setFont(OpenSans12B);
    drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20, "Verifica conexion WiFi", CENTER);
    return;
  }

  // Current news item
  NewsItem& item = newsData.items[newsData.currentPage];

  // Image area dimensions (larger for better visibility)
  const int imgW = 400, imgH = 280;
  const int imgX = 70, imgY = 90;
  int textX = 50;
  int textWidth = SCREEN_WIDTH - 100;

  // Try to display image if available
  if (item.hasImage && strlen(item.imageUrl) > 0) {
    // Fetch and display image (no border, full size)
    if (displayNewsImage(item.imageUrl, imgX, imgY, imgW, imgH)) {
      // Image displayed, text goes to the right (closer to photo)
      textX = imgX + imgW - 35;
      textWidth = SCREEN_WIDTH - textX - 50;
    }
  }

  // Title - to the right of image or full width
  setFont(OpenSans16B);
  int titleLines = drawWrappedText(textX, 65, textWidth, 36, item.title, 4, 15);

  // Description - below image (moved up 75px)
  int descY = imgY + imgH - 65;
  setFont(OpenSans12B);
  drawWrappedText(50, descY, SCREEN_WIDTH - 100, 28, item.description, 8, 14);

  // Footer: source and date
  drawFastHLine(50, SCREEN_HEIGHT - 80, SCREEN_WIDTH - 100, Grey);
  setFont(OpenSans10B);
  drawString(50, SCREEN_HEIGHT - 60, item.source, LEFT);

  // Parse and show simplified date
  String dateStr = item.pubDate;
  if (dateStr.length() > 16) {
    dateStr = dateStr.substring(0, 16); // "Mon, 19 Mar 2026"
  }
  drawString(SCREEN_WIDTH - 50, SCREEN_HEIGHT - 60, dateStr, RIGHT);

  // Navigation hints
  setFont(OpenSans10B);
  if (newsData.currentPage > 0) {
    drawString(100, SCREEN_HEIGHT - 30, "< Anterior", LEFT);
  } else {
    drawString(100, SCREEN_HEIGHT - 30, "< Menu", LEFT);
  }
  if (newsData.currentPage < newsData.count - 1) {
    drawString(SCREEN_WIDTH - 100, SCREEN_HEIGHT - 30, "Siguiente >", RIGHT);
  } else {
    drawString(SCREEN_WIDTH - 100, SCREEN_HEIGHT - 30, "Menu >", RIGHT);
  }
  drawString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, "Centro: Menu", CENTER);
}

// Navigate news pages
void newsNextPage() {
  if (newsData.currentPage < newsData.count - 1) {
    newsData.currentPage++;
  }
}

void newsPrevPage() {
  if (newsData.currentPage > 0) {
    newsData.currentPage--;
  }
}

// Handle touch for news navigation
// Returns: 0 = no action, 1 = prev page, 2 = next page, 3 = exit
int handleNewsTouch(int touchX, int touchY) {
  // Left third = previous
  if (touchX < SCREEN_WIDTH / 3) {
    if (newsData.currentPage > 0) {
      newsPrevPage();
      return 1;
    }
  }
  // Right third = next
  else if (touchX > SCREEN_WIDTH * 2 / 3) {
    if (newsData.currentPage < newsData.count - 1) {
      newsNextPage();
      return 2;
    }
  }
  // Middle = exit to menu
  else {
    return 3;
  }
  return 0;
}

#endif // NEWS_READER_H
