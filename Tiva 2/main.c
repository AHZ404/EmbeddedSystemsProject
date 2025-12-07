#include "tm4c123gh6pm.h"
#include "uart_receiver.h"
#include "password_db.h"
#include "dio.h"
#include "systick.h"

int main(void) {
    // Initialize system
    SysTick_Init(16000, SYSTICK_NOINT); // 1ms tick
    UARTReceiver_Init();
    PasswordDB_Init();
    
    // Initialize LED for status (PF2 - Blue LED)
    DIO_Init(PORTF, PIN2, OUTPUT);
    DIO_WritePin(PORTF, PIN2, LOW);
    
    // Initialize door lock relay (PF1)
    DIO_Init(PORTF, PIN1, OUTPUT);
    DIO_WritePin(PORTF, PIN1, LOW);
    
    // Blink LED to indicate system is ready
    for(int i = 0; i < 3; i++) {
        DIO_WritePin(PORTF, PIN2, HIGH);
        DelayMs(200);
        DIO_WritePin(PORTF, PIN2, LOW);
        DelayMs(200);
    }
    
    while(1) {
        // Process incoming UART messages
        UARTReceiver_Process();
        
        // Blink LED slowly when idle
        static uint32_t last_blink = 0;
        if(msTicks - last_blink > 1000) {
            DIO_TogglePin(PORTF, PIN2);
            last_blink = msTicks;
        }
    }
}