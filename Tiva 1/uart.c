#include "tm4c123gh6pm.h"
#include "uart.h"

// Initialize UART0 at 115200 baud rate
void UART0_Init(void)
{
    SYSCTL_RCGCUART_R |= 1;
    SYSCTL_RCGCGPIO_R |= 1;
    
    UART0_CTL_R &= ~1;
    UART0_IBRD_R = 104;
    UART0_FBRD_R = 11;
    UART0_LCRH_R = 0x70;
    UART0_CTL_R = 0x301;
    
    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R |= 0x11;
    GPIO_PORTA_DEN_R |= 0x03;
}

// Send one character
void UART0_SendChar(char c)
{
    while((UART0_FR_R & 0x20));
    UART0_DR_R = c;
}

// Send a string
void UART0_SendString(char *str)
{
    while(*str)
        UART0_SendChar(*str++);
}

// Receive one character (waits until data arrives)
char UART0_ReceiveChar(void)
{
    while(UART0_FR_R & 0x10);
    return (char)(UART0_DR_R & 0xFF);
}

// Check if data is ready to read
uint8_t UART0_DataAvailable(void)
{
    return !(UART0_FR_R & 0x10);
}

// Receive a string until newline or buffer full
uint8_t UART0_ReceiveString(char *buffer, uint8_t max_len)
{
    uint8_t count = 0;
    char c;
    
    while(count < max_len - 1)
    {
        while(UART0_FR_R & 0x10);
        c = (char)(UART0_DR_R & 0xFF);
        
        if(c == '\n' || c == '\r')
            break;
            
        buffer[count++] = c;
    }
    
    buffer[count] = '\0';
    return count;
}

// Clear any data waiting in receive buffer
void UART0_FlushRX(void)
{
    while(UART0_DataAvailable())
    {
        volatile char temp = (char)(UART0_DR_R & 0xFF);
        (void)temp;
    }
}