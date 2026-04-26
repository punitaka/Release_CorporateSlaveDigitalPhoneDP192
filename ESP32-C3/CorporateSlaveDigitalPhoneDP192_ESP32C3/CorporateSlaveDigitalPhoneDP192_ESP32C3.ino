/*
  Exchange Online Alert OLED & Buzzer Controller for XIAO ESP32-C3
  Exchange Online REST API と OLED / Passive Buzzer 制御プログラム

  機能：
  - Wi-Fi接続
  - Exchange Online REST APIをポーリング
  - 前回のチェック時刻以降に新たに受信したメールをチェック
  - OLED表示をON（3分間継続）
  - ブザーでメロディを再生（30秒継続）
  - 自動的に表示OFF
  - メールを既読にしない（複数台の機材で同じメールを処理可能）

  対応ボード：Seeed Studio XIAO ESP32C3
  OLED接続：I2C（SDA=GPIO6 / SCL=GPIO7）
  ブザー接続：D10（GPIO10）

  必要なライブラリ：
  - WiFi.h（ESP32標準）
  - HTTPClient.h（ESP32標準）
  - ArduinoJson.h（Arduino JSON Library）
  - Wire.h（Arduino標準）
  - U8g2lib.h（OLED表示用）
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <time.h>
#include "yukkuri_face.h"

#ifndef D10
#define D10 10
#endif

// XIAO ESP32C3 の I2C ピン
static const int OLED_SDA = 6;   // D4
static const int OLED_SCL = 7;   // D5
static const int BUZZER_PIN = D10;

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
const unsigned long IMAGE_PHASE_MS = 6000;
const unsigned long TEXT_PHASE_MS = 7200;
const char* SCROLL_TEXT = "ゆっくりしていってね!!!";

// 曲全体の音符部分（ユーザー提供のMIDI基準メロディを周波数へ変換）
int alert_melody[] = {
  698, 740, 831, 831, 831, 831, 831, 554, 554, 554, 831, 740,
  698, 740, 622, 1047, 1047, 932, 1047, 1109, 932, 1047, 1109, 1047,
  932, 932, 1047, 1047, 1245, 1109, 698, 740, 740, 831, 932, 880,
  880, 740, 698, 622, 698, 831, 1109, 932, 1109, 1047, 1047, 1109
};

// 各音符の持続時間（1.0 = 4分音符, 0.5 = 8分音符）
float alert_duration[] = {
  0.5, 0.5, 1.0, 0.5, 1.0, 1.0, 1.0, 0.5, 1.5, 0.5, 0.5, 0.5,
  1.0, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 1.5, 0.5, 0.5, 1.0, 0.5,
  1.0, 1.0, 1.0, 0.5, 1.0, 1.5, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0,
  0.5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 2.0, 1.0, 0.5, 5.0
};

static_assert(sizeof(alert_melody) / sizeof(alert_melody[0]) == sizeof(alert_duration) / sizeof(alert_duration[0]),
              "alert_melody と alert_duration の要素数が一致していません");

// ===== 設定 =====
const char* SSID = "★ここにWifiのSSIDを設定★";              // Wi-Fi SSID（設定してください）
const char* PASSWORD = "★ここにWifiのパスワードを設定★";          // Wi-Fi パスワード（設定してください）

// Microsoft Entra ID認証情報
const char* TENANT_ID = "★ここにExchangeのテナントIDを設定★";         // テナントID（設定してください）
const char* CLIENT_ID = "★ここにExchangeのクライアントIDを設定★";         // クライアントID（設定してください）
const char* CLIENT_SECRET = "★ここにExchangeのクライアントシークレットを設定★";     // クライアントシークレット（設定してください）
const char* MAILBOX_ADDRESS = "xxxxx@XXXXX.XXXXX"; // 監視対象メールアドレス（設定してください）

const int CHECK_INTERVAL = 300000;            // メール受信チェック間隔（ミリ秒）：5分 = 300000ms
const int OLED_ON_DURATION = 180000;          // OLED表示継続時間（ミリ秒）：3分 = 180000ms
const int BUZZER_DURATION = 42000;            // ブザー鳴動時間（ミリ秒）：45秒 = 30000ms
//const int CHECK_INTERVAL = 30000;          // メール受信チェック間隔（ミリ秒）：30秒 = 30000ms
//const int OLED_ON_DURATION = 30000;        // OLED表示継続時間（ミリ秒）：30秒 = 30000ms
//const int BUZZER_DURATION = 15000;         // ブザー鳴動時間（ミリ秒）：15秒 = 15000ms

const float BPM = 158.000764f;               // ユーザー提供MIDI基準のテンポ
const float NOTE_GAP_RATIO = 0.08f;           // 各音符の間に入れる無音比率
const int REPEAT_WAIT_MS = 3000;              // 1フレーズ再生後の待機時間（ミリ秒）
const char* ALERT_KEYWORD = "";              // 検出キーワード
const int STARTUP_DELAY = 10000;              // 起動直後のスリープ時間（ミリ秒）：10秒 = 10000ms
const int STARTUP_PREVIEW_MS = 10000;     // 起動直後のOLED表示確認時間（ミリ秒）：10秒

// ===== グローバル変数 =====
unsigned long oled_on_time = 0;               // OLED表示開始時刻
bool oled_active = false;                     // OLEDアクティブフラグ
unsigned long buzzer_on_time = 0;             // ブザーON開始時刻
bool buzzer_active = false;                   // ブザーアクティブフラグ
bool startup_complete = false;                // 起動完了フラグ
unsigned long last_check_time = 0;            // 最後にメール受信をチェックした時刻
unsigned long last_oled_check_time = 0;       // 最後にOLED停止条件をチェックした時刻
String access_token = "";                     // アクセストークン
unsigned long token_expiry_time = 0;          // トークン有効期限
time_t last_email_check_timestamp = 0;        // 前回のメール受信チェック時刻（Unix timestamp）
int melody_index = 0;                         // メロディ再生位置
bool note_in_gap = false;                     // ブザー音符の無音区間フラグ
bool melody_waiting_restart = false;          // 1フレーズ再生後の待機フラグ
unsigned long note_phase_start = 0;           // 現在の音符または無音区間の開始時刻
unsigned long repeat_wait_start = 0;          // フレーズ再生後の待機開始時刻
unsigned long current_sound_ms = 0;           // 現在の音符の発音時間
unsigned long current_gap_ms = 0;             // 現在の音符の無音時間

uint8_t oledAddress = 0;
bool oled_available = false;
unsigned long phaseStartMs = 0;
unsigned long lastFrameMs = 0;
bool showImagePhase = true;
int viewX = 0;
int viewY = 0;
int dirX = 1;
int dirY = 1;
int textX = SCREEN_WIDTH;
int textWidth = 0;

const int alert_melody_length = sizeof(alert_melody) / sizeof(alert_melody[0]);

// ===== 関数プロトタイプ =====
void setup_wifi();
void setup_oled_and_buzzer();
void setup_oled();
uint8_t detectI2CAddress();
void clear_oled();
void show_oled_error_screen();
void resetImagePhase();
void resetTextPhase();
void updateImageScroll();
void drawImagePhase();
void updateTextScroll();
void drawTextPhase();
void update_oled_animation();
void set_oled_alert(bool state);
void start_buzzer();
void stop_buzzer();
int beatToMs(float beats);
void start_next_note(unsigned long now);
void manage_buzzer();
void manage_oled_timer();
void check_emails();
bool get_access_token();
String make_graph_api_request(String endpoint);
bool contains_keyword(String text, String keyword);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("============================================================");
  Serial.println("Exchange Online Alert OLED & Buzzer Controller");
  Serial.println("for XIAO ESP32-C3");
  Serial.println("============================================================");
  Serial.print("Mailbox: ");
  Serial.println(MAILBOX_ADDRESS);
  Serial.print("Alert Keyword: ");
  Serial.println(ALERT_KEYWORD);
  Serial.println("============================================================");

  // OLED・ブザー設定
  setup_oled_and_buzzer();

  // Wi-Fi接続
  setup_wifi();

  // 時刻同期（NTP）
  Serial.println("NTPサーバーから時刻を同期しています...");
  configTime(9 * 3600, 0, "ntp.nict.jp", "time.google.com");
  delay(2000);

  // 起動直後のスリープ
  Serial.println("起動直後の初期化スリープを開始します（10秒）");
  delay(STARTUP_DELAY);
  startup_complete = true;
  Serial.println("起動直後の初期化スリープが完了しました。通常動作を開始します");

  // 起動直後にOLEDとブザーを10秒稼働
  Serial.println("起動直後にOLEDとブザーを10秒稼働");
  set_oled_alert(true);
  start_buzzer();
  unsigned long preview_start = millis();
  while (millis() - preview_start < STARTUP_PREVIEW_MS) {
    update_oled_animation();
    manage_buzzer();
    delay(5);
  }
  set_oled_alert(false);
  stop_buzzer();

  last_check_time = millis();
  last_oled_check_time = millis();

  // 現在時刻を取得（初回チェック用） 2分前にします
  time_t now = time(nullptr);

  // ★ NTP同期確認ログ
  Serial.print("time(nullptr) の値: ");
  Serial.println((long)now);
  if (now < 100000) {
    Serial.println("警告: NTP同期が完了していません（時刻が不正です）");
  } else {
    Serial.println("NTP同期OK: 時刻は正常です");
  }

  last_email_check_timestamp = now - (2 * 60);
  Serial.print("last_email_check_timestamp の値: ");
  Serial.println((long)last_email_check_timestamp);

  // 起動直後にまずメールチェック
  check_emails();
}

void loop() {
  unsigned long current_time = millis();

  manage_buzzer();

  // OLEDタイマー管理と描画更新
  if (current_time - last_oled_check_time >= 50) {
    manage_oled_timer();
    last_oled_check_time = current_time;
  }

  // メール受信チェック（5分ごと）
  if (current_time - last_check_time >= CHECK_INTERVAL) {
    check_emails();
    last_check_time = millis();
  }

  delay(5); // CPU負荷軽減
}

// ===== Wi-Fi設定 =====
void setup_wifi() {
  Serial.print("Wi-Fi接続中: ");
  Serial.println(SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi接続成功");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWi-Fi接続失敗");
  }
}

// ===== OLED・ブザー設定 =====
void setup_oled_and_buzzer() {
  setup_oled();

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  Serial.print("GPIO ");
  Serial.print(BUZZER_PIN);
  Serial.println(" (D10) を初期化しました（ブザーOFF状態）");
}

void setup_oled() {
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);

  oledAddress = detectI2CAddress();
  if (oledAddress == 0) {
    Serial.println("OLED was not detected at 0x3C or 0x3D.");
    oled_available = false;
    return;
  }

  oled.setI2CAddress(oledAddress << 1);
  oled.begin();
  oled.enableUTF8Print();
  oled_available = true;

  Serial.println("=== XIAO ESP32C3 OLED Yukkuri Scroll ===");
  Serial.printf("SDA = GPIO%d, SCL = GPIO%d\n", OLED_SDA, OLED_SCL);
  Serial.printf("OLED detected at 0x%02X\n", oledAddress);
  Serial.printf("Bitmap size = %d x %d\n", YUKKURI_FACE_WIDTH, YUKKURI_FACE_HEIGHT);

  clear_oled();
}

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

void clear_oled() {
  if (!oled_available) {
    return;
  }

  oled.clearBuffer();
  oled.sendBuffer();
}

void show_oled_error_screen() {
  if (!oled_available) {
    return;
  }

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
  if (!oled_available) {
    return;
  }

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
  if (!oled_available) {
    return;
  }

  oled.clearBuffer();
  oled.setFont(u8g2_font_unifont_t_japanese1);
  oled.drawUTF8(textX, 30, SCROLL_TEXT);
  oled.setFont(u8g2_font_lubBI12_te);
  oled.drawUTF8(28, 55, "O K I R O");
  oled.sendBuffer();
}

void update_oled_animation() {
  if (!oled_active || !oled_available) {
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

// ===== OLED制御 =====
void set_oled_alert(bool state) {
  if (state) {
    oled_active = true;
    oled_on_time = millis();
    resetImagePhase();
    lastFrameMs = 0;
    Serial.println("OLED表示をONにしました");
    if (oled_available) {
      drawImagePhase();
    } else {
      Serial.println("OLEDが未検出のため表示は行いません");
    }
  } else {
    oled_active = false;
    oled_on_time = 0;
    clear_oled();
    Serial.println("OLED表示をOFFにしました");
  }
}

// ===== ブザー制御 =====
void start_buzzer() {
  buzzer_active = true;
  buzzer_on_time = millis();
  melody_index = 0;
  note_in_gap = false;
  melody_waiting_restart = false;
  note_phase_start = 0;
  repeat_wait_start = 0;
  current_sound_ms = 0;
  current_gap_ms = 0;
  Serial.println("ブザーを起動しました");
}

void stop_buzzer() {
  noTone(BUZZER_PIN);
  buzzer_active = false;
  melody_index = 0;
  note_in_gap = false;
  melody_waiting_restart = false;
  note_phase_start = 0;
  repeat_wait_start = 0;
  current_sound_ms = 0;
  current_gap_ms = 0;
  Serial.println("ブザー鳴動を停止しました");
}

int beatToMs(float beats) {
  const float quarterMs = 60000.0f / BPM;
  return (int)(quarterMs * beats + 0.5f);
}

void start_next_note(unsigned long now) {
  int total_ms = beatToMs(alert_duration[melody_index]);
  current_sound_ms = (unsigned long)(total_ms * (1.0f - NOTE_GAP_RATIO));
  current_gap_ms = (unsigned long)(total_ms - current_sound_ms);
  note_phase_start = now;
  note_in_gap = false;
  melody_waiting_restart = false;
  repeat_wait_start = 0;

  if (alert_melody[melody_index] > 0) {
    tone(BUZZER_PIN, alert_melody[melody_index]);  // 停止はmanage_buzzer()内のnoTone()に任せる
  } else {
    noTone(BUZZER_PIN);
  }
}

void manage_buzzer() {
  if (!buzzer_active) {
    return;
  }

  unsigned long now = millis();
  if (now - buzzer_on_time >= BUZZER_DURATION) {
    Serial.println("ブザー鳴動時間終了");
    stop_buzzer();
    return;
  }

  if (melody_waiting_restart) {
    if (now - repeat_wait_start >= REPEAT_WAIT_MS) {
      melody_index = 0;
      start_next_note(now);
    }
    return;
  }

  if (note_phase_start == 0) {
    start_next_note(now);
    return;
  }

  if (!note_in_gap) {
    if (now - note_phase_start >= current_sound_ms) {
      noTone(BUZZER_PIN);
      note_in_gap = true;
      note_phase_start = now;
    }
    return;
  }

  if (now - note_phase_start >= current_gap_ms) {
    melody_index++;
    if (melody_index >= alert_melody_length) {
      noTone(BUZZER_PIN);
      melody_index = 0;
      note_phase_start = 0;
      note_in_gap = false;
      melody_waiting_restart = true;
      repeat_wait_start = now;
      return;
    }
    start_next_note(now);
  }
}

// ===== OLEDタイマー管理 =====
void manage_oled_timer() {
  if (!oled_active || oled_on_time == 0 || !startup_complete) {
    return;
  }

  unsigned long elapsed_time = millis() - oled_on_time;
  if (elapsed_time >= OLED_ON_DURATION) {
    Serial.print("OLED表示時間（");
    Serial.print(OLED_ON_DURATION / 1000);
    Serial.println("秒）に達しました");
    set_oled_alert(false);
    return;
  }

  update_oled_animation();
}

// ===== メール受信チェック =====
void check_emails() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fiが接続されていません");
    return;
  }

  // OLEDの中央に「メールチェック中」を1秒間表示
  if (oled_available) {
    const char* checking_msg = "メールチェック中";
    oled.clearBuffer();
    oled.setFont(u8g2_font_b10_t_japanese1);  // 小さい日本語フォント（文字高さ約10px）
    int msg_width = oled.getUTF8Width(checking_msg);
    int msg_x = (SCREEN_WIDTH - msg_width) / 2;
    int msg_y = (SCREEN_HEIGHT / 2) + 5;  // フォント高さ約10pxを考慮して中央揃え
    oled.drawUTF8(msg_x, msg_y, checking_msg);
    oled.sendBuffer();
    delay(2000);
    if (!oled_active) {
      clear_oled();
    }
  }

  Serial.println("--- メール受信チェック開始 ---");

  // 前回のチェック時刻以降に受信したメールをフィルタ
  struct tm* timeinfo = gmtime(&last_email_check_timestamp);
  char timestamp_buffer[30];
  strftime(timestamp_buffer, sizeof(timestamp_buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);

  String filter = "receivedDateTime%20gt%20";
  filter += timestamp_buffer;

  // エンドポイント構築
  String endpoint = "/users/";
  endpoint += MAILBOX_ADDRESS;
  endpoint += "/messages?%24filter=";
  endpoint += filter;
  endpoint += "&%24select=subject,bodyPreview,from,receivedDateTime&%24top=10&%24orderby=receivedDateTime%20desc";

  String response = make_graph_api_request(endpoint);

  if (response.length() == 0) {
    Serial.println("エラー: メール取得失敗");
    Serial.println("--- メール受信チェック終了 ---\n");
    return;
  }

  // JSON解析
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.println("JSON解析エラー");
    Serial.println("--- メール受信チェック終了 ---\n");
    return;
  }

  JsonArray messages = doc["value"].as<JsonArray>();

  if (messages.size() == 0) {
    Serial.println("新しいメールはありません");
    last_email_check_timestamp = time(nullptr);
    Serial.println("--- メール受信チェック終了 ---\n");
    return;
  }

  Serial.print(messages.size());
  Serial.println("件の新しいメールを検出しました");

  for (JsonObject message : messages) {
    String subject = message["subject"].as<String>();
    String bodyPreview = message["bodyPreview"].as<String>();
    String from = message["from"]["emailAddress"]["address"].as<String>();
    String receivedDateTime = message["receivedDateTime"].as<String>();

    Serial.print("メール受信 - From: ");
    Serial.print(from);
    Serial.print(", Subject: ");
    Serial.print(subject);
    Serial.print(", ReceivedTime: ");
    Serial.println(receivedDateTime);

    bool should_activate_alert = false;

    if (ALERT_KEYWORD[0] == '\0') {
      should_activate_alert = true;
      Serial.print("メールを検出しました（キーワード未設定）: ");
      Serial.println(subject);
    } else {
      if (contains_keyword(subject, ALERT_KEYWORD) || contains_keyword(bodyPreview, ALERT_KEYWORD)) {
        should_activate_alert = true;
        Serial.print("ALERTメールを検出しました: ");
        Serial.println(subject);
      }
    }

    if (should_activate_alert) {
      if (!oled_active && startup_complete) {
        set_oled_alert(true);
        Serial.print("OLED表示開始時刻: ");
        Serial.println(oled_on_time);
        start_buzzer();
      } else if (oled_active) {
        Serial.println("OLEDは既に表示中です。タイマーはリセットしません");
      }
    }
  }

  last_email_check_timestamp = time(nullptr);
  Serial.println("--- メール受信チェック終了 ---\n");
}

// ===== アクセストークン取得 =====
bool get_access_token() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fiが接続されていません");
    return false;
  }

  // トークンがまだ有効な場合はスキップ
  if (access_token.length() > 0 && millis() < token_expiry_time) {
    Serial.println("既存のアクセストークンを使用します");
    return true;
  }

  HTTPClient http;
  String url = "https://login.microsoftonline.com/";
  url += TENANT_ID;
  url += "/oauth2/v2.0/token";

  String payload = "client_id=";
  payload += CLIENT_ID;
  payload += "&scope=https://graph.microsoft.com/.default";
  payload += "&client_secret=";
  payload += CLIENT_SECRET;
  payload += "&grant_type=client_credentials";

  Serial.println("アクセストークンを取得しています...");

  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(payload);

  if (httpCode == 200) {
    String response = http.getString();

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      access_token = doc["access_token"].as<String>();
      int expires_in = doc["expires_in"].as<int>();
      token_expiry_time = millis() + (expires_in * 1000) - 60000;
      Serial.println("アクセストークンを取得しました");
      http.end();
      return true;
    } else {
      Serial.println("JSON解析エラー");
      http.end();
      return false;
    }
  } else {
    Serial.print("トークン取得エラー: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

// ===== Graph API呼び出し =====
String make_graph_api_request(String endpoint) {
  if (!get_access_token()) {
    return "";
  }

  HTTPClient http;
  String url = "https://graph.microsoft.com/v1.0";
  url += endpoint;

  Serial.print("リクエストURL: ");
  Serial.println(url);

  http.begin(url);
  http.addHeader("Authorization", "Bearer " + access_token);

  int httpCode = http.GET();

  if (httpCode == 200) {
    String response = http.getString();
    http.end();
    return response;
  } else {
    String errorResponse = http.getString();
    Serial.print("Graph API エラー: ");
    Serial.println(httpCode);
    Serial.print("エラーレスポンス: ");
    Serial.println(errorResponse);
    http.end();
    return "";
  }
}

// ===== キーワード検索 =====
bool contains_keyword(String text, String keyword) {
  if (text.length() == 0 || keyword.length() == 0) {
    return false;
  }

  String text_upper = text;
  String keyword_upper = keyword;

  text_upper.toUpperCase();
  keyword_upper.toUpperCase();

  return text_upper.indexOf(keyword_upper) >= 0;
}
