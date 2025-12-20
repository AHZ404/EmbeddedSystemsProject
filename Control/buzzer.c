#include "buzzer.h"
#include "tm4c123gh6pm.h"
#include <stdint.h>
// External dependency: You likely have a Delay function available.
// If not, we can define a simple one here. 
void delayMs(uint32_t ms);
void Buzzer_Init(void)
{
    // 1. Enable Clock for Port E
    SYSCTL_RCGCGPIO_R |= 0x10; // 0x10 = Port E
    while((SYSCTL_PRGPIO_R & 0x10) == 0); // Wait for clock
    
    // 2. Configure PE4 as Output
    GPIO_PORTE_DIR_R |= 0x10;  // Pin 4 Output
    GPIO_PORTE_DEN_R |= 0x10;  // Pin 4 Digital Enable
    GPIO_PORTE_DATA_R &= ~0x10;// Initialize Low (Off)
}
void Buzzer_Beep(uint32_t duration_ms)
{
    GPIO_PORTE_DATA_R |= 0x10; // Turn ON (PE4 High)
    delayMs(duration_ms);
    GPIO_PORTE_DATA_R &= ~0x10; // Turn OFF (PE4 Low)
}