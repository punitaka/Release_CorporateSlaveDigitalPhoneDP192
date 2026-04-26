// XIAO ESP32C3 + パッシブブザー
// 修正版: 添付MP3の 35秒〜46秒 を基準にサビ音程を再調査した版
// 対象フレーズ:
// 「シャイニングスター綴れば 夢に眠る幻が 掌に降り注ぐ 新たな世界へ」
//
// 方針:
// - 前回の G 系ではなく、実音源に近い Bb minor / Db major 系の音列へ修正
// - パッシブブザー向けに単音メロディへ簡略化
// - テンポは公式値に近い BPM 158 を採用
//
// 備考:
// - XIAO ESP32C3 の環境で D10 が未定義なら、必要に応じて GPIO 番号へ変更してください。

#ifndef D10
#define D10 10
#endif

const int BUZZER_PIN = D10;
const int BPM = 158;
const float GAP_RATIO = 0.10f;
const int REPEAT_WAIT_MS = 3000;

#define NOTE_REST 0
#define NOTE_BB4 466
#define NOTE_C5  523
#define NOTE_DB5 554
#define NOTE_EB5 622
#define NOTE_F5  698
#define NOTE_GB5 740
#define NOTE_AB5 831

// 1.0 = 4分音符, 0.5 = 8分音符
// 今回の骨格メロディは主に8分音符で構成
const int melody[] = {
  // シャイニングスター綴れば
  NOTE_BB4, NOTE_BB4, NOTE_BB4, NOTE_C5, NOTE_DB5, NOTE_EB5, NOTE_F5, NOTE_EB5, NOTE_DB5, NOTE_C5, NOTE_BB4,

  // 夢に眠る幻が
  NOTE_BB4, NOTE_BB4, NOTE_BB4, NOTE_C5, NOTE_DB5, NOTE_EB5, NOTE_F5, NOTE_EB5, NOTE_DB5, NOTE_C5, NOTE_BB4,

  // 掌に降り注ぐ
  NOTE_BB4, NOTE_BB4, NOTE_BB4, NOTE_C5, NOTE_DB5, NOTE_EB5, NOTE_F5, NOTE_GB5, NOTE_F5, NOTE_EB5, NOTE_DB5, NOTE_C5, NOTE_BB4,

  // 新たな世界へ
  NOTE_BB4, NOTE_BB4, NOTE_BB4, NOTE_C5, NOTE_DB5, NOTE_EB5, NOTE_F5, NOTE_GB5, NOTE_AB5
};

const float noteBeats[] = {
  // シャイニングスター綴れば
  0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 1.0,

  // 夢に眠る幻が
  0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 1.0,

  // 掌に降り注ぐ
  0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 1.0,

  // 新たな世界へ
  0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 2.0
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
  const size_t count = sizeof(melody) / sizeof(melody[0]);
  for (size_t i = 0; i < count; ++i) {
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
