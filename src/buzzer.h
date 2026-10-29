#pragma once
#include <Arduino.h>
#include "config.h"

enum SoundAlert { SND_NONE, SND_CLICK, SND_REFLOW_START, SND_COMPLETE, SND_FAULT, SND_COOLED, SND_COUNTDOWN };

class BuzzerEngine {
public:
  bool mute = false;

  void init() {
#if defined(ESP32)
    ledcSetup(0, 2000, 8); // Канал 0, 2 кГц, 8 бит
    ledcAttachPin(PIN_BUZZER, 0);
    ledcWriteTone(0, 0);
#else
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
#endif
  }

  void playTone(unsigned int freq) {
#if defined(ESP32)
    ledcWriteTone(0, freq);
#else
    tone(PIN_BUZZER, freq);
#endif
  }

  void stopTone() {
#if defined(ESP32)
    ledcWriteTone(0, 0); // Глушим звук без деинициализации таймера (нет ошибок в логе!)
#else
    noTone(PIN_BUZZER);
#endif
  }

  void play(SoundAlert alert) {
    if (mute && alert != SND_FAULT) return;
    current_sound = alert;
    sound_step = 0;
    sound_step_time = millis();
  }

  void tick(uint32_t now) {
    if (current_sound == SND_NONE) return;
    uint32_t dt = now - sound_step_time;

    switch (current_sound) {
      case SND_CLICK:
        if (sound_step == 0) { playTone(2400); sound_step++; sound_step_time = now; }
        else if (dt > 25) { stopTone(); current_sound = SND_NONE; }
        break;

      case SND_REFLOW_START:
        if (sound_step == 0) { playTone(2200); sound_step++; sound_step_time = now; }
        else if (sound_step == 1 && dt > 120) { stopTone(); sound_step++; sound_step_time = now; }
        else if (sound_step == 2 && dt > 80)  { playTone(2600); sound_step++; sound_step_time = now; }
        else if (sound_step == 3 && dt > 150) { stopTone(); current_sound = SND_NONE; }
        break;

      case SND_COMPLETE:
        if (sound_step == 0) { playTone(1800); sound_step++; sound_step_time = now; }
        else if (sound_step == 1 && dt > 120) { playTone(2200); sound_step++; sound_step_time = now; }
        else if (sound_step == 2 && dt > 120) { playTone(2700); sound_step++; sound_step_time = now; }
        else if (sound_step == 3 && dt > 300) { stopTone(); current_sound = SND_NONE; }
        break;

      case SND_COOLED:
        if (sound_step == 0) { playTone(1500); sound_step++; sound_step_time = now; }
        else if (dt > 200) { stopTone(); current_sound = SND_NONE; }
        break;

      case SND_COUNTDOWN:
        if (sound_step == 0) { playTone(2800); sound_step++; sound_step_time = now; }
        else if (dt > 40) { stopTone(); current_sound = SND_NONE; }
        break;

      case SND_FAULT:
        if (sound_step % 2 == 0) {
          if (dt == 0) playTone(1200);
          if (dt > 250) { stopTone(); sound_step++; sound_step_time = now; }
        } else {
          if (dt > 150) {
            sound_step++; sound_step_time = now;
            if (sound_step >= 6) current_sound = SND_NONE;
            else playTone(1200);
          }
        }
        break;

      default:
        stopTone();
        current_sound = SND_NONE;
        break;
    }
  }

private:
  SoundAlert current_sound = SND_NONE;
  uint32_t sound_step_time = 0;
  uint8_t sound_step = 0;
};

extern BuzzerEngine buzzer;
