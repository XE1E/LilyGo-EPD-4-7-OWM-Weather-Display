#!/usr/bin/env python3
"""
Convert PNG image to EPD47 4-bit grayscale format
Output: .h file with uint8_t array (2 pixels per byte)
"""

from PIL import Image, ImageEnhance, ImageOps
import sys

# Configuration
INPUT_FILE = "Standard_time_zones_of_the_world._LOC_00552771.jpg"
OUTPUT_FILE = "worldmap_data.h"
OUTPUT_WIDTH = 960   # Full screen width
OUTPUT_HEIGHT = 540  # Full screen height
VAR_NAME = "worldmap"

# Processing options
BRIGHTNESS_BOOST = 1.0   # >1.0 = lighter, <1.0 = darker (1.0 = original)

def convert_image():
    # Open image
    img = Image.open(INPUT_FILE)
    print(f"Original size: {img.width} x {img.height}")

    # Stretch to exact screen size (slight deformation OK)
    new_width = OUTPUT_WIDTH
    new_height = OUTPUT_HEIGHT
    img = img.resize((new_width, new_height), Image.LANCZOS)

    print(f"Stretched to: {new_width} x {new_height}")

    # Convert to grayscale
    img = img.convert('L')

    # Lighten the grays
    enhancer = ImageEnhance.Brightness(img)
    img = enhancer.enhance(BRIGHTNESS_BOOST)
    print(f"Brightness boost: {BRIGHTNESS_BOOST}x")

    # Get pixel data
    pixels = list(img.getdata())

    # Convert to 4-bit (0-15 range) and pack 2 pixels per byte
    # EPD47 format: high nibble = first pixel, low nibble = second pixel
    data = []
    for i in range(0, len(pixels), 2):
        p1 = (pixels[i] >> 4) & 0x0F  # Convert 8-bit to 4-bit
        if i + 1 < len(pixels):
            p2 = (pixels[i + 1] >> 4) & 0x0F
        else:
            p2 = 0xF  # Padding with white

        byte = (p1 << 4) | p2
        data.append(byte)

    # Save preview image for debugging
    preview = Image.new('L', (new_width, new_height))
    preview.putdata(pixels)
    preview.save("worldmap_preview.png")
    print(f"Saved preview: worldmap_preview.png")

    # Generate .h file
    with open(OUTPUT_FILE, 'w') as f:
        f.write(f"// World Map bitmap for EPD47 e-paper display\n")
        f.write(f"// Converted from: {INPUT_FILE}\n")
        f.write(f"// Size: {new_width} x {new_height} pixels\n")
        f.write(f"// Format: 4-bit grayscale (2 pixels per byte)\n\n")
        f.write(f"#ifndef WORLDMAP_DATA_H\n")
        f.write(f"#define WORLDMAP_DATA_H\n\n")
        f.write(f"const uint32_t {VAR_NAME}_width = {new_width};\n")
        f.write(f"const uint32_t {VAR_NAME}_height = {new_height};\n")
        f.write(f"const uint8_t {VAR_NAME}_data[{len(data)}] = {{\n")

        # Write data in rows of 38 bytes (like moon.h)
        for i in range(0, len(data), 38):
            row = data[i:i+38]
            hex_str = ", ".join(f"0x{b:02X}" for b in row)
            if i + 38 < len(data):
                f.write(f"  {hex_str},\n")
            else:
                f.write(f"  {hex_str}\n")

        f.write(f"}};\n\n")
        f.write(f"#endif // WORLDMAP_DATA_H\n")

    print(f"Generated: {OUTPUT_FILE}")
    print(f"Data size: {len(data)} bytes")

if __name__ == "__main__":
    convert_image()
