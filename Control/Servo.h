#ifndef SERVO_H

#define SERVO_H



#include <stdint.h>

#include "dio.h"



// Define Servo Port and Pin

// Using Port E, Pin 5

#define SERVO_PORT   PORTE

#define SERVO_PIN    PIN5



// Function Prototypes

void Servo_Init(void);

void Servo_SetAngle(int angle);



#endif