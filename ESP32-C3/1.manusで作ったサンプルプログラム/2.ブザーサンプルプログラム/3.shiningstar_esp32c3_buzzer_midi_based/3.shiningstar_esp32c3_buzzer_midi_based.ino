// XIAO ESP32C3 + パッシブブザー
// MIDI基準の修正版
// 参照: [hev]シャイニングスター_short_black.mid の主旋律候補トラック後半を使用
//
// 対象フレーズ:
// 「シャイニングスター綴れば 夢に眠る幻が 掌に降り注ぐ 新たな世界へ」
//
// 重要:
// - 以前の仮推定ではなく、今回はユーザー提供MIDIのノート番号をそのまま使用しています。
// - デフォルトは MIDI 通りの音程です。
// - ブザーで高すぎると感じる場合のみ TRANSPOSE_SEMITONES = -12 にしてください。

#ifndef D10
#define D10 10
#endif

#include <math.h>

const int BUZZER_PIN = D10;
const float BPM = 158.000764f;
const float GAP_RATIO = 0.08f;
const int REPEAT_WAIT_MS = 3000;

// 0 = MIDI通り, -12 = 1オクターブ下
const int TRANSPOSE_SEMITONES = 0;

// MIDIノート番号。
// 抽出区間は主旋律候補トラックの後半フレーズで、音価もMIDIに合わせています。
const int melodyMidi[] = {
  77, 78, 80, 80, 80, 80, 80, 73, 73, 73, 80, 78,
  77, 78, 75, 84, 84, 82, 84, 85, 82, 84, 85, 84,
  82, 82, 84, 84, 87, 85, 77, 78, 78, 80, 82, 81,
  81, 78, 77, 75, 77, 80, 85, 82, 85, 84, 84, 85
};

// 1.0 = 4分音符, 0.5 = 8分音符
const float noteBeats[] = {
  0.5, 0.5, 1.0, 0.5, 1.0, 1.0, 1.0, 0.5, 1.5, 0.5, 0.5, 0.5,
  1.0, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 1.5, 0.5, 0.5, 1.0, 0.5,
  1.0, 1.0, 1.0, 0.5, 1.0, 1.5, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0,
  0.5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 2.0, 1.0, 0.5, 5.0
};

static_assert(sizeof(melodyMidi) / sizeof(melodyMidi[0]) == sizeof(noteBeats) / sizeof(noteBeats[0]),
              "melodyMidi と noteBeats の要素数が一致していません");

int beatToMs(float beats) {
  const float quarterMs = 60000.0f / BPM;
  return (int)(quarterMs * beats + 0.5f);
}

int midiToFreq(int midiNote) {
  int shifted = midiNote + TRANSPOSE_SEMITONES;
  float freq = 440.0f * powf(2.0f, (shifted - 69) / 12.0f);
  return (int)(freq + 0.5f);
}

void playNoteMidi(int midiNote, float beats) {
  int fullLength = beatToMs(beats);
  int soundLength = (int)(fullLength * (1.0f - GAP_RATIO));
  int gapLength = fullLength - soundLength;

  if (midiNote > 0) {
    int freq = midiToFreq(midiNote);
    tone(BUZZER_PIN, freq, soundLength);
    delay(soundLength);
    noTone(BUZZER_PIN);
  } else {
    delay(soundLength);
  }

  delay(gapLength);
}

void playMelody() {
  const size_t count = sizeof(melodyMidi) / sizeof(melodyMidi[0]);
  for (size_t i = 0; i < count; ++i) {
    playNoteMidi(melodyMidi[i], noteBeats[i]);
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
