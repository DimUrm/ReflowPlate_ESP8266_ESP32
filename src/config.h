#pragma once
#include <Arduino.h>

// ================= ВЫБОР ТИПА ДИСПЛЕЯ =================
// Для ESP8266 всегда используется OLED (SSD1306/SH1106).
// Для ESP32 можно выбрать либо монохромный OLED, либо цветной TFT ST7789:
#if defined(ESP32)
  #define DISPLAY_ST7789       // Раскомментировано: Цветной IPS 1.14" 240x135
//#define DISPLAY_NOKIA5110  // Вариант 2: Nokia 5110 (84x48)  
//#define DISPLAY_SSD1306    // Закомментировано: Монохромный OLED 128x64
#else
  #define DISPLAY_SSD1306      // На ESP8266 всегда OLED
#endif

// ================= КРОССПЛАТФОРМЕННЫЕ ПИНЫ =================
#if defined(ESP8266)
  #define PIN_SSR       D8   // GPIO15 (Твердотельное реле 220V)
  #define PIN_BUZZER    3    // GPIO3 / RX (ШИМ-зуммер)
  #define PIN_NTC_ADC   A0   // ДЛЯ ESP8266 (АЦП A0)
  #define PIN_ENC_CLK   D5   // GPIO14 (Энкодер CLK)
  #define PIN_ENC_DT    D6   // GPIO12 (Энкодер DT)
  #define PIN_ENC_SW    D7   // GPIO13 (Кнопка энкодера)

  #define PIN_MAX_SO    D3   // GPIO0 (MAX6675 MISO)
  #define PIN_MAX_CS    D4   // GPIO2 (MAX6675 CS)
  #define PIN_MAX_SCK   D0   // GPIO16 (MAX6675 SCK)

  #define PIN_I2C_SDA   D2   // GPIO4 (OLED SDA)
  #define PIN_I2C_SCL   D1   // GPIO5 (OLED SCL)
  
  // На ESP8266 вентилятор и кнопки висят на расширителе PCF8574
  #define USE_PCF8574

#elif defined(ESP32)
  #define PIN_SSR       25   // GPIO25 (Твердотельное реле 220V)
  #define PIN_BUZZER    26   // GPIO26 (ШИМ-зуммер)
  #define PIN_NTC_ADC   36   // ДЛЯ ESP32 (GPIO36 / SENSOR_VP)
  #define PIN_ENC_CLK   18   // GPIO18 (Энкодер CLK)
  #define PIN_ENC_DT    19   // GPIO19 (Энкодер DT)
  #define PIN_ENC_SW    23   // GPIO23 (Кнопка энкодера)

  #define PIN_MAX_SO    16   // GPIO16 (MAX6675 MISO)
  #define PIN_MAX_CS    4    // GPIO4  (MAX6675 CS)
  #define PIN_MAX_SCK   17   // GPIO17 (MAX6675 SCK)

  // Пины для цветного дисплея 1.14" ST7789 (4-Wire SPI)
  #define PIN_TFT_SCLK  14   // GPIO14 (SPI Clock)
  #define PIN_TFT_MOSI  13   // GPIO13 (SPI Data / MOSI)
  #define PIN_TFT_CS    5    // GPIO5  (Chip Select)
  #define PIN_TFT_DC    21   // GPIO21 (Data / Command — эксклюзивно для TFT!)
  #define PIN_TFT_RST   22   // GPIO22 (Reset дисплея — эксклюзивно для TFT!)
  #define PIN_TFT_BL    -1   // Подсветка подключена к 3.3V

  #if !defined(DISPLAY_ST7789) && !defined(DISPLAY_NOKIA5110)
    #define PIN_I2C_SDA 21   // Активно только если выбран OLED
    #define PIN_I2C_SCL 22
  #endif

  #define PIN_FAN       27   // GPIO27 (Мосфет вентилятора)
  #define PIN_BTN_BACK  32   // GPIO32 (Кнопка Back)
  #define PIN_BTN_CONF  33   // GPIO33 (Кнопка Confirm)
#endif

// ================= СИСТЕМНЫЕ КОНСТАНТЫ =================
static const char*    AP_SSID = "ReflowPlate";
static const char*    AP_PASS = "reflow1234";
static const char*    HOST    = "reflow";

static const float    HEATER_RATED_WATTS = 450.0f; // Паспортная мощность ТЭНа (450W)
static const float    LIMIT_TC      = 270.0f;
static const float    TARGET_MARGIN = 10.0f;
static const uint32_t SAMPLE_MS     = 250;
static const uint32_t WINDOW_MS     = 1000;
static const uint32_t HEAT_MAX_MS   = 60UL * 60UL * 1000UL;
static const uint32_t NO_RISE_MS    = 60000UL;
static const float    NO_RISE_DT    = 3.0f;

