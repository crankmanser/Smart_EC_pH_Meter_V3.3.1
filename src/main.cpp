#include "globals.h"          // Our new global declarations
#include "ButtonLogic.h"      // The button module
#include "EncoderLogic.h"     // The encoder module
#include "Display.h"
// We will add more includes here as we create more modules

// =================================================================
// Global Variable Definitions
// This is the one and only place where memory for these variables is allocated.
// =================================================================

// --- Peripheral Objects ---
Adafruit_SSD1306 oled1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 oled2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 oled3(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_PCF8563 rtc;
Adafruit_INA219 ina219;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20_sensors(&oneWire);
DHT dht(DHTPIN, DHTTYPE);

// --- UI State ---
DeviceStatus currentDeviceStatus = STATUS_BOOTING;
MenuState currentMenuState = STATE_HOME_SCREEN;
MenuState previousMenuStateBeforeAction = STATE_HOME_SCREEN;
int currentMenuLevel = 0;
const MenuItem* L1_activeMenuItems = nullptr;
int L1_numActiveItems = 0, L1_selectedIndex = 0;
const char* L1_selectedLabelForOLED3Display = nullptr;
int L1_parentIndexOfActiveL2 = -1;
const MenuItem* L2_activeMenuItems = nullptr;
int L2_numActiveItems = 0, L2_selectedIndex = 0;
const char* L2_selectedLabelForOLED2Display = nullptr;
int L2_parentIndexOfActiveL3 = -1;
const MenuItem* L3_activeMenuItems = nullptr;
int L3_numActiveItems = 0, L3_selectedIndex = 0;
bool needsRedrawOLED1 = true, needsRedrawOLED2 = true, needsRedrawOLED3 = true;
bool isTextScrollingActive = false;
uint16_t rtcSet_year = 2025;
uint8_t rtcSet_month = 1, rtcSet_day = 1, rtcSet_hour = 10, rtcSet_minute = 0;
int rtcSet_focusedField = RTC_FOCUS_DAY;
DateTime currentRtcDateTimeForDisplay_RTCAdjust;

// --- Encoder ---
volatile long encoder_raw_pulses = 0;
volatile uint8_t last_encoder_ab_state = 0;
const int8_t qem_decode_table[] = {0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0};

// --- Sensor Data for Display ---
float currentWaterTempC_display = -999.0f;
float currentAmbientTempC_display = -999.0f;
float currentHumidity_display = -999.0f;
uint16_t batteryChargeCycles_display = 1;
uint8_t batteryHealthPercentage_display = 100;
int currentBatteryIconLevel = 0; //

// --- FreeRTOS Handles ---
TaskHandle_t uiTaskHandle = NULL;
TaskHandle_t sensorTaskHandle = NULL;
QueueHandle_t sensorDataQueue = NULL;

// Forward declare action functions that are defined below
void placeholderAction();
void navigateToRtcAdjust();
void navigateToBatteryInfo();
// We will need to add ALL other function declarations here that are not yet in modules

void tcaSelect(uint8_t ch) {
    if(ch>7)return;
    Wire.beginTransmission(TCA_ADDRESS);
    Wire.write(1<<ch);
    Wire.endTransmission();
    delay(5);
}


// =================================================================
// Menu Definitions
// =================================================================
const MenuItem settingsSubMenuItems[] = {
    {"Set Date/Time", STATE_SETTINGS_RTC_ADJUST, nullptr, 0, navigateToRtcAdjust},
    {"Battery Info",  STATE_SETTINGS_BATTERY_INFO, nullptr, 0, navigateToBatteryInfo},
    {"WiFi Config",   STATE_MENU_NAV,            nullptr, 0, placeholderAction},
    {"Display Options",STATE_MENU_NAV,            nullptr, 0, placeholderAction}
};
const int numSettingsSubMenuItems = sizeof(settingsSubMenuItems) / sizeof(MenuItem);

const MenuItem mainMenu_Items[] = {
    {"Sensor Data",    STATE_HOME_SCREEN,         nullptr,                 0,                         nullptr},
    {"Settings",       STATE_MENU_NAV,            settingsSubMenuItems,    numSettingsSubMenuItems,     nullptr},
    {"Calibration Menu",STATE_MENU_NAV,            nullptr,                 0,                         placeholderAction},
    {"Run Test Cycle", STATE_MENU_NAV,            nullptr,                 0,                         placeholderAction}
};
const int numMainMenuItems = sizeof(mainMenu_Items) / sizeof(MenuItem);

// =================================================================
// Placeholder / Action Functions & Other Helpers
// (These will eventually be moved to their own modules)
// =================================================================

void placeholderAction() {
    Serial.println("Placeholder Action");
    currentDeviceStatus = STATUS_CALIBRATING;
    needsRedrawOLED1 = needsRedrawOLED2 = needsRedrawOLED3 = true;
}
void navigateToRtcAdjust() {
    previousMenuStateBeforeAction=STATE_MENU_NAV;
    currentMenuState=STATE_SETTINGS_RTC_ADJUST;
    tcaSelect(RTC_TCA_CHANNEL);
    currentRtcDateTimeForDisplay_RTCAdjust=rtc.now();
    rtcSet_day=currentRtcDateTimeForDisplay_RTCAdjust.day();
    rtcSet_month=currentRtcDateTimeForDisplay_RTCAdjust.month();
    rtcSet_year=currentRtcDateTimeForDisplay_RTCAdjust.year();
    rtcSet_hour=currentRtcDateTimeForDisplay_RTCAdjust.hour();
    rtcSet_minute=currentRtcDateTimeForDisplay_RTCAdjust.minute();
    rtcSet_focusedField=RTC_FOCUS_DAY;
    needsRedrawOLED1=needsRedrawOLED2=needsRedrawOLED3=true;
}
void navigateToBatteryInfo(){
    previousMenuStateBeforeAction=STATE_MENU_NAV;
    currentMenuState=STATE_SETTINGS_BATTERY_INFO;
    needsRedrawOLED1=needsRedrawOLED2=needsRedrawOLED3=true;
}


// =================================================================
// FreeRTOS Tasks (To be moved to their own files later)
// =================================================================
void sensorTask(void *pvParameters) {
    // The full implementation of sensorTask will go here
    for(;;) { vTaskDelay(1000); }
}

void uiTask(void *pvParameters) {
    // The full implementation of uiTask will go here
    for(;;) {
        handleButtons_ui(); // Call our module function
        handleEncoder_ui(); // Call our module function
        vTaskDelay(20);
    }
}


// =================================================================
// Main Setup and Loop
// =================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\nSmart EC & pH Meter - V3.3.1 Booting...");
    currentDeviceStatus = STATUS_BOOTING;

    sensorDataQueue = xQueueCreate(5, sizeof(SensorCoreData_t));
    if (sensorDataQueue == NULL) {
        Serial.println("Error creating sensorDataQueue!");
        while(1);
    }

    xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 1, &sensorTaskHandle, 0);
    xTaskCreatePinnedToCore(uiTask, "UITask", 8192, NULL, 2, &uiTaskHandle, 1);

    Serial.println("Main setup() complete. Tasks launched.");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}