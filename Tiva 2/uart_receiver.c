#include "tm4c123gh6pm.h"
#include "uart_receiver.h"
#include "password_db.h"
#include "dio.h"
#include <string.h>
#include <stdint.h>

static char rx_buffer[MAX_BUFFER_SIZE];
static uint8_t rx_index = 0;
static uint8_t receiving = 0;

void UARTReceiver_Init(void) {
    // Enable UART0
    SYSCTL_RCGCUART_R |= 0x01;
    SYSCTL_RCGCGPIO_R |= 0x01;
    
    // Wait for clock stability
    while((SYSCTL_PRUART_R & 0x01) == 0);
    while((SYSCTL_PRGPIO_R & 0x01) == 0);
    
    // Disable UART during configuration
    UART0_CTL_R &= ~0x01;
    
    // Configure baud rate (115200)
    UART0_IBRD_R = 8;
    UART0_FBRD_R = 44;
    
    // Configure line control
    UART0_LCRH_R = 0x70;
    
    // Enable UART, TX, RX
    UART0_CTL_R = 0x301;
    
    // Configure GPIO pins
    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R |= 0x11;
    GPIO_PORTA_DEN_R |= 0x03;
    
    // Initialize buffer
    rx_index = 0;
    receiving = 0;
}

void UARTReceiver_Process(void) {
    // Check if data is available
    if (UART0_FR_R & 0x10) {
        return; // No data
    }
    
    // Read character
    char c = (char)(UART0_DR_R & 0xFF);
    
    // Check for start of password transmission
    if (c == 'P') {
        receiving = 1;
        rx_index = 0;
        rx_buffer[0] = '\0';
        return;
    }
    
    if (receiving) {
        // Check for end of password
        if (c == '\n' || c == '\r' || rx_index >= MAX_BUFFER_SIZE - 1) {
            receiving = 0;
            rx_buffer[rx_index] = '\0';
            
            // Extract password (after "PASSWORD=")
            char* password_start = strstr(rx_buffer, "PASSWORD=");
            if (password_start) {
                password_start += 9; // Skip "PASSWORD="
                
                // Verify password
                uint8_t result = PasswordDB_Verify(password_start);
                
                // Send result back
                if (result == 1) {
                    // Correct password
                    UART0_DR_R = 'C'; // 'C' for Correct
                    // You can also activate door lock relay here
                    DIO_Init(PORTF, PIN1, OUTPUT); // PF1 for door lock relay
                    DIO_WritePin(PORTF, PIN1, HIGH); // Activate relay
                    
                    // Keep door open for 5 seconds
                    for(volatile int i = 0; i < 5000000; i++);
                    DIO_WritePin(PORTF, PIN1, LOW); // Deactivate relay
                    
                } else if (result == 2) {
                    // System locked
                    UART0_DR_R = 'L'; // 'L' for Locked
                } else {
                    // Incorrect password
                    UART0_DR_R = 'W'; // 'W' for Wrong
                }
            }
        } else {
            // Store character
            rx_buffer[rx_index++] = c;
        }
    }
}