// --- Паузы подключения к MQTT-брокеру (v2.4.23) ---
static const uint32_t MQTT_RETRY_MIN_MS = 10000UL;   // пауза между попытками подключения
static const uint32_t MQTT_RETRY_MAX_MS = 120000UL;  // максимум паузы (удваивается после каждой неудачи)
// --- Версия прошивки: макросы задаёт version_bump.py при сборке в PlatformIO; в других средах подставятся запасные ---
#ifndef AUTO_FW_VERSION
#define AUTO_FW_VERSION "dev"
#endif
#ifndef AUTO_BUILD_DATE
#define AUTO_BUILD_DATE __DATE__ " " __TIME__
#endif
static const float    I_ZONE        = 10.0f;

//static const float    NTC_R_FIXED   = 100000.0f, NTC_R0 = 100000.0f, NTC_BETA = 3950.0f;
//static const float    NTC_VCC       = 3.3f, ADC_FULL_V = 3.2f;
//static const float    LIMIT_NTC     = 120.0f;
static const float      NTC_R_FIXED   = 4700.0f,   NTC_R0 = 100000.0f, NTC_BETA = 3950.0f;
static const float      NTC_VCC       = 3.3f, ADC_FULL_V = 3.3f;
static const float      LIMIT_NTC     = 250.0f;

static const float    SIM_AMB       = 25.0f;
static const float    SIM_PMAX      = 400.0f;
static const float    SIM_C         = 300.0f;
static const float    SIM_K         = 1.0f;
static const float    SIM_TAU_S     = 3.0f;

// ================= ПЕРЕЧИСЛЕНИЯ СТАТУСОВ =================
enum SensorType { SN_MAX, SN_NTC, SN_SIM };
enum SensorState { SS_OK, SS_ABSENT, SS_OPEN, SS_RANGE };
enum FaultCode { F_NONE, F_NO_SENSOR, F_SENSOR_OPEN, F_SENSOR_RANGE, F_OVERTEMP, F_NO_RISE, F_TIMEOUT };

const char* const SENSOR_NAMES[] = {"MAX6675", "NTC", "SIM"};
const char* const SS_NAMES[]     = {"OK", "ABSENT", "OPEN/SHORT", "RANGE"};
const char* const FAULT_NAMES[]  = {"OK", "SENSOR ABSENT", "SENSOR OPEN", "SENSOR RANGE", "OVERTEMP", "NO HEATING", "TIMEOUT"};

enum CfgSensor { CFG_AUTO, CFG_MAX, CFG_NTC, CFG_SIM };
const char* const CFG_NAMES[]    = {"auto", "max", "ntc", "sim"};

// ================= НАСТРОЙКИ СНА И ОЖИДАНИЯ =================
static const float STANDBY_TEMP = 100.0f; // Дежурная температура в режиме ожидания (°C)

enum StandbyMode {
  STBY_ACTIVE,   // Обычный рабочий нагрев
  STBY_STANDBY,  // Сброс до 100°C из-за бездействия
  STBY_SLEEP_OFF // Полное отключение нагрева (Auto-OFF)
  };

// ================= РАСШИРИТЕЛЬ ПОРТОВ PCF8574 (I2C) =================
#define PCF8574_ADDR_1   0x20  // Базовый адрес PCF8574 (A0=A1=A2=GND)
#define PCF8574_ADDR_2   0x27  // Альтернативный адрес
#define PCF8574A_ADDR    0x38  // Адрес для версии PCF8574A

// ================= СТАТУС AUTOTUNE =================
enum TuneState { TUNE_OFF, TUNE_RUNNING, TUNE_DONE, TUNE_FAILED };

// Номера пинов на гребенке PCF8574 (P0 - P7)
#define PCF_PIN_FAN      0     // P0 -> Мосфет вентилятора охлаждения
#define PCF_PIN_LIGHT    1     // P1 -> Реле вытяжки дыма или подсветка
#define PCF_PIN_BTN_BACK 2     // P2 -> Кнопка Back (Стоп / Обдув Fan / Сброс аварии)
#define PCF_PIN_BTN_CONF 3     // P3 -> Кнопка Confirm (Смена пресетов по кругу)

// ================= КРОССПЛАТФОРМЕННЫЕ ХЕЛПЕРЫ =================
static inline uint32_t getBoardChipId() {
#if defined(ESP32)
  return (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF);
#else
  return ESP.getChipId();
#endif
}

static inline uint32_t getLargestFreeBlock() {
#if defined(ESP32)
  return ESP.getMaxAllocHeap();
#else
  return ESP.getMaxFreeBlockSize();
#endif
}

static inline String getBoardResetReason() {
#if defined(ESP32)
  return "ESP32 System";
#else
  return ESP.getResetReason();
#endif
}
