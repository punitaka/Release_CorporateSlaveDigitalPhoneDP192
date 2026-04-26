#include <Wire.h>
#include <U8g2lib.h>
#include "yukkuri_face.h"

// XIAO ESP32C3 の I2C ピン
static const int OLED_SDA = 6;   // D4
static const int OLED_SCL = 7;   // D5

// 1.3インチ 128x64 の 4ピン I2C OLED では SH1106 系が多いため、まずはこちらを使用します。
// 表示されない場合は、下の SSD1306 の行へ切り替えてください。
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int VIRTUAL_WIDTH = 160;
const int VIRTUAL_HEIGHT = 80;
const int IMAGE_POS_X = 16;
const int IMAGE_POS_Y = 8;
const unsigned long FRAME_INTERVAL_MS = 45;
const unsigned long IMAGE_PHASE_MS = 14000;
const unsigned long TEXT_PHASE_MS = 9000;
const char* SCROLL_TEXT = "ゆっくりしていってね";

uint8_t oledAddress = 0;
unsigned long phaseStartMs = 0;
unsigned long lastFrameMs = 0;
bool showImagePhase = true;
int viewX = 0;
int viewY = 0;
int dirX = 1;
int dirY = 1;
int textX = SCREEN_WIDTH;
int textWidth = 0;

uint8_t detectI2CAddress() {
  const uint8_t candidates[] = {0x3C, 0x3D};

  for (uint8_t i = 0; i < sizeof(candidates); i++) {
    Wire.beginTransmission(candidates[i]);
    if (Wire.endTransmission() == 0) {
      return candidates[i];
    }
  }

  return 0;
}

void showErrorScreen() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x12_tr);
  oled.drawStr(0, 14, "OLED not found");
  oled.drawStr(0, 30, "Check wiring:");
  oled.drawStr(0, 44, "SDA=D4(GPIO6)");
  oled.drawStr(0, 58, "SCL=D5(GPIO7)");
  oled.sendBuffer();
}

void resetImagePhase() {
  showImagePhase = true;
  phaseStartMs = millis();
  viewX = 0;
  viewY = 0;
  dirX = 1;
  dirY = 1;
}

void resetTextPhase() {
  showImagePhase = false;
  phaseStartMs = millis();
  oled.setFont(u8g2_font_unifont_t_japanese1);
  textWidth = oled.getUTF8Width(SCROLL_TEXT);
  textX = SCREEN_WIDTH;
}

void updateImageScroll() {
  const int maxViewX = VIRTUAL_WIDTH - SCREEN_WIDTH;
  const int maxViewY = VIRTUAL_HEIGHT - SCREEN_HEIGHT;

  viewX += dirX;
  viewY += dirY;

  if (viewX <= 0 || viewX >= maxViewX) {
    dirX = -dirX;
    viewX += dirX;
  }

  if (viewY <= 0 || viewY >= maxViewY) {
    dirY = -dirY;
    viewY += dirY;
  }
}

void drawImagePhase() {
  oled.clearBuffer();
  oled.drawXBMP(IMAGE_POS_X - viewX, IMAGE_POS_Y - viewY,
                YUKKURI_FACE_WIDTH, YUKKURI_FACE_HEIGHT, YUKKURI_FACE_BITS);
  oled.sendBuffer();
}

void updateTextScroll() {
  textX -= 2;
  if (textX < -textWidth - 12) {
    textX = SCREEN_WIDTH;
  }
}

void drawTextPhase() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_unifont_t_japanese1);
  oled.drawUTF8(textX, 40, SCROLL_TEXT);
  oled.setFont(u8g2_font_6x12_tr);
  oled.drawStr(22, 60, "Yukkuri Shiteitte Ne");
  oled.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);
  oled.begin();
  oled.enableUTF8Print();

  oledAddress = detectI2CAddress();

  Serial.println();
  Serial.println("=== XIAO ESP32C3 OLED Yukkuri Scroll ===");
  Serial.printf("SDA = GPIO%d, SCL = GPIO%d\n", OLED_SDA, OLED_SCL);

  if (oledAddress == 0) {
    Serial.println("OLED was not detected at 0x3C or 0x3D.");
    showErrorScreen();
    return;
  }

  Serial.printf("OLED detected at 0x%02X\n", oledAddress);
  Serial.printf("Bitmap size = %d x %d\n", YUKKURI_FACE_WIDTH, YUKKURI_FACE_HEIGHT);

  resetImagePhase();
  drawImagePhase();
}

void loop() {
  if (oledAddress == 0) {
    showErrorScreen();
    delay(1000);
    return;
  }

  unsigned long now = millis();

  if (showImagePhase) {
    if (now - phaseStartMs >= IMAGE_PHASE_MS) {
      Serial.println("Switch to text scroll phase");
      resetTextPhase();
    }
  } else {
    if (now - phaseStartMs >= TEXT_PHASE_MS) {
      Serial.println("Switch to image scroll phase");
      resetImagePhase();
    }
  }

  if (now - lastFrameMs < FRAME_INTERVAL_MS) {
    return;
  }
  lastFrameMs = now;

  if (showImagePhase) {
    updateImageScroll();
    drawImagePhase();
  } else {
    updateTextScroll();
    drawTextPhase();
  }
}
