#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "systick.h"
#include "dio.h"

volatile uint32_t msTicks = 0;
static uint8_t interruptMode = 0;

void SysTick_Init(uint32_t reload, uint8_t mode)
{
    interruptMode = mode;

    NVIC_ST_CTRL_R = 0;               // Disable SysTick
    NVIC_ST_RELOAD_R = reload - 1;    // Set reload value
    NVIC_ST_CURRENT_R = 0;            // Clear current

    if (mode == SYSTICK_INT)
    {
        NVIC_ST_CTRL_R = 0x07;        // ENABLE | TICKINT | CLK_SRC
    }
    else
    {
        NVIC_ST_CTRL_R = 0x05;        // ENABLE | CLK_SRC (no interrupt)
    }
}

void DelayMs(uint32_t ms)
{
    if (interruptMode == SYSTICK_NOINT)
    {
        // POLLING MODE - actively check COUNT flag
        for (uint32_t i = 0; i < ms; i++)
        {
            // Wait until COUNT flag is set (timer reached zero)
            while ((NVIC_ST_CTRL_R & (1 << 16)) == 0);
            // Clear the flag by writing to CURRENT register
            NVIC_ST_CURRENT_R = 0;
        }
    }
}

/* SysTick Interrupt Handler */
void SystickHandler(void)
{
    /* Toggle Green LED every SysTick interrupt */
    DIO_TogglePin(PORTF, PIN3);
    DIO_TogglePin(PORTF, PIN3);
    DIO_TogglePin(PORTF, PIN3);
}
