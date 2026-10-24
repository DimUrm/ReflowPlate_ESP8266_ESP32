#pragma once
#include <Arduino.h>

#if defined(ESP32) && defined(DISPLAY_ST7789)

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <WiFi.h>
#include "config.h"

// 16-битные цвета (RGB565)
#define C_BLACK       0x0000
#define C_DARK_BG     0x0841
#define C_WHITE       0xFFFF
#define C_GRAY        0x7BEF
#define C_CYAN        0x07FF
#define C_GREEN       0x07E0
#define C_YELLOW      0xFFE0
#define C_ORANGE      0xFD20
#define C_RED         0xF800
#define C_BLUE        0x001F

// Мост совместимости с кодом OLED
#define SSD1306_WHITE        C_WHITE
#define SSD1306_BLACK        C_BLACK
#define SSD1306_SWITCHCAPVCC 0x02
#define SSD1306_DISPLAYON    0xAF
#define SSD1306_DISPLAYOFF   0xAE
#define SSD1306_SETCONTRAST  0x81

// Адаптер: добавляет методы clearDisplay(), display() и ssd1306_command()
class Display_ST7789_Adapter : public Adafruit_ST7789 {
public:
  Display_ST7789_Adapter(SPIClass *spiClass, int8_t cs, int8_t dc, int8_t rst)
    : Adafruit_ST7789(spiClass, cs, dc, rst) {}

  void clearDisplay() { fillScreen(C_BLACK); }
  void display() {}
  
  void ssd1306_command(uint8_t c) {
    if (c == SSD1306_DISPLAYON) {
      enableSleep(false);
      enableDisplay(true);
    } else if (c == SSD1306_DISPLAYOFF) {
      enableDisplay(false);
      enableSleep(true);
    }
  }
};

