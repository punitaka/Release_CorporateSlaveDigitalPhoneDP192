#include <Wire.h>
#include <U8g2lib.h>

// XIAO ESP32C3 の I2C ピン
static const int OLED_SDA = 6;   // D4
static const int OLED_SCL = 7;   // D5

// 1.3インチ 128x64 の 4ピン I2C OLED では SH1106 系が多いため、まずはこちらを使用します。
// もし何も表示されない場合は、下の SH1106 の行をコメントアウトし、
// SSD1306 の行を有効化して試してください。
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

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

void drawFrame(uint8_t address, int counter) {
  char line1[32];
  char line2[32];
  char line3[32];

  snprintf(line1, sizeof(line1), "XIAO ESP32C3");
  snprintf(line2, sizeof(line2), "OLED addr: 0x%02X", address);
  snprintf(line3, sizeof(line3), "Count: %d", counter);

  oled.clearBuffer();

  oled.setFont(u8g2_font_ncenB08_tr);
  oled.drawStr(0, 14, line1);
  oled.drawLine(0, 18, 127, 18);

  oled.setFont(u8g2_font_6x12_tr);
  oled.drawStr(0, 34, line2);
  oled.drawStr(0, 50, line3);

  oled.drawFrame(96, 26, 28, 28);
  oled.drawDisc(110, 40, 8);

  oled.sendBuffer();
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

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);

  oled.begin();

  uint8_t address = detectI2CAddress();

  Serial.println();
  Serial.println("=== XIAO ESP32C3 OLED Sample ===");
  Serial.printf("SDA = GPIO%d, SCL = GPIO%d\n", OLED_SDA, OLED_SCL);

  if (address == 0) {
    Serial.println("OLED was not detected at 0x3C or 0x3D.");
    showErrorScreen();
    return;
  }

  Serial.printf("OLED detected at 0x%02X\n", address);

  oled.clearBuffer();
  oled.setFont(u8g2_font_ncenB08_tr);
  oled.drawStr(8, 18, "OLED START OK");
  oled.setFont(u8g2_font_6x12_tr);
  oled.drawStr(0, 40, "XIAO ESP32C3");
  oled.drawStr(0, 56, "Display ready");
  oled.sendBuffer();
  delay(1500);
}

void loop() {
  static int counter = 0;
  static uint8_t address = detectI2CAddress();

  if (address == 0) {
    Serial.println("OLED connection error");
    showErrorScreen();
    delay(1000);
    return;
  }

  drawFrame(address, counter);
  Serial.printf("Display update: %d\n", counter);

  counter++;
  delay(1000);
}
