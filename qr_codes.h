// QR Code for Weather Station Display
// Pre-generated QR codes - no external library needed

#ifndef QR_CODES_H
#define QR_CODES_H

// QR Code structure
typedef struct {
  const uint8_t* data;
  int size;
  const char* label;
} QRCodeDef;

// QR Code: xe1e.net
// Version 1 (21x21), ECC Level M
// Generated from api.qrserver.com
// Binary representation: 1=black, 0=white
// Each row is 21 bits, stored in 3 bytes (24 bits, rightmost 3 unused)
const uint8_t QR_XE1E_DATA[] = {
  0xFE, 0x73, 0xF8,  // Row 0
  0x82, 0x6A, 0x08,  // Row 1
  0xBA, 0x92, 0xE8,  // Row 2
  0xBA, 0xAA, 0xE8,  // Row 3
  0xBA, 0xFA, 0xE8,  // Row 4
  0x82, 0xAA, 0x08,  // Row 5
  0xFE, 0xAB, 0xF8,  // Row 6
  0x01, 0x30, 0x00,  // Row 7
  0xBE, 0x2B, 0xE0,  // Row 8
  0xB5, 0x4C, 0xE8,  // Row 9
  0x6A, 0xD0, 0x70,  // Row 10
  0xC8, 0x80, 0x20,  // Row 11
  0xB7, 0xB0, 0x48,  // Row 12
  0x01, 0xFF, 0xB8,  // Row 13
  0xFE, 0x2A, 0x70,  // Row 14
  0x82, 0xDF, 0xC0,  // Row 15
  0xBA, 0xC8, 0x88,  // Row 16
  0xBA, 0xC9, 0xE0,  // Row 17
  0xBA, 0xD4, 0x60,  // Row 18
  0x82, 0x41, 0xA0,  // Row 19
  0xFE, 0xF4, 0x90,  // Row 20
};

const QRCodeDef QR_XE1E = {
  QR_XE1E_DATA,
  21,
  "xe1e.net"
};

// Draw QR code from pre-generated data
void drawQRCode(const QRCodeDef& qr, int x, int y, int moduleSize) {
  int margin = moduleSize * 2;
  int totalSize = qr.size * moduleSize;

  // Draw white background with quiet zone
  fillRect(x - margin, y - margin, totalSize + margin * 2, totalSize + margin * 2, White);

  // Draw QR modules
  for (int row = 0; row < qr.size; row++) {
    for (int col = 0; col < qr.size; col++) {
      int byteIndex = row * 3 + (col / 8);
      int bitIndex = 7 - (col % 8);
      if (qr.data[byteIndex] & (1 << bitIndex)) {
        fillRect(x + col * moduleSize, y + row * moduleSize, moduleSize, moduleSize, Black);
      }
    }
  }
}

#endif // QR_CODES_H
