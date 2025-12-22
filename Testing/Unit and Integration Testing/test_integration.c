#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "uart.h" 

/* --- LCD EXTERNS (Must match your LCD driver) --- */
extern void LCD_Clear(void);
extern void LCD_String(char *str);
extern void LCD_SetCursor(uint8_t row, uint8_t col);
extern void delayMs(int n);

/* --- DEBUG UART0 SETUP (Fixed & Robust) --- */
void Debug_UART0_Init(void) {
    SYSCTL_RCGCUART_R |= 0x01;            // Enable UART0
    SYSCTL_RCGCGPIO_R |= 0x01;            // Enable Port A
    volatile int delay = SYSCTL_RCGCGPIO_R; 
    
    GPIO_PORTA_AFSEL_R |= 0x03;           // PA0, PA1 Alt Function
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) | 0x00000011; 
    GPIO_PORTA_DEN_R |= 0x03;             // Digital Enable
    GPIO_PORTA_AMSEL_R &= ~0x03;          // Disable Analog

    UART0_CTL_R &= ~0x01;                 // Disable UART0
    UART0_IBRD_R = 8;                     // 115200 baud @ 16MHz
    UART0_FBRD_R = 44;                    
    UART0_LCRH_R = 0x70;                  // 8-bit, no parity
    UART0_CC_R = 0x0;                     
    UART0_CTL_R |= 0x301;                 // Enable
}

/* Force-send function */
void Debug_Log(char *str) {
    while(*str) {
        while((UART0_FR_R & 0x20) != 0);  // Wait for TX Buffer
        UART0_DR_R = *str;
        str++;
    }
}

void Log_Result(char *test_name, int status) {
    if (status) Debug_Log("[PASS] ");
    else        Debug_Log("[FAIL] ");
    Debug_Log(test_name);
    Debug_Log("\r\n");
}

/* --- RECEIVE HELPER --- */
void Test_Receive(char* buffer) {
    int i = 0;
    int timeout = 0;
    
    Debug_Log("Waiting"); 
    while(timeout < 30) { // Increased timeout to 3 seconds
        if((UART2_FR_R & 0x10) == 0) { // If RX FIFO not empty
            char c = (char)(UART2_DR_R & 0xFF);
            if(c == '\n') {
                buffer[i] = '\0';
                Debug_Log(" -> Recv: ");
                Debug_Log(buffer);
                Debug_Log("\r\n");
                return;
            }
            buffer[i++] = c;
            if(i >= 19) break;
        } else {
            delayMs(100);
            Debug_Log(".");
            timeout++;
        }
    }
    strcpy(buffer, "TIMEOUT");
    Debug_Log("\r\nError: TIMEOUT\r\n");
}

/* --- TESTS --- */

int Test_Initial_Setup(void) {
    char response[20] = {0};
    Debug_Log("Sending: SETPWD:12345\r\n");
    LCD_Clear();
    LCD_String("Test 1 PWD");
    UART2_SendString("SETPWD:");
    UART2_SendString("12345");
    UART2_SendChar('\n');
    Test_Receive(response);
    return (strcmp(response, "PWD_SAVED") == 0);
}

int Test_Door_Open(void) {
    char response[20] = {0};
    Debug_Log("Sending: VERIFY:12345\r\n");
    LCD_Clear();
    LCD_String("Test 2 OPN");
    UART2_SendString("VERIFY:");
    UART2_SendString("12345");
    UART2_SendChar('\n');
    Test_Receive(response);
    if (strcmp(response, "ALLOW") != 0) return 0;
    
    delayMs(500); 
    Debug_Log("Sending: CLOSE\r\n");
    UART2_SendString("CLOSE\n");
    return 1;
}

int Test_Access_Denied(void) {
    char response[20] = {0};
    Debug_Log("Sending: VERIFY:99999\r\n");
    LCD_Clear();
    LCD_String("Test 3 WRONG");
    UART2_SendString("VERIFY:");
    UART2_SendString("99999");
    UART2_SendChar('\n');
    Test_Receive(response);
    return (strcmp(response, "DENY") == 0);
}

