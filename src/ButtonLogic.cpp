#include "ButtonLogic.h"
#include "globals.h"

// These variables are now PRIVATE to this file. No other module can see or change them.
static int topButtonStableState = HIGH;
static int lastTopButtonRawState = HIGH;
static unsigned long lastTopDebounceTime = 0;

static int midButtonStableState = HIGH;
static int lastMidButtonRawState = HIGH;
static unsigned long lastMidDebounceTime = 0;

static int botButtonStableState = HIGH;
static int lastBotButtonRawState = HIGH;
static unsigned long lastBotDebounceTime = 0;

static const long buttonDebounceDelay = 50;

// This is the public function that the rest of the program can call
void handleButtons_ui() {
    bool topAct = false, midAct = false, botAct = false;
    unsigned long curT = millis();

    // Top Button Logic
    int tr = digitalRead(BTN_TOP_PIN);
    if (tr != lastTopButtonRawState) lastTopDebounceTime = curT;
    if ((curT - lastTopDebounceTime) > buttonDebounceDelay) {
        if (tr != topButtonStableState) {
            topButtonStableState = tr;
            if (topButtonStableState == LOW) topAct = true;
        }
    }
    lastTopButtonRawState = tr;

    // Middle Button Logic
    int mr = digitalRead(BTN_MIDDLE_PIN);
    if (mr != lastMidButtonRawState) lastMidDebounceTime = curT;
    if ((curT - lastMidDebounceTime) > buttonDebounceDelay) {
        if (mr != midButtonStableState) {
            midButtonStableState = mr;
            if (midButtonStableState == LOW) midAct = true;
        }
    }
    lastMidButtonRawState = mr;

    // Bottom Button Logic
    int br = digitalRead(BTN_BOTTOM_PIN);
    if (br != lastBotButtonRawState) lastBotDebounceTime = curT;
    if ((curT - lastBotDebounceTime) > buttonDebounceDelay) {
        if (br != botButtonStableState) {
            botButtonStableState = br;
            if (botButtonStableState == LOW) botAct = true;
        }
    }
    lastBotButtonRawState = br;

    if (topAct || midAct || botAct) {
        needsRedrawOLED1 = needsRedrawOLED2 = needsRedrawOLED3 = true;
        isTextScrollingActive = false;

        if (currentMenuState == STATE_HOME_SCREEN) {
            if (botAct) {
                currentMenuState = STATE_MENU_NAV;
                currentMenuLevel = 1;
                L1_activeMenuItems = mainMenu_Items;
                L1_numActiveItems = numMainMenuItems;
                L1_selectedIndex = 0;
                L1_selectedLabelForOLED3Display = nullptr;
                L1_parentIndexOfActiveL2 = -1;
                L2_activeMenuItems = nullptr;
                L2_selectedLabelForOLED2Display = nullptr;
                L2_parentIndexOfActiveL3 = -1;
                L3_activeMenuItems = nullptr;
            }
        } else if (currentMenuState == STATE_MENU_NAV) {
            const MenuItem* selItem = nullptr;
            if (topAct) {
                if (currentMenuLevel == 3) {
                    currentMenuLevel = 2;
                    L3_activeMenuItems = nullptr;
                } else if (currentMenuLevel == 2) {
                    currentMenuLevel = 1;
                    L2_activeMenuItems = nullptr;
                    L1_selectedLabelForOLED3Display = nullptr;
                    L1_parentIndexOfActiveL2 = -1;
                } else if (currentMenuLevel == 1) {
                    currentMenuState = STATE_HOME_SCREEN;
                    currentMenuLevel = 0;
                    L1_activeMenuItems = nullptr;
                }
            } else if (midAct) {
                currentMenuState = STATE_HOME_SCREEN;
                currentMenuLevel = 0;
                L1_activeMenuItems = nullptr;
                L2_activeMenuItems = nullptr;
                L3_activeMenuItems = nullptr;
            } else if (botAct) {
                if (currentMenuLevel == 1 && L1_activeMenuItems && L1_selectedIndex < L1_numActiveItems) {
                    selItem = &L1_activeMenuItems[L1_selectedIndex];
                    if (selItem->subMenu && selItem->numSubItems > 0) {
                        L1_selectedLabelForOLED3Display = selItem->label;
                        L1_parentIndexOfActiveL2 = L1_selectedIndex;
                        L2_activeMenuItems = selItem->subMenu;
                        L2_numActiveItems = selItem->numSubItems;
                        L2_selectedIndex = 0;
                        currentMenuLevel = 2;
                        L2_selectedLabelForOLED2Display = nullptr;
                        L2_parentIndexOfActiveL3 = -1;
                        L3_activeMenuItems = nullptr;
                    } else if (selItem->actionFunction) {
                        selItem->actionFunction();
                    } else if (selItem->targetStateOnSelect == STATE_HOME_SCREEN) {
                        currentMenuState = STATE_HOME_SCREEN;
                        currentMenuLevel = 0;
                    }
                } else if (currentMenuLevel == 2 && L2_activeMenuItems && L2_selectedIndex < L2_numActiveItems) {
                    selItem = &L2_activeMenuItems[L2_selectedIndex];
                    if (selItem->subMenu && selItem->numSubItems > 0) {
                        L2_selectedLabelForOLED2Display = selItem->label;
                        L2_parentIndexOfActiveL3 = L2_selectedIndex;
                        L3_activeMenuItems = selItem->subMenu;
                        L3_numActiveItems = selItem->numSubItems;
                        L3_selectedIndex = 0;
                        currentMenuLevel = 3;
                    } else if (selItem->actionFunction) {
                        selItem->actionFunction();
                    }
                } else if (currentMenuLevel == 3 && L3_activeMenuItems && L3_selectedIndex < L3_numActiveItems) {
                    selItem = &L3_activeMenuItems[L3_selectedIndex];
                    if (selItem->actionFunction) selItem->actionFunction();
                }
            }
        } else if (currentMenuState == STATE_SETTINGS_RTC_ADJUST) {
            // This logic will be updated later per the new UI plan
            if (topAct) {
                rtcSet_focusedField = (rtcSet_focusedField + 1) % NUM_RTC_FIELDS;
            } else if (midAct) {
                currentMenuState = previousMenuStateBeforeAction; // Simplified exit
            } else if (botAct) {
                // Placeholder for Save
            }
        } else if (currentMenuState == STATE_SETTINGS_BATTERY_INFO) {
            if (topAct || midAct || botAct) { // Any button to go back
                currentMenuState = previousMenuStateBeforeAction; // Simplified exit
            }
        }
    }
}