#include "EncoderLogic.h"
#include "globals.h"

// This static variable is private to this file. No other module can access it.
static int acc_raw_pulses = 0; 

// Encoder ISR (from V3.1.1-12, for uiTask)
// This function is designed to be called by an interrupt. It must be fast.
void IRAM_ATTR customEncoderISR() {
    // Read the state of the encoder pins
    uint8_t ca = digitalRead(ENCODER_PIN_A);
    uint8_t cb = digitalRead(ENCODER_PIN_B);
    uint8_t csa = (cb << 1) | ca;

    // If the state has changed, update the pulse count based on the lookup table
    if (csa != last_encoder_ab_state) {
        encoder_raw_pulses += qem_decode_table[(last_encoder_ab_state << 2) | csa];
        last_encoder_ab_state = csa;
    }
}

// This function will be part of uiTask
void handleEncoder_ui() {
    long pulses_isr;

    // Atomically copy the raw pulse count from the ISR and reset it
    noInterrupts();
    pulses_isr = encoder_raw_pulses;
    encoder_raw_pulses = 0;
    interrupts(); 

    // --- This section for dynamic speed control can be uncommented later if needed ---
    // int eff_pulses_step = FINE_CONTROL_PULSES_PER_STEP;
    // if (pulses_isr != 0) {
    //     acc_raw_pulses += pulses_isr;
    //     long speed_ind = abs(pulses_isr);
    //     if (speed_ind < SLOW_SPEED_MAX_PULSES_INTERVAL) eff_pulses_step = FINE_CONTROL_PULSES_PER_STEP;
    //     else if (speed_ind < MEDIUM_SPEED_MAX_PULSES_INTERVAL) eff_pulses_step = MEDIUM_SPEED_PULSES_PER_STEP;
    //     else eff_pulses_step = FAST_SPEED_PULSES_PER_STEP;
    // }
    // if (eff_pulses_step == 0) eff_pulses_step = FINE_CONTROL_PULSES_PER_STEP;
    //
    // int ui_steps = 0;
    // while (acc_raw_pulses >= eff_pulses_step) { ui_steps++; acc_raw_pulses -= eff_pulses_step; }
    // while (acc_raw_pulses <= -eff_pulses_step) { ui_steps--; acc_raw_pulses += eff_pulses_step; }
    // --- For now, we use a simpler 1-to-1 pulse to step conversion ---
    int ui_steps = pulses_isr / 4; // A common simplification for quadrature encoders

    if (ui_steps == 0) return;

    bool item_changed = false;
    if (currentMenuState == STATE_MENU_NAV) {
        isTextScrollingActive = false; 
        const MenuItem* activeList = nullptr;
        int* listSize = nullptr;
        int* selIdx = nullptr;
        bool* redrawFlag = nullptr;

        if (currentMenuLevel == 1) {
            activeList = L1_activeMenuItems;
            listSize = &L1_numActiveItems;
            selIdx = &L1_selectedIndex;
            redrawFlag = &needsRedrawOLED3; 
        } else if (currentMenuLevel == 2) {
            activeList = L2_activeMenuItems;
            listSize = &L2_numActiveItems;
            selIdx = &L2_selectedIndex;
            redrawFlag = &needsRedrawOLED2; 
        } else if (currentMenuLevel == 3) {
            activeList = L3_activeMenuItems;
            listSize = &L3_numActiveItems;
            selIdx = &L3_selectedIndex;
            redrawFlag = &needsRedrawOLED1; 
        }

        if (activeList && listSize && selIdx && *listSize > 0) {
            int old = *selIdx;
            *selIdx += ui_steps;
            *selIdx = constrain(*selIdx, 0, *listSize - 1);
            if (old != *selIdx) item_changed = true;
            if (item_changed && redrawFlag) *redrawFlag = true;
        }
    } else if (currentMenuState == STATE_SETTINGS_RTC_ADJUST) {
        uint16_t oy = rtcSet_year;
        uint8_t om = rtcSet_month, od = rtcSet_day, oh = rtcSet_hour, oi = rtcSet_minute;

        if (rtcSet_focusedField == RTC_FOCUS_DAY) rtcSet_day = constrain(rtcSet_day + ui_steps, 1, 31); 
        else if (rtcSet_focusedField == RTC_FOCUS_MONTH) rtcSet_month = constrain(rtcSet_month + ui_steps, 1, 12); 
        else if (rtcSet_focusedField == RTC_FOCUS_YEAR) rtcSet_year = constrain(rtcSet_year + ui_steps, 2024, 2099); 
        else if (rtcSet_focusedField == RTC_FOCUS_HOUR) rtcSet_hour = constrain(rtcSet_hour + ui_steps, 0, 23); 
        else if (rtcSet_focusedField == RTC_FOCUS_MINUTE) rtcSet_minute = constrain(rtcSet_minute + ui_steps, 0, 59); 

        if (oy != rtcSet_year || om != rtcSet_month || od != rtcSet_day || oh != rtcSet_hour || oi != rtcSet_minute) item_changed = true;
        if (item_changed) needsRedrawOLED1 = true; 
    }
}