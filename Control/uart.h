#ifndef UART_H
#define UART_H

// UART initialization function
void UART2_Init(void);

// Send a single character via UART
void UART2_SendChar(char c);

// Receive a single character via UART (blocking)
char UART2_ReceiveChar(void);

// Send a string via UART
void UART2_SendString(char *str);

// Receive a single character with timeout to prevent deadlock
int UART2_ReceiveCharTimeout(char *result, int timeout_ms);

#endif // UART_H
