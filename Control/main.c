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

/* --- MAGIC NUMBER CONSTANTS (VIOLATION FIX #3) --- */
#define GPIO_RED_LED            0x02U
#define GPIO_GREEN_LED          0x08U
#define GPIO_LED_ALL            0x0EU
#define GPIO_PORTD_UART_MASK    0xC0U
#define GPIO_PORTB_SERVO_MASK   0x40U
#define RX_BUFFER_SIZE          50U
#define RX_BUFFER_MAX_INDEX     49U
#define SYSCTL_GPIO_ENABLE_MASK 0x2AU
#define DELAY_CALIBRATION_MS    3180U
#define DELAY_CALIBRATION_US    3U
#define SERVO_PULSE_90DEG       1500U
#define SERVO_PULSE_0DEG        1000U
#define SERVO_PERIOD_HIGH_90    18500U
#define SERVO_PERIOD_HIGH_0     19000U

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

    /* --- VIOLATION FIX #1 (MISRA C 2012 Rule 21.3) --- */
    /* BEFORE: Used strncpy without null termination guarantee */
    /* AFTER: Added explicit buffer zeroing and null termination */
    uint8_t read_buffer[PASSWORD_MAX_LENGTH];
    memset(read_buffer, 0, PASSWORD_MAX_LENGTH);

    if(EEPROM_ReadBuffer(EEPROM_PASSWORD_BLOCK, EEPROM_PASSWORD_OFFSET, read_buffer, PASSWORD_MAX_LENGTH) == EEPROM_SUCCESS) {
        /* Safe string copy with explicit null termination */
        strncpy(master_password, (char*)read_buffer, PASSWORD_MAX_LENGTH - 1U);
        master_password[PASSWORD_MAX_LENGTH - 1U] = '\0'; /* Ensure null termination */
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

    /* UART Buffer with proper sized constant (VIOLATION FIX #3) */
    char rx_buffer[RX_BUFFER_SIZE];
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
                    GPIO_PORTF_DATA_R |= GPIO_RED_LED;    /* Red LED On (VIOLATION FIX #3) */
                    Buzzer_Beep(1000);                    /* Beep 1s */
                    GPIO_PORTF_DATA_R &= ~GPIO_RED_LED;   /* Red LED Off (VIOLATION FIX #3) */
                    rx_index = 0;
                    memset(rx_buffer, 0, sizeof(rx_buffer));
                    continue; 
                }
                

            
            if(c == '\n') // End of command
            {
                rx_buffer[rx_index] = '\0'; // Null terminate
                
                // --- PARSE COMMAND ---
                
                /* A. SET NEW PASSWORD */
                if(strncmp(rx_buffer, "SETPWD:", 7) == 0) 
                {
                    char *new_pass = rx_buffer + 7;
                    if(strlen(new_pass) < PASSWORD_MAX_LENGTH) {
                        /* VIOLATION FIX #1 (MISRA C 2012 Rule 21.3): Replace unsafe strcpy with strncpy and explicit null termination */
                        strncpy(master_password, new_pass, PASSWORD_MAX_LENGTH - 1U);
                        master_password[PASSWORD_MAX_LENGTH - 1U] = '\0'; /* Ensure null termination */
                        
                        /* Prepare buffer for EEPROM with safe copy */
                        memset(read_buffer, 0, PASSWORD_MAX_LENGTH);
                        strncpy((char*)read_buffer, master_password, PASSWORD_MAX_LENGTH - 1U);
                        ((char*)read_buffer)[PASSWORD_MAX_LENGTH - 1U] = '\0';
                        
                        /* Write to EEPROM */
                        if(EEPROM_WriteBuffer(EEPROM_PASSWORD_BLOCK, EEPROM_PASSWORD_OFFSET, read_buffer, PASSWORD_MAX_LENGTH) == EEPROM_SUCCESS) {
                            UART2_SendString("PWD_SAVED\n");
                            /* Success Signal: Green LED Flash (VIOLATION FIX #3) */
                            GPIO_PORTF_DATA_R |= GPIO_GREEN_LED; 
                            Delay_ms(1000); 
                            GPIO_PORTF_DATA_R &= ~GPIO_GREEN_LED;
                        } else {
                            UART2_SendString("PWD_ERROR\n");
                            /* Error Signal: Red LED Flash (VIOLATION FIX #3) */
                            GPIO_PORTF_DATA_R |= GPIO_RED_LED; 
                            Delay_ms(1000); 
                            GPIO_PORTF_DATA_R &= ~GPIO_RED_LED;
                        }
                    } else {
                        UART2_SendString("PWD_TOO_LONG\n");
                    }
                }
                /* B. SET TIMEOUT (only accept if authenticated) */
                else if(strncmp(rx_buffer, "TIMEOUT:", 8) == 0)
                {
                    /* VIOLATION FIX #4 (CERT C DCL04-C): Add explicit comparison against enumerated value */
                    if(authenticated != 0) /* Only allow if user has verified password */
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
                /* C. AUTHENTICATE PASSWORD FOR SETTINGS (no door open) */
                else if(strncmp(rx_buffer, "VERIFYPWD:", 10) == 0)
                {
                    /* Check against current master password */
                    if(strcmp(master_password, rx_buffer + 10) == 0) {
                        UART2_SendString("AUTH_OK\n");
                        authenticated = 1; /* Set authentication flag for settings changes */
                        // Note: No door open, just authenticate for settings
                    } else {
                        UART2_SendString("AUTH_FAILED\n");
                        authenticated = 0;
                    }
                }
                /* D. VERIFY PASSWORD (opens door) */
                else if(strncmp(rx_buffer, "VERIFY:", 7) == 0)
                {
                    /* Check against current master password */
                    if(strcmp(master_password, rx_buffer + 7) == 0) {
                        UART2_SendString("ALLOW\n");
                        GPIO_PORTF_DATA_R |= GPIO_GREEN_LED; /* Green LED ON (VIOLATION FIX #3) */
                        authenticated = 1; /* Set authentication flag for settings changes */
                        
                        /* 1. Clear buffer immediately to ensure we catch the fresh "CLOSE" command */
                        rx_index = 0;
                        memset(rx_buffer, 0, sizeof(rx_buffer));

                        /* 2. DOOR OPEN LOOP - Stay here until "CLOSE" is received */
                        while(1) 
                        {
                             /* Keep energizing the Servo to hold it Open (90 deg) */
                             Servo_SetAngle(90); 
                             
                             /* Check UART for "CLOSE" command */
                             if(UART2_Available()) 
                             {
                                 char c = UART2_ReadChar();
                                 if(c == '\n') 
                                 {
                                     rx_buffer[rx_index] = '\0'; /* Null terminate */
                                     
                                     /* Check for CLOSE command */
                                     if(strncmp(rx_buffer, "CLOSE", 5) == 0) 
                                     {
                                         Servo_SetAngle(0);                    /* Lock Door */
                                         GPIO_PORTF_DATA_R &= ~GPIO_GREEN_LED; /* Green LED OFF (VIOLATION FIX #3) */
                                         authenticated = 0; /* Clear authentication flag when exiting door open state */
                                         
                                         /* Clear buffer and break the loop to return to main idle state */
                                         rx_index = 0;
                                         memset(rx_buffer, 0, sizeof(rx_buffer));
                                         break; 
                                     }
                                     
                                     /* Reset buffer if message was not CLOSE */
                                     rx_index = 0; 
                                     memset(rx_buffer, 0, sizeof(rx_buffer));
                                 }
                                 else if(rx_index < RX_BUFFER_MAX_INDEX) { /* VIOLATION FIX #3: Use constant instead of magic 49 */
                                     rx_buffer[rx_index++] = c;
                                 }
                             }
                        }
                    } else {
                        UART2_SendString("DENY\n");
                        authenticated = 0; /* Clear authentication flag on failed password */
                        GPIO_PORTF_DATA_R |= GPIO_RED_LED;   /* Red LED ON (VIOLATION FIX #3) */
                        Delay_ms(500);
                        GPIO_PORTF_DATA_R &= ~GPIO_RED_LED;  /* VIOLATION FIX #3 */
                    }
                }
                
                rx_index = 0; /* Reset buffer after processing */
                memset(rx_buffer, 0, sizeof(rx_buffer));
            }
            else if(rx_index < RX_BUFFER_MAX_INDEX) /* VIOLATION FIX #3: Use constant instead of magic 49 */
            {
                rx_buffer[rx_index++] = c;
            }
        }

        
    }
}

