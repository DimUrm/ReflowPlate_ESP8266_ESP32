// ============================================================================
//  menu.h — меню энкодера на OLED, заставка с версией и экран прогресса OTA.
//
//  Подключается из main.cpp ПОСЛЕ startHeating()/startAutoTune()/limitTemp() и ПЕРЕД pollInputs(),
//  использует глобальные переменные и функции main.cpp (общая единица трансляции).
//
//  Управление (как у промышленных плит):
//    долгое нажатие (>= 1 с) в простое — открыть меню
//    поворот ручки                     — выбор пункта (в меню уставку не меняет)
//    короткое нажатие / кнопка CONFIRM — открыть пункт, переключить значение, подтвердить
//    долгое нажатие / кнопка BACK      — назад (из главного меню — выход)
//  Меню закрывается само: через 30 с без действий, при запуске нагрева, при аварии, если пропал экран.
//  Без OLED меню не открывается (иначе ручка «пропала» бы вслепую).
// ============================================================================
#pragma once

int      ota_percent = -1;          // прогресс прошивки по воздуху, -1 = обновления нет
uint32_t ota_last_chunk = 0;        // когда пришёл последний кусок прошивки (для сброса зависшей загрузки)

enum MenuScreen : uint8_t { MS_CLOSED = 0, MS_MAIN, MS_PROFILES, MS_SETTINGS, MS_INFO, MS_CONFIRM };
enum MenuConfirmKind : uint8_t { MC_PROFILE = 0, MC_TUNE };

uint8_t  menu_screen = MS_CLOSED;
uint8_t  menu_sel = 0, menu_info_page = 0, menu_return = MS_MAIN;
uint8_t  menu_confirm_kind = MC_PROFILE, menu_confirm_idx = 0;
uint32_t menu_last_input = 0;
bool     menu_dirty = false;

static const uint32_t MENU_TIMEOUT_MS = 30000UL;
static const uint8_t  MENU_STBY_CHOICES[] = {0, 5, 10, 20, 30, 60};
static const char* const MENU_SENSOR_LABEL[] = {"AUTO", "MAX6675", "NTC", "SIM"};

bool menuActive() { return menu_screen != MS_CLOSED; }

// Меню доступно только в простое и только при исправном экране
bool menuCanOpen() {
  return oled_ok && !is_heating && !profile.is_active && tune_state != TUNE_RUNNING && fault == F_NONE;
}

void menuOpen() {
  if (!menuCanOpen()) return;
  menu_screen = MS_MAIN; menu_sel = 0; menu_last_input = millis(); menu_dirty = true;
  buzzer.play(SND_CLICK);
}

void menuClose() { menu_screen = MS_CLOSED; menu_dirty = false; }

static uint8_t menuCount() {
  switch (menu_screen) {
    case MS_MAIN:     return 5;   // Profiles, Settings, Auto-Tune, Info, Exit
    case MS_PROFILES: return 5;   // 4 профиля + Back
    case MS_SETTINGS: return 5;   // Sound, Standby, Sensor, Fan, Back
    default:          return 1;
  }
}

void menuTurn(int d) {
  menu_last_input = millis(); menu_dirty = true;
  int steps = d < 0 ? -d : d;
  if (steps > 4) steps = 4;                           // быстрое вращение не должно «проскакивать» весь список
  for (int k = 0; k < steps; k++) {
    if (menu_screen == MS_INFO) {                     // в Info поворот листает страницы по кругу из 4
      menu_info_page = (menu_info_page + (d > 0 ? 1 : 3)) % 4;
    } else if (menu_screen != MS_CONFIRM) {
      uint8_t n = menuCount();
      menu_sel = (menu_sel + (d > 0 ? 1 : n - 1)) % n;
    }
  }
  buzzer.play(SND_CLICK);
}

static void menuConfirmYes() {
  if (menu_confirm_kind == MC_PROFILE) {
    uint8_t idx = menu_confirm_idx;
    // те же условия, что у веб-команды start_prof_idx, плюс пик профиля не выше лимита датчика
    if (fault == F_NONE && temp_valid && idx < 4 && profile.list[idx].reflow <= limitTemp() - TARGET_MARGIN) {
      profile.startByIndex(idx);
      startHeating();
      menuClose();
    } else {
      buzzer.play(SND_FAULT);
    }
  } else {
    if (fault == F_NONE && temp_valid) { startAutoTune(); menuClose(); }
    else buzzer.play(SND_FAULT);
  }
  forceTele = true;
}

