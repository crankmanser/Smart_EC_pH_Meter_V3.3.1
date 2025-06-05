#ifndef ENCODER_LOGIC_H
#define ENCODER_LOGIC_H

#include <Arduino.h> // Required for IRAM_ATTR

// Public functions provided by this module
void handleEncoder_ui();
void IRAM_ATTR customEncoderISR(); // The Interrupt Service Routine

#endif // ENCODER_LOGIC_H