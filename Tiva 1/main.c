#include "lcd.h"
#include "keypad.h"
#include "uart.h"
#include "dio.h"
#include <string.h>

// Add these functions to uart.h:
// char UART0_ReceiveChar(void);
// uint8_t UART0_DataAvailable(void);

int main(void)
{
    LCD_Init();
    Keypad_Init();
    UART0_Init();
    
    // Initialize LED for status (PF3 - Green LED)
    DIO_Init(PORTF, PIN3, OUTPUT);
    DIO_WritePin(PORTF, PIN3, LOW);
    
    LCD_Clear();
    LCD_String("Enter Pass:");
    
    char pass[6]; // 5 digits + null terminator
    int index = 0;

    while(1)
    {
        char key = Keypad_GetKey();

        if(key)
        {
            if(key >= '0' && key <= '9')
            {
                if(index < 5)
                {
                    pass[index++] = key;
                    LCD_Char('*');
                }
            }
            else if(key == '#')
            {
                pass[index] = '\0';
                
                // Send password to Tiva 2
                UART0_SendString("PASSWORD=");
                UART0_SendString(pass);
                UART0_SendString("\n");
                
                // Wait for response with timeout
                uint32_t timeout = 0;
                char response = 0;
                
                while(timeout++ < 1000000) {
                    // Check if data available (you need to implement this)
                    if(UART0_DataAvailable()) {
                        response = UART0_ReceiveChar();
                        break;
                    }
                }
                
                LCD_Clear();
                
                if(response == 'C') {
                    // Correct password
                    LCD_String("Access Granted!");
                    DIO_WritePin(PORTF, PIN3, HIGH); // Green LED on
                    
                    // Wait and clear
                    for(volatile int i = 0; i < 5000000; i++);
                    DIO_WritePin(PORTF, PIN3, LOW);
                    
                } else if(response == 'W') {
                    // Wrong password
                    LCD_String("Wrong Password!");
                } else if(response == 'L') {
                    // System locked
                    LCD_String("System Locked!");
                } else {
                    // No response/timeout
                    LCD_String("No Response!");
                }
                
                // Reset for next attempt
                index = 0;
                DelayMs(2000);
                LCD_Clear();
                LCD_String("Enter Pass:");
            }
            else if(key == '*')
            {
                // Clear input
                index = 0;
                LCD_Clear();
                LCD_String("Enter Pass:");
            }
        }
    }
}