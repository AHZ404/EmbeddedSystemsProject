#ifndef UART_H_
#define UART_H_

#include <stdint.h>

void UART0_Init(void);
void UART0_SendChar(char c);
void UART0_SendString(char *str);
char UART0_ReceiveChar(void);
uint8_t UART0_DataAvailable(void);
uint8_t UART0_ReceiveString(char *buffer, uint8_t max_len);
void UART0_FlushRX(void);

#endif