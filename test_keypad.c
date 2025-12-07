/*****************************************************************************
 * File: test_keypad.c
 * Description: Step-by-step keypad testing program
 * This program tests each component individually
 *****************************************************************************/

#include "keypad.h"
#include "dio.h"
#include <stdio.h>

/*
 * TEST 1: Check if GPIO pins are responding to writes
 * Tests that we can toggle column pins
 */
void test_column_pins(void) {
    printf("\n=== TEST 1: COLUMN PIN TOGGLE ===\n");
    
    uint8_t col_pins[4] = {4, 5, 6, 7}; // PC4-PC7
    
    for (int i = 0; i < 4; i++) {
        printf("Setting PC%d HIGH...\n", col_pins[i]);
        DIO_Init(PORTC, col_pins[i], OUTPUT);
        DIO_WritePin(PORTC, col_pins[i], HIGH);
        for (volatile int d = 0; d < 100000; d++); // Delay
        
        printf("Setting PC%d LOW...\n", col_pins[i]);
        DIO_WritePin(PORTC, col_pins[i], LOW);
        for (volatile int d = 0; d < 100000; d++); // Delay
    }
    
    printf("Column pins test complete. Check if LED blinks or use oscilloscope.\n");
}

/*
 * TEST 2: Check if row pins can read input
 * Tests that we can read row pin states
 */
void test_row_pins(void) {
    printf("\n=== TEST 2: ROW PIN READ ===\n");
    
    uint8_t row_pins[4] = {2, 3, 4, 5}; // PA2-PA5
    
    // Initialize row pins as inputs with pull-up
    for (int i = 0; i < 4; i++) {
        DIO_Init(PORTA, row_pins[i], INPUT);
        DIO_SetPUR(PORTA, row_pins[i], ENABLE);
    }
    
    printf("Row pins initialized. Reading for 10 seconds...\n");
    printf("Press keys during this time:\n");
    printf("PA2 PA3 PA4 PA5\n");
    
    for (int count = 0; count < 50; count++) {
        printf("Read %d: ", count);
        for (int i = 0; i < 4; i++) {
            uint8_t val = DIO_ReadPin(PORTA, row_pins[i]);
            printf("PA%d=%d  ", row_pins[i], val);
        }
        printf("\n");
        for (volatile int d = 0; d < 200000; d++); // 200ms delay
    }
}

/*
 * TEST 3: Manual column scan with row read
 * This manually sets one column LOW and reads all rows
 */
void test_manual_scan(void) {
    printf("\n=== TEST 3: MANUAL COLUMN SCAN ===\n");
    
    uint8_t row_pins[4] = {2, 3, 4, 5}; // PA2-PA5
    uint8_t col_pins[4] = {4, 5, 6, 7}; // PC4-PC7
    
    // Initialize pins
    for (int i = 0; i < 4; i++) {
        DIO_Init(PORTA, row_pins[i], INPUT);
        DIO_SetPUR(PORTA, row_pins[i], ENABLE);
        DIO_Init(PORTC, col_pins[i], OUTPUT);
        DIO_WritePin(PORTC, col_pins[i], HIGH);
    }
    
    printf("Testing each column one at a time...\n");
    printf("Press different keys and watch for row changes:\n\n");
    
    for (int count = 0; count < 100; count++) {
        for (int col = 0; col < 4; col++) {
            // Set all columns HIGH
            for (int c = 0; c < 4; c++) {
                DIO_WritePin(PORTC, col_pins[c], HIGH);
            }
            
            // Set current column LOW
            DIO_WritePin(PORTC, col_pins[col], LOW);
            
            // Delay for signal to settle
            for (volatile int d = 0; d < 100; d++);
            
            // Read and display
            printf("COL%d (PC%d): ", col, col_pins[col]);
            for (int row = 0; row < 4; row++) {
                uint8_t val = DIO_ReadPin(PORTA, row_pins[row]);
                if (val == LOW) {
                    printf("ROW%d=LOW ", row);
                }
            }
            printf("\n");
        }
        printf("---\n");
        for (volatile int d = 0; d < 500000; d++); // 500ms delay between scans
    }
}

/*
 * Main test menu
 */
int main(void) {
    printf("\n\n=== KEYPAD DEBUGGING TESTS ===\n");
    printf("Starting tests...\n");
    
    // Run tests in sequence
    test_column_pins();
    
    // Uncomment one test at a time
    // test_row_pins();
    // test_manual_scan();
    
    printf("\n=== TESTS COMPLETE ===\n");
    
    return 0;
}
