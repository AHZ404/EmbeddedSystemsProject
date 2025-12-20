#include "tm4c123gh6pm.h"
#include "uart.h"

void delayMs(int ms);  // Forward declaration

// NOTE: Function named UART0 but initializes UART2 (PD6/PD7)
void UART2_Init(void) // Renamed from UART0_Init to match the actual hardware
{
    // 1. Enable Clocks
    SYSCTL_RCGCUART_R |= 0x04;      // Enable UART2 (Bit 2)
    SYSCTL_RCGCGPIO_R |= 0x08;      // Enable Port D (Bit 3)
    
    // Wait for Port D to be ready
    while((SYSCTL_PRGPIO_R & 0x08) == 0);

    // =======================================================
    // FIX: UNLOCK PD7 (Required because PD7 is an NMI pin)
    // =======================================================
    GPIO_PORTD_LOCK_R = 0x4C4F434B; // Unlock GPIO Port D
    GPIO_PORTD_CR_R |= 0x80;        // Allow changes to PD7 (Bit 7)
    // =======================================================

    // 2. Disable UART2 during configuration
    UART2_CTL_R = 0;                

    // 3. Configure Baud Rate: 115200 @ 16MHz
    // IBRD = 8, FBRD = 44 (Calculations are correct)
    UART2_IBRD_R = 8;              
    UART2_FBRD_R = 44;              

    // 4. Configure Line Control: 8-bit, no parity, 1 stop, FIFOs
    UART2_LCRH_R = 0x70;
    
    // 5. Set Clock Source (Best practice)
    UART2_CC_R = 0x0;               // Use System Clock

    // 6. Configure UART2 Control Register
    // Enable UART (Bit 0), TX (Bit 9), RX (Bit 8) -> 0x301
    UART2_CTL_R = 0x301;            

    // 7. Configure GPIO Pins (PD6=Rx, PD7=Tx)
    GPIO_PORTD_AFSEL_R |= 0xC0;     // Enable Alt Function on PD6, PD7
    
    // Configure PCTL for UART on PD6 and PD7 (Value 1 in nibbles)
    // Clear nibbles for 6 and 7, then set them to 1
    GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0x00FFFFFF) | 0x11000000;
    
    GPIO_PORTD_DEN_R |= 0xC0;       // Enable Digital on PD6, PD7
}

void UART2_SendChar(char c)
{
    while((UART2_FR_R & 0x20) != 0); // Wait if TX FIFO is Full
    UART2_DR_R = c;
}

char UART2_ReceiveChar(void)
{
    while((UART2_FR_R & 0x10) != 0); // Wait if RX FIFO is Empty
    return (char)(UART2_DR_R & 0xFF);
}

void UART2_SendString(char *str)
{
    while(*str) {
        UART2_SendChar(*str++);
    }
    // Small delay after sending string to ensure transmission
    delayMs(10);
}

// Returns 1 if char received, 0 if timeout
int UART2_ReceiveCharTimeout(char *result, int timeout_ms)
{
    int elapsed = 0;
    // Check RX FIFO Empty bit (0x10). If set, FIFO is empty.
    while((UART2_FR_R & 0x10) != 0 && elapsed < timeout_ms) {
        delayMs(1); 
        elapsed++;
    }
    
    if((UART2_FR_R & 0x10) != 0) {
        return 0; // Timeout
    }
    
    *result = (char)(UART2_DR_R & 0xFF);
    return 1; // Success
}

// Check if data is available in RX FIFO (for non-blocking reads)
int UART2_Available(void)
{
    // Return 1 if RX FIFO is NOT empty (data available)
    // Bit 4 of FR register: 1 = FIFO empty, 0 = FIFO has data
    return ((UART2_FR_R & 0x10) == 0) ? 1 : 0;
}

// Non-blocking read from UART
char UART2_ReadChar(void)
{
    return (char)(UART2_DR_R & 0xFF);
}