// Создание аппаратного SPI и самого объекта экрана прямо здесь
static SPIClass tftSPI(HSPI);
static Display_ST7789_Adapter display(&tftSPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
static bool tft_initialized = false;

#define tft display

inline uint16_t getTemperatureColor(float t) {
  if (t < 50.0f)  return C_CYAN;
  if (t < 140.0f) return C_YELLOW;
  if (t < 195.0f) return C_ORANGE;
  return C_RED;
}

inline bool initTFT() {
  tftSPI.begin(PIN_TFT_SCLK, -1, PIN_TFT_MOSI, PIN_TFT_CS);
  display.init(135, 240); // 1.14" IPS
  display.setRotation(1); // Альбомная (240x135)
  display.fillScreen(C_BLACK);
  tft_initialized = true;
  return true;
}

inline void drawSplashST7789() {
  if (!tft_initialized) return;
  display.fillScreen(C_DARK_BG);
  display.drawRect(2, 2, 236, 131, C_ORANGE);
  display.drawRect(4, 4, 232, 127, C_GRAY);

  display.setTextWrap(false);
  display.setTextColor(C_ORANGE, C_DARK_BG);
  display.setTextSize(3);
  display.setCursor(20, 20);
  display.print("REFLOW");

  display.setTextColor(C_WHITE, C_DARK_BG);
  display.setCursor(20, 50);
  display.print("PLATE");

  display.setTextSize(1);
  display.setTextColor(C_CYAN, C_DARK_BG);
  display.setCursor(20, 90);
  display.print("ESP32 COLOR EDITION");

  display.setTextColor(C_GRAY, C_DARK_BG);
  display.setCursor(20, 105);
  display.printf("FW %s (%s)", AUTO_FW_VERSION, AUTO_BUILD_DATE);

  delay(1500);
  display.fillScreen(C_BLACK);
}

inline void drawUI_ST7789(float current_t, float target_t, float power, bool heating, bool ssr, 
                          bool fan, const char* prof, const char* phase, uint8_t fault_c, 
                          bool mute, bool sleeping, bool valid) {
  if (!tft_initialized || sleeping) return;

  // 1. Верхний бар
  display.setTextSize(1);
  display.setCursor(4, 4);
  display.setTextColor(WiFi.status() == WL_CONNECTED ? C_GREEN : C_YELLOW, C_BLACK);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("IP: "); display.print(WiFi.localIP());
  } else {
    display.print("AP: "); display.print(WiFi.softAPIP());
  }

  display.setCursor(195, 4);
  if (mute) {
    display.setTextColor(C_ORANGE, C_BLACK);
    display.print("[MUTE]");
  } else {
    display.print("      ");
  }

  display.drawFastHLine(0, 16, 240, C_GRAY);

  // 2. Статус и профиль
  display.setCursor(4, 20);
  if (fault_c != 0) {
    display.setTextColor(C_WHITE, C_RED);
    display.printf(" ! FAULT: %-16s ", FAULT_NAMES[fault_c]);
  } else {
    display.setTextColor(heating ? C_RED : C_GREEN, C_BLACK);
    display.print(heating ? "HEAT " : "IDLE ");
    
    display.setTextColor(C_WHITE, C_BLACK);
    display.printf("[%-8s] ", prof);

    if (strcmp(phase, "IDLE") != 0) {
      display.setTextColor(C_YELLOW, C_BLACK);
      display.printf("%-8s", phase);
    } else {
      display.print("        ");
    }
  }

  // 3. Главная цифра температуры
  uint16_t t_color = valid ? getTemperatureColor(current_t) : C_GRAY;
  display.setTextSize(4);
  display.setCursor(15, 38);
  display.setTextColor(t_color, C_BLACK);

  if (valid) {
    display.printf("%5.1f", current_t);
  } else {
    display.print("NO SEN");
  }
  
  display.setTextSize(2);
  display.print(" o");
  display.setTextSize(3);
  display.print("C");

  // 4. Графический бар мощности (0 - 100%)
  int bar_w = (int)((power / 100.0f) * 230.0f);
  display.drawRect(5, 78, 230, 8, C_GRAY);
  
  uint16_t bar_color = (power < 40) ? C_GREEN : ((power < 80) ? C_YELLOW : C_RED);
  display.fillRect(6, 79, bar_w, 6, bar_color);
  display.fillRect(6 + bar_w, 79, 228 - bar_w, 6, C_BLACK);

  display.drawFastHLine(0, 92, 240, C_GRAY);

  // 5. Подвал
  display.setTextSize(2);
  display.setCursor(6, 98);
  display.setTextColor(C_YELLOW, C_BLACK);
  display.printf("SET:%3d", (int)target_t);
  display.setTextSize(1); display.print("o"); display.setTextSize(2); display.print("C ");

  display.setCursor(130, 98);
  display.setTextColor(C_CYAN, C_BLACK);
  display.printf("PWR:%3d%%", (int)power);

  display.setTextSize(1);
  display.setCursor(6, 122);
  display.setTextColor(ssr ? C_RED : C_GRAY, C_BLACK);
  display.print(ssr ? "[SSR: ON ] " : "[SSR: OFF] ");

  display.setTextColor(fan ? C_CYAN : C_GRAY, C_BLACK);
  display.print(fan ? "[FAN: ON ] " : "[FAN: OFF] ");

  display.setTextColor(C_GRAY, C_BLACK);
  display.print(valid ? "K-TYPE " : "DISCON ");
}

inline void drawOtaST7789(int percent) {
  if (!tft_initialized) return;
  int p = percent < 0 ? 0 : (percent > 100 ? 100 : percent);

  display.fillScreen(C_DARK_BG);
  display.drawRect(10, 10, 220, 115, C_CYAN);

  display.setTextColor(C_WHITE, C_DARK_BG);
  display.setTextSize(2);
  display.setCursor(30, 30);
  display.print("OTA FLASHING");

  display.drawRect(20, 60, 200, 16, C_WHITE);
  display.fillRect(22, 62, (196 * p) / 100, 12, C_GREEN);

  display.setTextSize(2);
  display.setTextColor(C_CYAN, C_DARK_BG);
  display.setCursor(95, 85);
  display.printf("%d%%", p);

  display.setTextSize(1);
  display.setTextColor(C_ORANGE, C_DARK_BG);
  display.setCursor(45, 108);
  display.print("DO NOT POWER OFF!");
}

#endif