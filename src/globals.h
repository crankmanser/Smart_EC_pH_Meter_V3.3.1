#ifndef GLOBALS_H
#define GLOBALS_H

// This file declares all shared variables, objects, enums, and structs for the project.

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <SPI.h>
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// =================================================================
// Pin Definitions & Constants
// =================================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define TCA_ADDRESS 0x70
#define RTC_TCA_CHANNEL 0
#define OLED1_TCA_CHANNEL 7
#define OLED2_TCA_CHANNEL 5
#define OLED3_TCA_CHANNEL 2
#define ENCODER_PIN_A 25
#define ENCODER_PIN_B 26
#define BTN_TOP_PIN 36
#define BTN_MIDDLE_PIN 39
#define BTN_BOTTOM_PIN 34
#define TOP_LED_RED_PIN 12
#define TOP_LED_GREEN_PIN 14
#define BOTTOM_LED_GREEN_PIN 27
#define ONE_WIRE_BUS 15
#define DHTPIN 13
#define DHTTYPE DHT11
#define SD_CS_PIN 5
#define SLOW_SPEED_MAX_PULSES_INTERVAL 55
#define MEDIUM_SPEED_MAX_PULSES_INTERVAL 165
#define FINE_CONTROL_PULSES_PER_STEP 433
#define MEDIUM_SPEED_PULSES_PER_STEP 161
#define FAST_SPEED_PULSES_PER_STEP 77
#define RTC_FOCUS_DAY 0
#define RTC_FOCUS_MONTH 1
#define RTC_FOCUS_YEAR 2
#define RTC_FOCUS_HOUR 3
#define RTC_FOCUS_MINUTE 4
#define NUM_RTC_FIELDS 5

// =================================================================
// Enums and Structs (Type Definitions)
// =================================================================

enum DeviceStatus {
    STATUS_BOOTING, STATUS_IDLE, STATUS_MEASURING, STATUS_CALIBRATING, STATUS_ERROR, STATUS_SAVING,
    STATUS_SD_ERROR
};

enum TempUnit { UNIT_CELSIUS, UNIT_FAHRENHEIT, UNIT_KELVIN };

enum MenuState {
    STATE_HOME_SCREEN,
    STATE_MENU_NAV,
    STATE_SETTINGS_RTC_ADJUST,
    STATE_SETTINGS_BATTERY_INFO
};

struct MenuItem {
    const char* label;
    MenuState targetStateOnSelect;
    const MenuItem* subMenu;
    int numSubItems;
    void (*actionFunction)();
};

typedef struct {
    float ds18b20_temperatureC;
    bool ds18b20_sensorFound;
    TempUnit ds18b20_displayUnit;
    float dht11_temperatureC;
    float dht11_humidity;
    bool dht11_sensorFound;
    TempUnit dht11_displayUnit;
    uint16_t battery_cycles;
    uint8_t battery_health_pct;
} SensorCoreData_t;


// =================================================================
// Global Function Declarations (Prototypes)
// =================================================================
// --- From Sect 2: tcaSelect (Helper needed by display functions) ---
void tcaSelect(uint8_t ch);

// =================================================================
// Global Variables (Declarations using 'extern')
// =================================================================

// --- Peripheral Objects ---
extern Adafruit_SSD1306 oled1, oled2, oled3;
extern RTC_PCF8563 rtc;
extern Adafruit_INA219 ina219;
extern OneWire oneWire;
extern DallasTemperature ds18b20_sensors;
extern DHT dht;

// --- UI State ---
extern DeviceStatus currentDeviceStatus;
extern MenuState currentMenuState;
extern MenuState previousMenuStateBeforeAction;
extern int currentMenuLevel;
extern const MenuItem* L1_activeMenuItems;
extern int L1_numActiveItems, L1_selectedIndex;
extern const char* L1_selectedLabelForOLED3Display;
extern int L1_parentIndexOfActiveL2;
extern const MenuItem* L2_activeMenuItems;
extern int L2_numActiveItems, L2_selectedIndex;
extern const char* L2_selectedLabelForOLED2Display;
extern int L2_parentIndexOfActiveL3;
extern const MenuItem* L3_activeMenuItems;
extern int L3_numActiveItems, L3_selectedIndex;
extern bool needsRedrawOLED1, needsRedrawOLED2, needsRedrawOLED3;
extern bool isTextScrollingActive;
extern uint16_t rtcSet_year;
extern uint8_t rtcSet_month, rtcSet_day, rtcSet_hour, rtcSet_minute;
extern int rtcSet_focusedField;
extern DateTime currentRtcDateTimeForDisplay_RTCAdjust;

// --- Encoder ---
extern volatile long encoder_raw_pulses;
extern volatile uint8_t last_encoder_ab_state;
extern const int8_t qem_decode_table[];

// --- Sensor Data for Display ---
extern float currentWaterTempC_display, currentAmbientTempC_display, currentHumidity_display;
extern uint16_t batteryChargeCycles_display;
extern uint8_t batteryHealthPercentage_display;
extern int currentBatteryIconLevel; 

// --- FreeRTOS Handles ---
extern TaskHandle_t uiTaskHandle, sensorTaskHandle;
extern QueueHandle_t sensorDataQueue;

// --- Menu Item Arrays ---
extern const MenuItem mainMenu_Items[];
extern const int numMainMenuItems;
extern const MenuItem settingsSubMenuItems[];
extern const int numSettingsSubMenuItems;

#endif // GLOBALS_H