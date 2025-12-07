#include "tm4c123gh6pm.h"
#include "lcd.h"
#include <stdint.h>

#define RS 0x01  // PB0
#define EN 0x02  // PB1

void delayMs(int n);
void delayUs(int n);

void LCD_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x02;  
    while((SYSCTL_PRGPIO_R & 0x02) == 0);

    GPIO_PORTB_DIR_R |= 0x3F;  
    GPIO_PORTB_DEN_R |= 0x3F;

    delayMs(20);
    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);
    delayMs(5);
}

void LCD_Command(unsigned char cmd)
{
    GPIO_PORTB_DATA_R = (GPIO_PORTB_DATA_R & ~0x3C) | ((cmd >> 2) & 0x3C);
    GPIO_PORTB_DATA_R &= ~RS;
    GPIO_PORTB_DATA_R |= EN;
    delayUs(1);
    GPIO_PORTB_DATA_R &= ~EN;

    GPIO_PORTB_DATA_R = (GPIO_PORTB_DATA_R & ~0x3C) | ((cmd << 2) & 0x3C);
    GPIO_PORTB_DATA_R |= EN;
    delayUs(1);
    GPIO_PORTB_DATA_R &= ~EN;

    delayMs(2);
}

void LCD_Char(unsigned char data)
{
    GPIO_PORTB_DATA_R = (GPIO_PORTB_DATA_R & ~0x3C) | ((data >> 2) & 0x3C);
    GPIO_PORTB_DATA_R |= RS;
    GPIO_PORTB_DATA_R |= EN;
    delayUs(1);
    GPIO_PORTB_DATA_R &= ~EN;

    GPIO_PORTB_DATA_R = (GPIO_PORTB_DATA_R & ~0x3C) | ((data << 2) & 0x3C);
    GPIO_PORTB_DATA_R |= EN;
    delayUs(1);
    GPIO_PORTB_DATA_R &= ~EN;

    delayMs(2);
}

void LCD_String(char *str)
{
    while(*str)
    {
        LCD_Char(*str++);
    }
}

void LCD_Clear(void)
{
    LCD_Command(0x01);
    delayMs(2);
}

void LCD_SetCursor(unsigned char row, unsigned char col)
{
    uint8_t address = (row == 1) ? (0x80 + col) : (0xC0 + col);
    LCD_Command(address);
}

void delayMs(int n)
{
    for(int i = 0; i < n; i++)
        for(int j = 0; j < 3180; j++);
}

void delayUs(int n)
{
    for(int i = 0; i < n; i++)
        for(int j = 0; j < 3; j++);
}
