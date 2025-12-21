#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "uart.h"      // Must contain UART0_Init (which maps to UART2)
#include "dio.h"
#include "systick.h"   
#include "eeprom.h"
#include "buzzer.h"
#include "Servo.h"

/* --- DEFINES --- */
#define PASSWORD_MAX_LENGTH     20
#define EEPROM_PASSWORD_BLOCK   0
#define EEPROM_PASSWORD_OFFSET  0
#define EEPROM_TIMEOUT_BLOCK    1
#define EEPROM_TIMEOUT_OFFSET   0

/* --- FUNCTION PROTOTYPES --- */
void System_Init(void);
// Note: We use the names from your existing uart.c, even if they say UART0/UART1
// The implementation inside uart.c targets UART2 (PD6/PD7) as verified.
void UART2_Init(void);             
char UART2_ReadChar(void);
void UART2_SendString(char *str);
int  UART2_Available(void);

void Delay_ms(uint32_t ms);
void Delay_us(uint32_t us);
int  stringToInt(const char* str);
void Servo_Update(int open);

/* --- GLOBAL VARIABLES --- */
char master_password[PASSWORD_MAX_LENGTH];
uint32_t auto_lock_timeout = 5;
int servo_open = 0; // 0 = Closed (0 deg), 1 = Open (90 deg)
int authenticated = 0; // 0 = Not authenticated, 1 = Authenticated for settings changes

