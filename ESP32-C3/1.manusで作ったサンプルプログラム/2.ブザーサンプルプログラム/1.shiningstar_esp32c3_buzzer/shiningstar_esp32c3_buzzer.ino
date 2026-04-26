// XIAO ESP32C3 + パッシブブザー
// 対象フレーズ:
// 「シャイニングスター 綴れば 夢に眠る幻が 掌に降り注ぐ 新たな世界へ」
//
// 参考:
// - ユーザー添付の簡易サンプル
// - 公式ページ記載 BPM = 158
// - 楽譜付き演奏動画の解析結果をもとに主旋律をブザー向けへ単音化
//
// 注意:
// - 今回はパッシブブザー向けに単音メロディへ簡略化しています。
// - XIAO ESP32C3 の Arduino 環境で D10 が未定義の場合は、BUZZER_PIN を実GPIO番号へ変更してください。

#ifndef D10
#define D10 10
#endif

const int BUZZER_PIN = D10;
const int BPM = 158;
const float GAP_RATIO = 0.12f;   // 各音の後ろに少しだけ隙間を入れる
const int REPEAT_WAIT_MS = 2500; // 1回演奏後の待機

// 音名定義
#define NOTE_REST 0
#define NOTE_FS5  740
#define NOTE_G5   784
#define NOTE_A5   880
#define NOTE_B5   988
#define NOTE_C6  1047
#define NOTE_D6  1175

// 1拍 = 4分音符
// 0.5 = 8分音符, 1.0 = 4分音符, 2.0 = 2分音符
const int melody[] = {
  // シャイニングスター
  NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5, NOTE_FS5, NOTE_G5,
  // 綴れば
  NOTE_G5, NOTE_A5, NOTE_B5,

  // 夢に眠る
  NOTE_B5, NOTE_B5, NOTE_B5, NOTE_B5, NOTE_A5, NOTE_B5,
  // 幻が
  NOTE_B5, NOTE_C6, NOTE_D6,

  // 掌に
  NOTE_D6, NOTE_D6, NOTE_D6, NOTE_D6, NOTE_C6, NOTE_B5,
  // 降り注ぐ
  NOTE_A5, NOTE_A5, NOTE_G5, NOTE_FS5,

  // 新たな
  NOTE_G5, NOTE_A5, NOTE_B5, NOTE_A5, NOTE_G5,
  // 世界へ
  NOTE_FS5, NOTE_G5
};

const float noteBeats[] = {
  // シャイニングスター
  0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
  // 綴れば
  0.5, 0.5, 1.0,

  // 夢に眠る
  0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
  // 幻が
  0.5, 0.5, 1.0,

  // 掌に
  0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
  // 降り注ぐ
  0.5, 0.5, 0.5, 0.5,

  // 新たな
  0.5, 0.5, 0.5, 0.5, 0.5,
  // 世界へ
  0.5, 2.0
};

static_assert(sizeof(melody) / sizeof(melody[0]) == sizeof(noteBeats) / sizeof(noteBeats[0]),
              "melody と noteBeats の要素数が一致していません");

int beatToMs(float beats) {
  const float quarterMs = 60000.0f / BPM;
  return (int)(quarterMs * beats);
}

void playNote(int freq, float beats) {
  int fullLength = beatToMs(beats);
  int soundLength = (int)(fullLength * (1.0f - GAP_RATIO));
  int gapLength = fullLength - soundLength;

  if (freq > 0) {
    tone(BUZZER_PIN, freq, soundLength);
    delay(soundLength);
    noTone(BUZZER_PIN);
  } else {
    delay(soundLength);
  }

  delay(gapLength);
}

void playMelody() {
  const size_t noteCount = sizeof(melody) / sizeof(melody[0]);
  for (size_t i = 0; i < noteCount; ++i) {
    playNote(melody[i], noteBeats[i]);
  }
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  delay(1000);
}

void loop() {
  playMelody();
  noTone(BUZZER_PIN);
  delay(REPEAT_WAIT_MS);
}