void menuPress() {
  menu_last_input = millis(); menu_dirty = true;
  switch (menu_screen) {
    case MS_MAIN:
      if (menu_sel == 0)      { menu_screen = MS_PROFILES; menu_sel = 0; }
      else if (menu_sel == 1) { menu_screen = MS_SETTINGS; menu_sel = 0; }
      else if (menu_sel == 2) { menu_confirm_kind = MC_TUNE; menu_return = MS_MAIN; menu_screen = MS_CONFIRM; }
      else if (menu_sel == 3) { menu_screen = MS_INFO; menu_info_page = 0; }
      else menuClose();
      break;
    case MS_PROFILES:
      if (menu_sel < 4) { menu_confirm_kind = MC_PROFILE; menu_confirm_idx = menu_sel; menu_return = MS_PROFILES; menu_screen = MS_CONFIRM; }
      else { menu_screen = MS_MAIN; menu_sel = 0; }
      break;
    case MS_SETTINGS:
      if (menu_sel == 0) {                                   // звук
        buzzer.mute = !buzzer.mute; cfg_save = true;
        if (!buzzer.mute) buzzer.play(SND_CLICK);
      } else if (menu_sel == 1) {                            // таймер standby: по кругу из списка
        uint8_t i = 0;
        for (uint8_t k = 0; k < sizeof(MENU_STBY_CHOICES); k++) if (MENU_STBY_CHOICES[k] == standby_timeout_min) i = k;
        standby_timeout_min = MENU_STBY_CHOICES[(i + 1) % sizeof(MENU_STBY_CHOICES)];
        cfg_save = true;
      } else if (menu_sel == 2) {                            // датчик: как веб-команда sensor
        cfg_sensor = (cfg_sensor + 1) % 4;
        stopHeating(); fault = F_NONE;
        apply_sensor = true; cfg_save = true;
      } else if (menu_sel == 3) {                            // обдув
        fan_manual = !fan_manual;
      } else { menu_screen = MS_MAIN; menu_sel = 1; }
      if (menu_sel != 4) buzzer.play(SND_CLICK);
      break;
    case MS_INFO:
      menu_screen = MS_MAIN; menu_sel = 3;
      break;
    case MS_CONFIRM:
      menuConfirmYes();
      break;
    default: break;
  }
  forceTele = true;
}

void menuBack() {
  menu_last_input = millis(); menu_dirty = true;
  switch (menu_screen) {
    case MS_PROFILES: menu_screen = MS_MAIN; menu_sel = 0; break;
    case MS_SETTINGS: menu_screen = MS_MAIN; menu_sel = 1; break;
    case MS_INFO:     menu_screen = MS_MAIN; menu_sel = 3; break;
    case MS_CONFIRM:  menu_screen = menu_return; break;
    default:          menuClose(); break;
  }
}

// ---------------------------------------------------------------- рисование
#if !defined(DISPLAY_ST7789) && !defined(DISPLAY_NOKIA5110)
static void menuTitle(const char* t) {
  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(3, 2);
  display.print(t);
  display.setTextColor(SSD1306_WHITE);
}

