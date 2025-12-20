// adc.c
#include "tm4c123gh6pm.h"
#include "adc.h"

// Initialize ADC0 on Pin PE3 (AIN0)
void ADC_Pot_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x10;      // Activate clock for Port E
    SYSCTL_RCGCADC_R |= 0x01;       // Activate clock for ADC0

    GPIO_PORTE_DIR_R &= ~0x08;      // PE3 is an input
    GPIO_PORTE_AFSEL_R |= 0x08;     // Enable alternate function for PE3
    GPIO_PORTE_DEN_R &= ~0x08;      // Disable digital I/O on PE3
    GPIO_PORTE_AMSEL_R |= 0x08;     // Enable analog function on PE3 (AIN0)

    ADC0_PC_R = 0x01;               
    ADC0_SSPRI_R = 0x0123;          
    ADC0_ACTSS_R &= ~0x08;          // Disable SS3
    ADC0_EMUX_R &= ~0xF000;         // SS3 software triggered
    ADC0_SSMUX3_R = 0x00;           // Select AIN0 (PE3) for SS3
    ADC0_SSCTL3_R |= 0x06;          // Set END and IE
    ADC0_IM_R &= ~0x08;             
    ADC0_ACTSS_R |= 0x08;           // Enable SS3
}

// Read the ADC value
int ADC_ReadValue(void) // Renamed from UART0_ReadADC
{
    ADC0_PSSI_R |= 0x08;            // Initiate SS3 conversion
    while((ADC0_RIS_R & 0x08) == 0); // Wait for conversion to complete
    int value = ADC0_SSFIFO3_R;     // Read the ADC result
    ADC0_ISC_R = 0x08;              // Acknowledge completion
    return value;
}