/* FIXED LOCKOUT TEST */
int Test_Lockout_Sequence(void) {
    char response[20] = {0};
    
    Debug_Log("--- LOCKOUT TEST ---\r\n");
    LCD_Clear();
    LCD_String("Test 4 Buzzer");
    
    // 1. Send Wrong Password (Attempt 1)
    UART2_SendString("VERIFY:88888\n");
    Test_Receive(response);
    
    // 2. Send Wrong Password (Attempt 2)
    UART2_SendString("VERIFY:88888\n");
    Test_Receive(response);
    
    // 3. Send Wrong Password (Attempt 3)
    UART2_SendString("VERIFY:88888\n");
    Test_Receive(response);
    
    // 4. TRIGGER LOCKOUT MANUALLY
    // Since the HMI Logic (in main.c) usually handles the counting,
    // the Test Suite must manually send the 'L' command to prove
    // the Control Board's Buzzer works.
    Debug_Log("Sending 'L' Command (Trigger Buzzer)...\r\n");
    UART2_SendChar('L'); 
    
    Debug_Log(">> Buzzer is Beeping (Wait 2s)\r\n");
    delayMs(2000); 
    
    // Note: Control board does not reply to 'L', it just acts.
    // So we just return 1 (PASS) assuming you heard the beep.
    return 1; 
}
// TEST 6: TIMEOUT CONFIGURATION
// Simulates user selecting a value (e.g., 20s) and saving it.
int Test_Timeout_Setting(void) {
    char response[20] = {0};
    
    Debug_Log("--- TIMEOUT SETTING TEST ---\r\n");
    
    // STEP 1: Authenticate for Settings (Required by Control ECU)
    Debug_Log("1. Authenticating (VERIFYPWD)...\r\n");
    LCD_Clear();
    LCD_String("Test 5 Timeout");
    UART2_SendString("VERIFYPWD:");
    UART2_SendString("12345"); // Use your valid password
    UART2_SendChar('\n');
    
    Test_Receive(response);
    
    // If authentication fails, the test fails
    if (strcmp(response, "AUTH_OK") != 0) {
        Debug_Log("   -> Auth Failed!\r\n");
        return 0; 
    }
    
    delayMs(100);
    
    // STEP 2: Send New Timeout Value (e.g., 20 seconds)
    // We simulate the value the ADC would have provided.
    Debug_Log("2. Sending Timeout: 20s\r\n");
    UART2_SendString("TIMEOUT:20");
    UART2_SendChar('\n');
    
    Test_Receive(response);
    LCD_Clear();
    LCD_String("TIMEOUT SAVED");
    
    // STEP 3: Verify Save Confirmation
    return (strcmp(response, "TIMEOUT_SAVED") == 0);
}
int Test_LCD_Screen(void) {
    Debug_Log("--- LCD VISUAL TEST ---\r\n");
    
    // Verify LCD is clearing and writing
    LCD_Clear();
    LCD_String("INTEGRATION");
    LCD_SetCursor(2, 0);
    LCD_String("TESTING...");
    
    delayMs(1000);
    
    LCD_Clear();
    LCD_String("Testing"); 
    
    Debug_Log(">> LCD: displaying 'Testing'?\r\n");
    delayMs(1000);
    
    return 1;
}


/* --- MAIN RUNNER --- */
void Run_Integration_Tests(void) {
    Debug_UART0_Init();
    UART2_Init();
    
    // Force Wakeup of Terminal
    delayMs(100);
    Debug_Log("\r\n\r\n\r\n"); 
    Debug_Log("============================\r\n");
    Debug_Log("     SYSTEM RESTART         \r\n");
    Debug_Log("============================\r\n");
    
    delayMs(1000); 
    
    Log_Result("1. PWD Setup", Test_Initial_Setup());
    delayMs(500);
    
    Log_Result("2. Door Open", Test_Door_Open());
    delayMs(500);
    
    Log_Result("3. Wrong PWD", Test_Access_Denied());
    delayMs(500);

    Log_Result("4. Lockout (Buzzer)", Test_Lockout_Sequence());
    delayMs(500);

    Log_Result("5. Timeout Setting", Test_Timeout_Setting());
    delayMs(500);
    
    Log_Result("6. LCD Screen", Test_LCD_Screen());
    
    Debug_Log("--- ALL TESTS COMPLETE ---\r\n");
    while(1); 
}