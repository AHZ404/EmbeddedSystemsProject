#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

#define SYSTICK_NOINT   0
#define SYSTICK_INT     1

void SysTick_Init(uint32_t reload, uint8_t mode);
void DelayMs(uint32_t ms);

#endif
