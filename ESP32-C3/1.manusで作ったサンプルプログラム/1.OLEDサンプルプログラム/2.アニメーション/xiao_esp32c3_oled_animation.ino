#include <Wire.h>
#include <U8g2lib.h>

// XIAO ESP32C3 の I2C ピン
static const int OLED_SDA = 6;   // D4
static const int OLED_SCL = 7;   // D5

// 1.3インチ 128x64 の 4ピン I2C OLED では SH1106 系が多いため、まずはこちらを使用します。
// 表示されない場合は、下の SSD1306 の行へ切り替えてください。
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

const uint8_t SCREEN_WIDTH = 128;
const uint8_t SCREEN_HEIGHT = 64;
const uint8_t STAR_COUNT = 12;

struct Star {
  int x;
  int y;
  int speed;
};

Star stars[STAR_COUNT];
uint8_t oledAddress = 0;
unsigned long sceneStartMs = 0;
int currentScene = 0;

int ballX = 20;
int ballY = 20;
int ballVX = 2;
int ballVY = 2;
const int ballRadius = 6;

int textX = SCREEN_WIDTH;
const char* scrollMessage = "HELLO XIAO ESP32C3";

int shipX = 16;
int shipY = SCREEN_HEIGHT / 2;
int shipDir = 1;

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

void initStars() {
  for (uint8_t i = 0; i < STAR_COUNT; i++) {
    stars[i].x = random(0, SCREEN_WIDTH);
    stars[i].y = random(0, SCREEN_HEIGHT);
    stars[i].speed = random(1, 4);
  }
}

void resetScene(int sceneId) {
  currentScene = sceneId;
  sceneStartMs = millis();

  if (currentScene == 0) {
    ballX = 20;
    ballY = 20;
    ballVX = 2;
    ballVY = 2;
  } else if (currentScene == 1) {
    textX = SCREEN_WIDTH;
  } else {
    shipX = 16;
    shipY = SCREEN_HEIGHT / 2;
    shipDir = 1;
    initStars();
  }
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

void drawStatusBar(const char* label) {
  char addrText[20];
  snprintf(addrText, sizeof(addrText), "0x%02X", oledAddress);

  oled.setDrawColor(1);
  oled.drawBox(0, 0, SCREEN_WIDTH, 11);
  oled.setDrawColor(0);
  oled.setFont(u8g2_font_5x7_tr);
  oled.drawStr(2, 8, label);
  oled.drawStr(98, 8, addrText);
  oled.setDrawColor(1);
}

void drawBouncingBallScene() {
  ballX += ballVX;
  ballY += ballVY;

  if (ballX <= ballRadius || ballX >= SCREEN_WIDTH - ballRadius) {
    ballVX = -ballVX;
  }
  if (ballY <= 11 + ballRadius || ballY >= SCREEN_HEIGHT - ballRadius) {
    ballVY = -ballVY;
  }

  oled.clearBuffer();
  drawStatusBar("SCENE 1 BALL");
  oled.drawFrame(0, 11, SCREEN_WIDTH, SCREEN_HEIGHT - 11);
  oled.drawDisc(ballX, ballY, ballRadius);
  oled.drawCircle(ballX, ballY, ballRadius + 3);
  oled.sendBuffer();
}

void drawScrollingTextScene() {
  oled.clearBuffer();
  drawStatusBar("SCENE 2 TEXT");

  oled.setFont(u8g2_font_logisoso18_tr);
  oled.drawStr(textX, 38, "OLED");
  oled.setFont(u8g2_font_6x12_tr);
  oled.drawStr(textX + 6, 56, scrollMessage);
  oled.drawLine(0, 42, SCREEN_WIDTH - 1, 42);

  int textWidth = oled.getStrWidth("OLED");
  textX -= 3;
  if (textX < -textWidth - 70) {
    textX = SCREEN_WIDTH;
  }

  oled.sendBuffer();
}

void updateStars() {
  for (uint8_t i = 0; i < STAR_COUNT; i++) {
    stars[i].x -= stars[i].speed;
    if (stars[i].x < 0) {
      stars[i].x = SCREEN_WIDTH - 1;
      stars[i].y = random(12, SCREEN_HEIGHT);
      stars[i].speed = random(1, 4);
    }
  }
}

void drawShip(int x, int y) {
  oled.drawTriangle(x, y, x, y + 10, x + 18, y + 5);
  oled.drawLine(x + 3, y + 5, x + 18, y + 5);
  oled.drawPixel(x + 14, y + 3);
  oled.drawPixel(x + 14, y + 7);
}

void drawSpaceScene() {
  updateStars();

  shipY += shipDir;
  if (shipY < 16 || shipY > 48) {
    shipDir = -shipDir;
  }

  oled.clearBuffer();
  drawStatusBar("SCENE 3 SPACE");

  for (uint8_t i = 0; i < STAR_COUNT; i++) {
    oled.drawPixel(stars[i].x, stars[i].y);
    if (stars[i].x > 1) {
      oled.drawPixel(stars[i].x - 1, stars[i].y);
    }
  }

  drawShip(shipX, shipY);
  oled.drawStr(40, 58, "FLYING...");
  oled.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);
  oled.begin();

  randomSeed(micros());

  oledAddress = detectI2CAddress();

  Serial.println();
  Serial.println("=== XIAO ESP32C3 OLED Animation ===");
  Serial.printf("SDA = GPIO%d, SCL = GPIO%d\n", OLED_SDA, OLED_SCL);

  if (oledAddress == 0) {
    Serial.println("OLED was not detected at 0x3C or 0x3D.");
    showErrorScreen();
    return;
  }

  Serial.printf("OLED detected at 0x%02X\n", oledAddress);

  oled.clearBuffer();
  oled.setFont(u8g2_font_ncenB08_tr);
  oled.drawStr(10, 20, "ANIMATION OK");
  oled.setFont(u8g2_font_6x12_tr);
  oled.drawStr(8, 40, "XIAO ESP32C3");
  oled.drawStr(8, 56, "Starting...");
  oled.sendBuffer();
  delay(1200);

  resetScene(0);
}

void loop() {
  if (oledAddress == 0) {
    showErrorScreen();
    delay(1000);
    return;
  }

  unsigned long elapsed = millis() - sceneStartMs;
  if (elapsed > 5000) {
    resetScene((currentScene + 1) % 3);
    Serial.printf("Scene changed to %d\n", currentScene + 1);
  }

  if (currentScene == 0) {
    drawBouncingBallScene();
  } else if (currentScene == 1) {
    drawScrollingTextScene();
  } else {
    drawSpaceScene();
  }

  delay(40);
}
