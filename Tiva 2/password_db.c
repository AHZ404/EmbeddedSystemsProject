#include "tm4c123gh6pm.h"
#include "password_db.h"
#include <string.h>
#include <stdint.h>

static uint8_t failed_attempts = 0;

void PasswordDB_Init(void) {
    failed_attempts = 0;
}

uint8_t PasswordDB_Verify(const char* password) {
    // Check if system is locked
    if (PasswordDB_IsLocked()) {
        return 2; // System locked
    }
    
    // Verify password
    if (strcmp(password, CORRECT_PASSWORD) == 0) {
        failed_attempts = 0; // Reset on success
        return 1; // Correct password
    } else {
        failed_attempts++;
        return 0; // Incorrect password
    }
}

void PasswordDB_ResetAttempts(void) {
    failed_attempts = 0;
}

uint8_t PasswordDB_IsLocked(void) {
    return (failed_attempts >= MAX_ATTEMPTS);
}