#include "tm4c123gh6pm.h"

#include "Servo.h"



// Servo control via GPIO PE5 using PWM pulse width

// This code directly controls servo pulses without relying on Arduino libraries



void Servo_Init(void)

{

    // Enable Port E clock

    SYSCTL_RCGCGPIO_R |= 0x10;  // Port E (Bit 4 is 0x10)

    while((SYSCTL_PRGPIO_R & 0x10) == 0);

   

    // Configure PE5 as output for servo control

    GPIO_PORTE_DIR_R |= 0x20;   // PE5 as output (Bit 5 is 0x20)

    GPIO_PORTE_DEN_R |= 0x20;   // Enable digital I/O

}



// Move servo to specific angle (0-180 degrees)

// 0 degrees   = 1.0 ms pulse

// 90 degrees  = 1.5 ms pulse

// 180 degrees = 2.0 ms pulse

void Servo_SetAngle(int angle)

{

    unsigned int pulse_us;

   

    // Convert angle to pulse width in microseconds

    // Formula: pulse_us = 1000 + (angle * 1000 / 180)

    if(angle < 0) angle = 0;

    if(angle > 180) angle = 180;

   

    pulse_us = 1000 + (angle * 1000 / 180);  // Range: 1000 to 2000 microseconds

   

    // Generate PWM pulse

    GPIO_PORTE_DATA_R |= 0x20;      // Set PE5 HIGH

    delayUs(pulse_us);               // Pulse width

    GPIO_PORTE_DATA_R &= ~0x20;     // Set PE5 LOW

    delayUs(20000 - pulse_us);       // Total cycle time 20ms

}