#include <Wire.h>
#include <U8g2lib.h>
#include "yukkuri_face.h"
// #include "yukkuri_full.h"

// XIAO ESP32C3 の I2C ピン
static const int OLED_SDA = 6;   // D4
static const int OLED_SCL = 7;   // D5

// 1.3インチ 128x64 の 4ピン I2C OLED では SH1106 系が多いため、まずはこちらを使用します。
// 表示されない場合は、下の SSD1306 の行へ切り替えてください。
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const unsigned long DRIFT_INTERVAL_MS = 1200;

uint8_t oledAddress = 0;
unsigned long lastDriftMs = 0;
int driftIndex = 0;

const int driftTable[][2] = {
  {0, 0},
  {1, 0},
  {1, 1},
  {0, 1},
  {-1, 1},
  {-1, 0},
  {-1, -1},
  {0, -1},
  {1, -1}
};
const int driftCount = sizeof(driftTable) / sizeof(driftTable[0]);

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

void drawYukkuriImage(int offsetX, int offsetY) {
  const int baseX = (SCREEN_WIDTH - YUKKURI_FACE_WIDTH) / 2;
  const int baseY = (SCREEN_HEIGHT - YUKKURI_FACE_HEIGHT) / 2;

  oled.clearBuffer();
  oled.drawXBMP(baseX + offsetX, baseY + offsetY,
                YUKKURI_FACE_WIDTH, YUKKURI_FACE_HEIGHT, YUKKURI_FACE_BITS);
  oled.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);
  oled.begin();

  oledAddress = detectI2CAddress();

  Serial.println();
  Serial.println("=== XIAO ESP32C3 OLED Yukkuri Image ===");
  Serial.printf("SDA = GPIO%d, SCL = GPIO%d\n", OLED_SDA, OLED_SCL);

  if (oledAddress == 0) {
    Serial.println("OLED was not detected at 0x3C or 0x3D.");
    showErrorScreen();
    return;
  }

  Serial.printf("OLED detected at 0x%02X\n", oledAddress);
  Serial.printf("Bitmap size = %d x %d\n", YUKKURI_FACE_WIDTH, YUKKURI_FACE_HEIGHT);

  drawYukkuriImage(0, 0);
}

void loop() {
  if (oledAddress == 0) {
    showErrorScreen();
    delay(1000);
    return;
  }

  unsigned long now = millis();
  if (now - lastDriftMs >= DRIFT_INTERVAL_MS) {
    lastDriftMs = now;
    driftIndex = (driftIndex + 1) % driftCount;

    const int offsetX = driftTable[driftIndex][0];
    const int offsetY = driftTable[driftIndex][1];
    drawYukkuriImage(offsetX, offsetY);
  }
}
