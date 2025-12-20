#include "tm4c123gh6pm.h"
#include "keypad.h"
#include <stdint.h>

const char KEYS[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

void Keypad_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x05;

    // PORTC rows PC4-PC7 = outputs
    GPIO_PORTC_DIR_R |= 0xF0;
    GPIO_PORTC_DEN_R |= 0xF0;

    // PORTA columns PA2-PA5 = inputs + pullups
    GPIO_PORTA_DIR_R &= ~0x3C;
    GPIO_PORTA_DEN_R |= 0x3C;
    GPIO_PORTA_PUR_R |= 0x3C;
}

char Keypad_GetKey(void)
{
    for(int row = 0; row < 4; row++)
    {
        GPIO_PORTC_DATA_R = ~(1 << (row + 4)) & 0xF0;

        uint32_t col = GPIO_PORTA_DATA_R & 0x3C;
        
        if(col != 0x3C)
        {
            for(int c = 0; c < 4; c++)
            {
                if(!(col & (1 << (c + 2))))
                {
                    while(!(GPIO_PORTA_DATA_R & (1 << (c + 2))));
                    return KEYS[row][3-c];
                }
            }
        }
    }
    return 0;
}