int main(void)
{
    // 1. Initialize Hardware
    System_Init();
    Buzzer_Init();
    Servo_Init();
    // 2. Initialize UART FIRST before anything else
    UART2_Init(); // Initializes UART2 (PD6/PD7)
    
    // Send startup message to verify UART is working
    UART2_SendString("CONTROL_READY\n");

    // 3. Initialize EEPROM
    if(EEPROM_Init() != EEPROM_SUCCESS) {
        // Fatal Error: Turn on Red LED and signal via UART
        GPIO_PORTF_DATA_R |= 0x02;  // Red LED
        UART2_SendString("EEPROM_ERROR\n");
        while(1); 
    }

    // 4. Load Password from EEPROM
    uint8_t read_buffer[PASSWORD_MAX_LENGTH];
    memset(read_buffer, 0, PASSWORD_MAX_LENGTH);

    if(EEPROM_ReadBuffer(EEPROM_PASSWORD_BLOCK, EEPROM_PASSWORD_OFFSET, read_buffer, PASSWORD_MAX_LENGTH) == EEPROM_SUCCESS) {
        strncpy(master_password, (char*)read_buffer, PASSWORD_MAX_LENGTH);
    }
    
    // Set default "12345" if EEPROM is empty (0xFF) or null
    if(master_password[0] == '\0' || (unsigned char)master_password[0] == 0xFF) {
        strcpy(master_password, "12345");
    }

    // 5. Load Timeout from EEPROM
    EEPROM_ReadWord(EEPROM_TIMEOUT_BLOCK, EEPROM_TIMEOUT_OFFSET, &auto_lock_timeout);
    // Sanity check: if invalid, set default 5 seconds
    if(auto_lock_timeout == 0xFFFFFFFF || auto_lock_timeout > 60) {
        auto_lock_timeout = 5;
    }

    // UART Buffer
    char rx_buffer[50];
    int rx_index = 0;

    while(1)
        {
            // --- 1. NON-BLOCKING UART RECEIVE ---
            while(UART2_Available())
            {
                char c = UART2_ReadChar(); 
                
                // --- SINGLE CHARACTER COMMANDS ---
                
                // 1. Lockout Signal
                if (c == 'L') 
                {
                    GPIO_PORTF_DATA_R |= 0x02; // Red LED On
                    Buzzer_Beep(1000);         // Beep 1s
                    GPIO_PORTF_DATA_R &= ~0x02; // Red LED Off
                    rx_index = 0; memset(rx_buffer, 0, sizeof(rx_buffer));
                    continue; 
                }
                

            
            if(c == '\n') // End of command
            {
                rx_buffer[rx_index] = '\0'; // Null terminate
                
                // --- PARSE COMMAND ---
                
                // A. SET NEW PASSWORD
                if(strncmp(rx_buffer, "SETPWD:", 7) == 0) 
                {
                    char *new_pass = rx_buffer + 7;
                    if(strlen(new_pass) < PASSWORD_MAX_LENGTH) {
                        strcpy(master_password, new_pass);
                        
                        // Prepare buffer for EEPROM
                        memset(read_buffer, 0, PASSWORD_MAX_LENGTH);
                        strcpy((char*)read_buffer, master_password);
                        
                        // Write to EEPROM
                        if(EEPROM_WriteBuffer(EEPROM_PASSWORD_BLOCK, EEPROM_PASSWORD_OFFSET, read_buffer, PASSWORD_MAX_LENGTH) == EEPROM_SUCCESS) {
                            UART2_SendString("PWD_SAVED\n");
                            // Success Signal: Green LED Flash
                            GPIO_PORTF_DATA_R |= 0x08; 
                            Delay_ms(1000); 
                            GPIO_PORTF_DATA_R &= ~0x08;
                        } else {
                            UART2_SendString("PWD_ERROR\n");
                            // Error Signal: Red LED Flash
                            GPIO_PORTF_DATA_R |= 0x02; 
                            Delay_ms(1000); 
                            GPIO_PORTF_DATA_R &= ~0x02;
                        }
                    } else {
                        UART2_SendString("PWD_TOO_LONG\n");
                    }
                }
                // B. SET TIMEOUT (only accept if authenticated)
                else if(strncmp(rx_buffer, "TIMEOUT:", 8) == 0)
                {
                    if(authenticated == 1) // Only allow if user has verified password
                    {
                        auto_lock_timeout = stringToInt(rx_buffer + 8);
                        if(EEPROM_WriteWord(EEPROM_TIMEOUT_BLOCK, EEPROM_TIMEOUT_OFFSET, auto_lock_timeout) == EEPROM_SUCCESS) {
                            UART2_SendString("TIMEOUT_SAVED\n");
                        } else {
                            UART2_SendString("TIMEOUT_ERROR\n");
                        }
                        authenticated = 0; // Clear authentication flag after use
                    }
                    else
                    {
                        UART2_SendString("TIMEOUT_DENIED\n"); // User not authenticated
                    }
                }
                // C. AUTHENTICATE PASSWORD FOR SETTINGS (no door open)
                else if(strncmp(rx_buffer, "VERIFYPWD:", 10) == 0)
                {
                    // Check against current master password
                    if(strcmp(master_password, rx_buffer + 10) == 0) {
                        UART2_SendString("AUTH_OK\n");
                        authenticated = 1; // Set authentication flag for settings changes
                        // Note: No door open, just authenticate for settings
                    } else {
                        UART2_SendString("AUTH_FAILED\n");
                        authenticated = 0;
                    }
                }
                // D. VERIFY PASSWORD (opens door)
                else if(strncmp(rx_buffer, "VERIFY:", 7) == 0)
                {
                    // Check against current master password
                    if(strcmp(master_password, rx_buffer + 7) == 0) {
                        UART2_SendString("ALLOW\n");
                        GPIO_PORTF_DATA_R |= 0x08; // Green LED ON
                        authenticated = 1; // Set authentication flag for settings changes
                        
                        // 1. Clear buffer immediately to ensure we catch the fresh "CLOSE" command
                        rx_index = 0;
                        memset(rx_buffer, 0, sizeof(rx_buffer));

                        // 2. DOOR OPEN LOOP 
                        // Stay here until "CLOSE" is received
                        while(1) 
                        {
                             // Keep energizing the Servo to hold it Open (90 deg)
                             Servo_SetAngle(90); 
                             
                             // Check UART for "CLOSE" command
                             if(UART2_Available()) 
                             {
                                 char c = UART2_ReadChar();
                                 if(c == '\n') 
                                 {
                                     rx_buffer[rx_index] = '\0'; // Null terminate
                                     
                                     // --- Check for CLOSE command ---
                                     if(strncmp(rx_buffer, "CLOSE", 5) == 0) 
                                     {
                                         Servo_SetAngle(0);           // Lock Door
                                         GPIO_PORTF_DATA_R &= ~0x08;  // Green LED OFF
                                         authenticated = 0; // Clear authentication flag when exiting door open state
                                         
                                         // Clear buffer and Break the loop to return to main idle state
                                         rx_index = 0;
                                         memset(rx_buffer, 0, sizeof(rx_buffer));
                                         break; 
                                     }
                                     
                                     // Reset buffer if message was not CLOSE
                                     rx_index = 0; 
                                     memset(rx_buffer, 0, sizeof(rx_buffer));
                                 }
                                 else if(rx_index < 49) {
                                     rx_buffer[rx_index++] = c;
                                 }
                             }
                        }
                    } else {
                        UART2_SendString("DENY\n");
                        authenticated = 0; // Clear authentication flag on failed password
                        GPIO_PORTF_DATA_R |= 0x02; // Red LED ON
                        Delay_ms(500);
                        GPIO_PORTF_DATA_R &= ~0x02;
                    }
                }
                
                rx_index = 0; // Reset buffer after processing
                memset(rx_buffer, 0, sizeof(rx_buffer));
            }
            else if(rx_index < 49) 
            {
                rx_buffer[rx_index++] = c;
            }
        }

        
    }
}

