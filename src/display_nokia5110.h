#pragma once
#include <Arduino.h>

#if defined(ESP32) && defined(DISPLAY_NOKIA5110)

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <WiFi.h>
#include "config.h"

// Используем быстрый программный SPI (пины 1:1 совпадают с подключением ST7789!)
static Adafruit_PCD8544 display(PIN_TFT_SCLK, PIN_TFT_MOSI, PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_RST);
static bool nokia_initialized = false;

inline bool initNokia() {
  display.begin();
  display.setContrast(58); // Оптимальный контраст для большинства матриц Nokia 5110 (55-60)
  display.clearDisplay();
  display.display();
  nokia_initialized = true;
  return true;
}

inline void drawSplashNokia() {
  if (!nokia_initialized) return;
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(BLACK);

  display.setTextSize(2);
  display.setCursor(6, 4);
  display.print("REFLOW");
  display.setCursor(12, 20);
  display.print("PLATE");

  display.setTextSize(1);
  display.setCursor(10, 38);
  display.printf("FW %s", AUTO_FW_VERSION);
  display.display();
  delay(1500);
}

// Компактный интерфейс, адаптированный под крошечное разрешение 84x48
inline void drawUI_Nokia(float current_t, float target_t, float power, bool heating, bool ssr, 
                         bool fan, const char* prof, const char* phase, uint8_t fault_c, 
                         bool mute, bool sleeping, bool valid) {
  if (!nokia_initialized || sleeping) return;
  display.clearDisplay();
  display.setTextColor(BLACK);

  // 1. Верхняя строка (y=0..7): Статус / Профиль / Ошибка
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (fault_c != 0) {
    display.printf("!%s", FAULT_NAMES[fault_c]);
  } else {
    display.printf("%-6.6s %-7.7s", prof, strcmp(phase, "IDLE") == 0 ? (heating ? "HEAT" : "IDLE") : phase);
  }
  display.drawFastHLine(0, 8, 84, BLACK);

  // 2. Центр (y=11..26): Крупная температура
  display.setCursor(6, 11);
  if (valid) {
    display.setTextSize(2);
    display.printf("%3.0f", current_t);
    display.setTextSize(1);
    display.print("o");
    display.setTextSize(2);
    display.print("C");
  } else {
    display.setTextSize(2);
    display.print("NO SEN");
  }

  // Индикация MUTE в правом верхнем углу
  if (mute) {
    display.setTextSize(1);
    display.setCursor(76, 11);
    display.print("M");
  }

  // 3. Шкала мощности ТЭНа 0..100% (y=29..32)
  display.drawRect(0, 29, 84, 4, BLACK);
  int bar_w = (int)((power / 100.0f) * 80.0f);
  if (bar_w > 0) display.fillRect(2, 30, bar_w, 2, BLACK);

  // 4. Подвал (y=35..47): Уставка и статусы SSR/FAN
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.printf("Set:%d%cC P:%d%%", (int)target_t, (char)247, (int)power);

  display.setCursor(0, 43);
  display.printf("%s %s %s", 
                 ssr ? "[SSR]" : " ssr ", 
                 fan ? "[FAN]" : "     ",
                 WiFi.status() == WL_CONNECTED ? "Wi-Fi" : "AP");

  display.display();
}

inline void drawOtaNokia(int percent) {
  if (!nokia_initialized) return;
  int p = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
  display.clearDisplay();
  display.setTextColor(BLACK);

  display.setTextSize(1);
  display.setCursor(12, 4);
  display.print("OTA UPDATE");

  display.drawRect(4, 18, 76, 12, BLACK);
  display.fillRect(6, 20, (72 * p) / 100, 8, BLACK);

  display.setCursor(32, 34);
  display.printf("%d%%", p);
  display.display();
}

#endif
