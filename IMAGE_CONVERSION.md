# Image Conversion for EPD47 E-Paper Display

This document describes how to convert images to the format required by the LilyGo EPD 4.7" e-paper display.

## Requirements

- Python 3.x
- Pillow library: `pip install Pillow`

## Format

The EPD47 display uses 4-bit grayscale (16 levels). Images are stored as:
- 2 pixels per byte (high nibble = first pixel, low nibble = second pixel)
- Values: 0x00 = black, 0xFF = white
- Array of `uint8_t` in a `.h` header file

## Conversion Script

The script `convert_image.py` converts PNG/JPG images to the EPD47 format.

### Configuration (edit in script)

```python
INPUT_FILE = "your_image.jpg"      # Source image
OUTPUT_FILE = "image_data.h"       # Output header file
OUTPUT_WIDTH = 960                 # Display width
OUTPUT_HEIGHT = 540                # Display height
VAR_NAME = "worldmap"              # Variable name in generated code
BRIGHTNESS_BOOST = 1.0             # 1.0 = original, >1.0 = lighter
```

### Usage

1. Place your image in the project folder
2. Edit `convert_image.py` with your settings
3. Run: `python convert_image.py`
4. Check `worldmap_preview.png` to verify the result
5. The `.h` file is ready to include in your code

### Output Files

- `worldmap_data.h` - Header file with image data array
- `worldmap_preview.png` - Preview of converted image

## Using in Code

```cpp
#include "worldmap_data.h"

void drawImage() {
  Rect_t area = {
    .x = 0,
    .y = 0,
    .width = worldmap_width,
    .height = worldmap_height
  };
  epd_draw_grayscale_image(area, (uint8_t *)worldmap_data);
}
```

## Tips

- Use high contrast images for best results
- Simple graphics work better than photos
- The display is 960x540 pixels
- Grayscale images will be converted automatically
- For full screen: set both dimensions to 960x540 (may stretch)
- For aspect ratio: the script can crop to fit

## Current World Map

Image source: Wikimedia Commons
- File: `Standard_time_zones_of_the_world._LOC_00552771.jpg`
- License: Public domain
- URL: https://commons.wikimedia.org/wiki/File:Standard_time_zones_of_the_world._LOC_00552771.jpg