void System_Init(void) {
    // Enable GPIO Port B (Servo), D (UART2), and F (LEDs/Buzzer)
    SYSCTL_RCGCGPIO_R |= 0x2A;  // Bits: Port A(1), B(2), D(4), F(8) = 0b01010 = 0x0A, actually need B and F and D, so 0b00110 for B,F is 0x22, add D which is bit 3 = 0x08, so 0x22 | 0x08 = 0x2A
    while((SYSCTL_PRGPIO_R & 0x2A) == 0);

    // Port D for UART2 is initialized in uart.c, but ensure it's enabled
    GPIO_PORTD_DIR_R &= ~0xC0;  // PD6/PD7 as inputs (UART RX/TX are inputs to the GPIO)
    GPIO_PORTD_DEN_R |= 0xC0;   // Enable digital for PD6/PD7

    // PB6 - Servo (Output)
    GPIO_PORTB_DIR_R |= 0x40;
    GPIO_PORTB_DEN_R |= 0x40;

    // PF1 (Red/Buzzer), PF2 (Blue), PF3 (Green)
    GPIO_PORTF_DIR_R |= 0x0E;
    GPIO_PORTF_DEN_R |= 0x0E;
    GPIO_PORTF_DATA_R &= ~0x0E;
}

/* --- LOCAL HELPER FUNCTIONS --- */

void Delay_ms(uint32_t ms) {
    volatile uint32_t i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 3180; j++);
}
/*
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
*/
void Delay_us(uint32_t us) {
    volatile uint32_t i, j;
    for(i = 0; i < us; i++)
        for(j = 0; j < 3; j++);
}

int stringToInt(const char* str) {
    int res = 0;
    while(*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

void Servo_Update(int open) {
    // Standard Servo: 50Hz (20ms period)
    GPIO_PORTB_DATA_R |= 0x40; // High
    if(open) {
        Delay_us(1500); // 90 degrees (~1.5ms)
        GPIO_PORTB_DATA_R &= ~0x40; // Low
        Delay_us(18500);
    } else {
        Delay_us(1000); // 0 degrees (~1.0ms)
        GPIO_PORTB_DATA_R &= ~0x40; // Low
        Delay_us(19000);
    }
}
/* Wrapper to satisfy the Linker. 
   uart.c is asking for "delayMs", so we pass it to our working "Delay_ms".
*/
void delayMs(uint32_t n)
{
    Delay_ms(n); 
}