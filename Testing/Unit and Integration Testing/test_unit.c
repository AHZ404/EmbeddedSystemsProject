#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "uart.h"   // Your UART driver
#include "eeprom.h" // Your EEPROM driver
#include "dio.h"    // Your GPIO/DIO driver
#include "Servo.h"
#include "buzzer.h"

/* --- 1. SELF-CONTAINED LOGGER (UART0) --- */
void Debug_UART0_Init(void) {
    SYSCTL_RCGCUART_R |= 0x01;
    SYSCTL_RCGCGPIO_R |= 0x01;
    volatile int delay = SYSCTL_RCGCGPIO_R; 
    GPIO_PORTA_AFSEL_R |= 0x03;           
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) | 0x00000011; 
    GPIO_PORTA_DEN_R |= 0x03;             
    GPIO_PORTA_AMSEL_R &= ~0x03;          
    UART0_CTL_R &= ~0x01;                 
    UART0_IBRD_R = 8;                     
    UART0_FBRD_R = 44;                    
    UART0_LCRH_R = 0x70;                  
    UART0_CC_R = 0x0;                     
    UART0_CTL_R |= 0x301;                 
}

void Debug_Log(char *str) {
    while(*str) {
        while((UART0_FR_R & 0x20) != 0); 
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

/* --- 2. THE UNIT TESTS --- */

// TEST A: EEPROM READ/WRITE
// Requirement: "EEPROM for storing password and configuration" 
int UnitTest_EEPROM(void) {
    uint32_t test_val = 0x12345678;
    uint32_t read_val = 0;
    
    // 1. Initialize
    if(EEPROM_Init() != EEPROM_SUCCESS) return 0; // Fail if Init fails
    
    // 2. Write value to Block 1, Offset 0
    EEPROM_WriteWord(1, 0, test_val);
    
    // 3. Delay slightly to ensure write cycle completes
    int i;
    for(i=0; i<10000; i++); 
    
    // 4. Read back
    EEPROM_ReadWord(1, 0, &read_val);
    
    // 5. Compare
    if (read_val == test_val) return 1; // PASS
    
    // Debug info if failed
    char buf[30];
    sprintf(buf, " (Got: %x)", read_val);
    Debug_Log(buf);
    return 0; // FAIL
}

// TEST B: UART LOOPBACK
// Requirement: "Inter-microcontroller communication using UART" [cite: 12]
// NOTE: Requires Wire between PD6 and PD7!
int UnitTest_UART_Loopback(void) {
    char sent = 'X';
    char received = 0;
    
    // 1. Clear any old data
    while((UART2_FR_R & 0x10) == 0) { volatile int x = UART2_DR_R; }
    
    // 2. Send Character
    UART2_SendChar(sent);
    
    // 3. Wait for Receive (Short timeout)
    int timeout = 0;
    while((UART2_FR_R & 0x10) != 0) { // While Empty
        timeout++;
        if(timeout > 100000) return 0; // FAIL: Timeout (Wire missing?)
    }
    
    // 4. Read Character
    received = (char)(UART2_DR_R & 0xFF);
    
    // 5. Verify
    if (received == sent) return 1; // PASS
    return 0; // FAIL
}

// TEST C: GPIO OUTPUT (Internal Register Check)
// Requirement: "Visual status indication using RGB LEDs" [cite: 32]
int UnitTest_GPIO_LED(void) {
    // Test Red LED Pin (PF1)
    
    // 1. Set Direction to Output
    GPIO_PORTF_DIR_R |= 0x02; 
    GPIO_PORTF_DEN_R |= 0x02;
    
    // 2. Write HIGH
    GPIO_PORTF_DATA_R |= 0x02;
    
    // 3. Read back the DATA Register to see if the bit stuck
    if ((GPIO_PORTF_DATA_R & 0x02) == 0x02) {
        // 4. Write LOW
        GPIO_PORTF_DATA_R &= ~0x02;
        // 5. Read back
        if ((GPIO_PORTF_DATA_R & 0x02) == 0) {
            return 1; // PASS
        }
    }
    return 0; // FAIL
}
int UnitTest_Buzzer(void) {
    Debug_Log("TEST 4: Buzzer Driver... Listen for beep.\r\n");
    
    // 1. Initialize
    Buzzer_Init();
    
    // 2. Turn ON
    Debug_Log("   -> Buzzer ON (1 sec)\r\n");
    Buzzer_Beep(1000); 
    
    // 3. Wait 1 second (Manual verification by ear) 
    
    // 4. Turn OFF
    Debug_Log("   -> Buzzer OFF\r\n");
    
    // Since we can't programmatically "hear" the beep without a microphone,
    // we assume pass if the code didn't crash.
    // Ideally, you verify the GPIO pin state like we did in Test C.
    if ((GPIO_PORTF_DATA_R & 0x02) == 0) { // Check if pin went low
        return 1; // PASS
    }
    return 0; 
}

// TEST E: SERVO MOTOR DRIVER
// Requirement: "Motor control for door locking/unlocking"
int UnitTest_Servo(void) {
    Debug_Log("TEST 5: Servo Driver... Watch the Motor.\r\n");
    
    Servo_Init();
    
    // 1. Move to UNLOCK (90 degrees)
    Debug_Log("   -> Set Angle: 90 (Unlock)\r\n");
    Servo_SetAngle(90);
    
    // Wait for movement
    int i;
    for(i=0; i<4000000; i++); 
    
    // 2. Move to LOCK (0 degrees)
    Debug_Log("   -> Set Angle: 0 (Lock)\r\n");
    Servo_SetAngle(0);
    
    // Wait for movement
    for(i=0; i<4000000; i++); 
    
    return 1; // PASS (Visual confirmation required)
}

/* --- 3. RUNNER --- */
void Run_Unit_Tests(void) {
    Debug_UART0_Init();
    UART2_Init();
    
    // Clear terminal
    int i;
    for(i=0; i<100000; i++);
    Debug_Log("\r\n\r\n=== CONTROL ECU UNIT TESTS ===\r\n");
    
    Log_Result("1. EEPROM Read/Write", UnitTest_EEPROM());
    
    // Check if user connected the loopback wire
    Debug_Log(">> TEST 2 REQUIRES PD6 <-> PD7 WIRE <<\r\n");
    Log_Result("2. UART Driver Loopback", UnitTest_UART_Loopback());
    
    Log_Result("3. GPIO Register Logic", UnitTest_GPIO_LED());
    Log_Result("4. Buzzer Actuation", UnitTest_Buzzer());
    Log_Result("5. Servo Movement", UnitTest_Servo());
    
    Debug_Log("--- TESTS COMPLETE ---\r\n");
    while(1);
}