void System_Init(void) {
    /* Enable GPIO Port B (Servo), D (UART2), and F (LEDs/Buzzer) */
    SYSCTL_RCGCGPIO_R |= SYSCTL_GPIO_ENABLE_MASK;  /* VIOLATION FIX #3: Ports B,D,F enable */
    while((SYSCTL_PRGPIO_R & SYSCTL_GPIO_ENABLE_MASK) == 0);

    /* Port D for UART2 is initialized in uart.c, but ensure it's enabled */
    GPIO_PORTD_DIR_R &= ~GPIO_PORTD_UART_MASK;  /* PD6/PD7 as inputs (UART RX/TX are inputs to the GPIO) */
    GPIO_PORTD_DEN_R |= GPIO_PORTD_UART_MASK;   /* Enable digital for PD6/PD7 */

    /* PB6 - Servo (Output) */
    GPIO_PORTB_DIR_R |= GPIO_PORTB_SERVO_MASK;
    GPIO_PORTB_DEN_R |= GPIO_PORTB_SERVO_MASK;

    /* PF1 (Red/Buzzer), PF2 (Blue), PF3 (Green) */
    GPIO_PORTF_DIR_R |= GPIO_LED_ALL;
    GPIO_PORTF_DEN_R |= GPIO_LED_ALL;
    GPIO_PORTF_DATA_R &= ~GPIO_LED_ALL;
}

