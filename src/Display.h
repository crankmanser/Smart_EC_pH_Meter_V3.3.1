#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>
#include "globals.h" // For MenuItem struct

// --- From Sect 5: Icon Drawing & Battery Calculation ---
int calculateBatteryLevel(float v);
float estimateBatteryPercentage(float v);
void drawBatteryIcon(Adafruit_SSD1306& d, int x, int y, int l);
// ... other icon functions if we decide to make them public ...



// --- From Sect 9: Display Router ---
void updateDisplay_ui();

#endif // DISPLAY_H