#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include "dio.h" 

// Define which Port and Pin the Buzzer is connected to.
// Based on your previous code, you were using PF1 for Red LED/Buzzer.
// If you want them separate, you might need a different pin, but let's assume PF1 for now.
#define BUZZER_PORT   PORTE
#define BUZZER_PIN    PIN4

void Buzzer_Init(void);
void Buzzer_Beep(uint32_t duration_ms); // Short beep for key press

#endif