/* --- LOCAL HELPER FUNCTIONS --- */

/* VIOLATION FIX #3: Replace magic numbers with constants */
void Delay_ms(uint32_t ms) {
    volatile uint32_t i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < DELAY_CALIBRATION_MS; j++);
}

/* VIOLATION FIX #5: Removed unused commented code block (MISRA C 2012 Rule 2.1) */
/* Original DelayMs function with SYSTICK was replaced by Delay_ms */

void Delay_us(uint32_t us) {
    volatile uint32_t i, j;
    for(i = 0; i < us; i++)
        for(j = 0; j < DELAY_CALIBRATION_US; j++);
}

/* VIOLATION FIX #2: Helper function already declared in function prototypes section */
/* Moved prototype to the top of file to comply with CERT C EXP05-C */
int stringToInt(const char* str) {
    int res = 0;
    while(*str >= '0' && *str <= '9') {
        res = (res * 10) + (*str - '0'); /* VIOLATION FIX #6: Added parentheses for clarity (MISRA C 2012 Rule 12.1) */
        str++;
    }
    return res;
}

/* VIOLATION FIX #3: Replace magic constants with named constants */
void Servo_Update(int open) {
    /* Standard Servo: 50Hz (20ms period) */
    GPIO_PORTB_DATA_R |= GPIO_PORTB_SERVO_MASK; /* High */
    if(open) {
        Delay_us(SERVO_PULSE_90DEG);     /* 90 degrees (~1.5ms) */
        GPIO_PORTB_DATA_R &= ~GPIO_PORTB_SERVO_MASK; /* Low */
        Delay_us(SERVO_PERIOD_HIGH_90);
    } else {
        Delay_us(SERVO_PULSE_0DEG);      /* 0 degrees (~1.0ms) */
        GPIO_PORTB_DATA_R &= ~GPIO_PORTB_SERVO_MASK; /* Low */
        Delay_us(SERVO_PERIOD_HIGH_0);
    }
}
/* Wrapper function to satisfy the linker requirements from uart.c */
void delayMs(uint32_t n)
{
    Delay_ms(n);
}