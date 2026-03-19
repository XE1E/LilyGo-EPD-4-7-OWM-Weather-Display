// World Clock / Timezone Map Display
// Shows world map with timezone information
// Access: Callsign screen (touch subtitle) -> World Clock -> touch to return
// NOTA: Esta pantalla es privada/oculta - NO DOCUMENTAR en manual

#ifndef WORLDCLOCK_H
#define WORLDCLOCK_H

#include "worldmap_data.h"

// External dependencies (defined in main sketch)
extern void setFont(GFXfont const &font);
extern void drawString(int x, int y, String text, alignment align);
extern void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern void epd_draw_grayscale_image(Rect_t area, uint8_t *data);
extern uint8_t *framebuffer;

// Screen dimensions
#define SCREEN_W 960
#define SCREEN_H 540

// Draw the world map bitmap (full screen)
void drawWorldMap() {
  Rect_t area = {
    .x = 0,
    .y = 0,
    .width = worldmap_width,
    .height = worldmap_height
  };
  epd_draw_grayscale_image(area, (uint8_t *)worldmap_data);
}

// ============== MAIN DISPLAY FUNCTION ==============

void DisplayWorldClockScreen() {
  // Draw the world map bitmap (full screen, no title for now)
  drawWorldMap();
}

#endif // WORLDCLOCK_H