static void menuRow(uint8_t row, const char* text, bool sel) {
  int y = 13 + row * 10;
  if (sel) {
    display.fillRect(0, y - 1, 128, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(3, y);
  display.print(text);
  display.setTextColor(SSD1306_WHITE);
}

void menuDraw() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  char line[40];

  switch (menu_screen) {
    case MS_MAIN: {
      static const char* const items[5] = {"Profiles >", "Settings >", "Auto-Tune", "Info >", "Exit"};
      menuTitle("MENU");
      for (uint8_t i = 0; i < 5; i++) menuRow(i, items[i], i == menu_sel);
      break;
    }
    case MS_PROFILES: {
      menuTitle("PROFILES");
      for (uint8_t i = 0; i < 4; i++) {
        snprintf(line, sizeof(line), "%-12.12s %3d%cC", profile.list[i].name, (int)profile.list[i].reflow, (char)247);
        menuRow(i, line, i == menu_sel);
      }
      menuRow(4, "Back", menu_sel == 4);
      break;
    }
    case MS_SETTINGS: {
      menuTitle("SETTINGS");
      snprintf(line, sizeof(line), "Sound: %s", buzzer.mute ? "OFF" : "ON");
      menuRow(0, line, menu_sel == 0);
      if (standby_timeout_min == 0) snprintf(line, sizeof(line), "Standby: OFF");
      else snprintf(line, sizeof(line), "Standby: %u min", (unsigned)standby_timeout_min);
      menuRow(1, line, menu_sel == 1);
      snprintf(line, sizeof(line), "Sensor: %s", MENU_SENSOR_LABEL[cfg_sensor % 4]);
      menuRow(2, line, menu_sel == 2);
      snprintf(line, sizeof(line), "Fan: %s", fan_manual ? "ON" : "AUTO");
      menuRow(3, line, menu_sel == 3);
      menuRow(4, "Back", menu_sel == 4);
      break;
    }
    case MS_INFO: {
      snprintf(line, sizeof(line), "INFO %u/4", (unsigned)menu_info_page + 1);
      menuTitle(line);
      char r[4][32];
      if (menu_info_page == 0) {
        snprintf(r[0], 32, "Version: %s", AUTO_FW_VERSION);
        snprintf(r[1], 32, "%s", AUTO_BUILD_DATE);
        snprintf(r[2], 32, "Chip ID: %06X", (unsigned)getBoardChipId());
        snprintf(r[3], 32, "Sketch: %u KB", (unsigned)(ESP.getSketchSize() / 1024));
      } else if (menu_info_page == 1) {
        bool w_ok = (WiFi.status() == WL_CONNECTED);
        String ip = w_ok ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
        snprintf(r[0], 32, "%s: %s", w_ok ? "IP" : "AP", ip.c_str());
        String ssid = w_ok ? WiFi.SSID() : String("-");
        snprintf(r[1], 32, "SSID: %.15s", ssid.c_str());
        if (w_ok) snprintf(r[2], 32, "RSSI: %d dBm", (int)WiFi.RSSI());
        else snprintf(r[2], 32, "Not connected");
        snprintf(r[3], 32, "%s.local", HOST);
      } else if (menu_info_page == 2) {
        uint32_t up = millis() / 1000UL;
        snprintf(r[0], 32, "Uptime: %02u:%02u:%02u", (unsigned)(up / 3600), (unsigned)((up / 60) % 60), (unsigned)(up % 60));
        snprintf(r[1], 32, "Heap: %u B", (unsigned)ESP.getFreeHeap());
        snprintf(r[2], 32, "MaxBlk: %u B", (unsigned)getLargestFreeBlock());
        String rr = getBoardResetReason();
        snprintf(r[3], 32, "Reset: %.13s", rr.c_str());
      } else {
        snprintf(r[0], 32, "Heater: %.1f h", lifetime_heat_min / 60.0f);
        snprintf(r[1], 32, "Reflow runs: %u", (unsigned)reflow_cycles_count);
        snprintf(r[2], 32, "Session: %.1f Wh", session_wh);
        snprintf(r[3], 32, "%s %s", SENSOR_NAMES[sensor], SS_NAMES[sensor_st]);
      }
      for (uint8_t i = 0; i < 4; i++) menuRow(i, r[i], false);
      menuRow(4, "turn:page hold:back", false);
      break;
    }
    case MS_CONFIRM: {
      if (menu_confirm_kind == MC_PROFILE) {
        const ReflowProfileItem& p = profile.list[menu_confirm_idx];
        menuTitle("START PROFILE?");
        snprintf(line, sizeof(line), "%.20s", p.name);               menuRow(0, line, false);
        snprintf(line, sizeof(line), "Pre    %3d%cC", (int)p.preheat, (char)247); menuRow(1, line, false);
        snprintf(line, sizeof(line), "Soak   %3d%cC %3ds", (int)p.soak, (char)247, (int)p.soak_t);     menuRow(2, line, false);
        snprintf(line, sizeof(line), "Reflow %3d%cC %3ds", (int)p.reflow, (char)247, (int)p.reflow_t); menuRow(3, line, false);
      } else {
        menuTitle("AUTO-TUNE?");
        menuRow(0, "Heats plate to ~150C", false);
        menuRow(1, "and cycles the power", false);
        menuRow(2, "for 5-10 minutes.", false);
        menuRow(3, "Stay near the plate!", false);
      }
      menuRow(4, "press=YES  hold=NO", true);
      break;
    }
    default: break;
  }
  display.display();
}

// Заставка при включении: название, версия и дата сборки
void drawSplash() {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 4);  display.print("REFLOW");
  display.setCursor(10, 22); display.print("PLATE");
  display.setTextSize(1);
  display.setCursor(10, 44); display.print("FW "); display.print(AUTO_FW_VERSION);
  display.setCursor(10, 54); display.print(AUTO_BUILD_DATE);
  display.display();
  delay(1500);                         // нагрев в это время выключен (SSR = LOW с первой строки setup)
}

// Экран обновления прошивки по воздуху
void drawOtaProgress() {
  int p = ota_percent < 0 ? 0 : (ota_percent > 100 ? 100 : ota_percent);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 8);  display.print("FIRMWARE UPDATE");
  display.drawRect(4, 26, 120, 12, SSD1306_WHITE);
  display.fillRect(6, 28, (116 * p) / 100, 8, SSD1306_WHITE);
  display.setCursor(52, 42); display.printf("%d%%", p);
  display.setCursor(16, 54); display.print("Do not power off!");
  display.display();
}
#else
// Заглушки для цветного экрана (отрисовка UI для ST7789 находится в display_st7789.h)
static void menuTitle(const char* t) {}
static void menuRow(uint8_t row, const char* text, bool sel) {}
void menuDraw() {}
void drawSplash() {}
void drawOtaProgress() {}
#endif

// Вызывается из loop(): таймаут меню, перерисовка по событию, сброс зависшей загрузки прошивки
void menuTick(uint32_t now) {
  if (menuActive()) {
    if (!menuCanOpen() || now - menu_last_input > MENU_TIMEOUT_MS) menuClose();   // нагрев/авария/пропал экран/бездействие
    else if (menu_dirty && !oled_sleeping) { menu_dirty = false; menuDraw(); }
  }
  if (ota_percent >= 0 && ota_percent < 100 && now - ota_last_chunk > 20000UL) {  // загрузка оборвалась
    Update.end(false);                                                            // сбрасывает состояние Updater
    ota_percent = -1;
  }
}
