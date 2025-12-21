#include "lcd.h"
#include "keypad.h"
#include "uart.h"
#include "dio.h"
#include <string.h>
#include <stdbool.h> 
#include <stdio.h>   // Added for sprintf (used in displaying timeout value)
#include "adc.h" // <-- NEW: Include the ADC Header
#include <tm4c123gh6pm.h>

// Helper function prototype (defined in lcd.c)
void delayMs(int n);

// ========== NEW: Function to send password to TIVA 1 (Control ECU) ==========
void SendPasswordToControl(char *password, char *command_type)
{
    // Send command in format: "SETPWD:xxxxx\n" (for password creation)
    // or "VERIFY:xxxxx\n" (for password verification)
    UART2_SendString(command_type);
    UART2_SendString(":");
    UART2_SendString(password);
    UART2_SendChar('\n');
    
    // Delay to ensure all data is transmitted and Control ECU can process
    delayMs(100);
}

// Function to receive response from Control ECU with timeout protection
char* ReceiveResponseFromControl()
{
    static char response[20];
    int buffer_index = 0;
    char received_char;
    int timeout_count = 0;
    int max_timeout = 10000;  // 10 second timeout (increased from 5s)
    
    memset(response, 0, sizeof(response));
    
    // Wait for response from Control ECU with timeout
    while(buffer_index < 19 && timeout_count < max_timeout) {
        if(UART2_ReceiveCharTimeout(&received_char, 1)) {
            if(received_char == '\n') { 
                response[buffer_index] = '\0';
                break;
            }
            response[buffer_index++] = received_char;
            timeout_count = 0;  // Reset timeout counter on successful receive
        } else {
            timeout_count++;  // Increment if no character received
        }
    }
    
    // If we timed out, mark response as error
    if(timeout_count >= max_timeout) {
        strcpy(response, "TIMEOUT");
    }
    
    return response;
}
// ========== END OF NEW PASSWORD COMMUNICATION FUNCTIONS ==========

// NOTE: The placeholder 'int UART0_ReadADC(void) { ... }' has been REMOVED
// to fix the duplicate definition error (Error[Li006]), as the actual
// implementation is now assumed to be in the separate adc.c file as ADC_ReadValue().

// 1. Define all States for the system
typedef enum {
    STATE_CREATE_PASS,          // Initial setup: Enter password
    STATE_CONFIRM_PASS,         // Initial setup: Confirm password
    STATE_MAIN_MENU,            // Main operational menu (A, B, C, D)
    STATE_VERIFY_PASS_A,        // Verify password for Action 'A' (Open)
    STATE_LOCKOUT,              // System Lockout state (for 3 failures on 'A', 'B', 'C', or 'D')
    // States for Password Change (B)
    STATE_CHANGE_PASS_VERIFY,   // Step 1: Verify the old password
    STATE_CHANGE_PASS_NEW,      // Step 2: Enter the new password
    STATE_CHANGE_PASS_CONFIRM,  // Step 3: Confirm the new password
    // States for Timeout Adjustment (C)
    STATE_ADJUST_TIMEOUT,       // Reading Potentiometer/ADC value
    STATE_VERIFY_PASS_C,        // Final password verification for saving
    // States for Reset/Logout (D)
    STATE_RESET_PASS_VERIFY,    // Verify old password before reset
    STATE_RESET_CREATE_PASS,    // Enter new password after reset verification
    STATE_RESET_CONFIRM_PASS    // Confirm new password after reset
} PasswordState;

int main(void)
{
    LCD_Init();
    Keypad_Init();
    UART2_Init();
    ADC_Pot_Init(); // <-- Call the new ADC initialization

    // Initialize LED for status (PF3 - Green LED)
    DIO_Init(PORTF, PIN3, OUTPUT);
    DIO_WritePin(PORTF, PIN3, LOW);
    
    // Wait for Control ECU to be ready
    LCD_Clear();
    LCD_String("Waiting for");
    LCD_SetCursor(2, 0);
    LCD_String("Control...");
    
    char startup_check[20] = "";
    int check_index = 0;
    int startup_timeout = 0;
    
    // Listen for "CONTROL_READY" message
    while(startup_timeout < 5000) {
        char received_char;
        if(UART2_ReceiveCharTimeout(&received_char, 1)) {
            if(received_char == '\n') {
                startup_check[check_index] = '\0';
                if(strstr(startup_check, "CONTROL_READY")) {
                    // Control ECU is ready!
                    break;
                }
                check_index = 0;
                memset(startup_check, 0, sizeof(startup_check));
            } else {
                if(check_index < 19) {
                    startup_check[check_index++] = received_char;
                }
            }
            startup_timeout = 0;
        } else {
            startup_timeout++;
        }
        delayMs(1);
    }
    
    // Display ready message
    LCD_Clear();
    LCD_String("System Ready!");
    delayMs(1500);

    PasswordState state = STATE_CREATE_PASS;
    char pass[6] = "";              // Stores the master password (5 digits + '\0')
    char Confirmpass[6] = "";       // Used for confirming master pass or verifying any attempt
    int index = 0;
    
    // Variables for 'A' verification
    int attempts_A = 0;             // Tracks incorrect attempts for 'A'
    bool lock_system = false;       // Flag: system is locked (A key disabled)

    // Variables for 'B' (Password Change)
    int attempts_B = 0;             // Tracks incorrect attempts for 'B'
    char new_pass[6] = "";          // Buffer for the new password input

    // Variables for 'C' (Timeout)
    int auto_lock_timeout = 10;     // Default timeout value (e.g., 10 seconds)
    int pot_value = 0;              // <-- FIX: Declared here to resolve Error[Pe020]
    int adjusted_timeout = 0;       // Temporary timeout value during adjustment (not saved until password verified)
    int attempts_C = 0;             // Tracks incorrect attempts for timeout verification

    // Variables for 'D' (Reset/Logout)
    int attempts_D = 0;             // Tracks incorrect attempts for 'D' password verification
    
    // ========== NEW: Variables for receiving password from TIVA 1 ==========
    char received_password[6] = "";      // Buffer to store password from TIVA 1
    bool password_received = false;      // Flag to indicate password was received
    bool password_matches = false;       // Flag to indicate password verification
    bool password_created = false;       // FIXED: Track if password has been created
    // ========== END OF NEW PASSWORD VARIABLES ==========

    LCD_Clear();
    LCD_String("CreatePass:"); 

    while(1)
    {
        char key = Keypad_GetKey();

        // --- 0. Handle Lockout Recovery via '*' ---
        if (lock_system && key == '*') {
            lock_system = false;
            attempts_A = 0;
            attempts_B = 0;
            state = STATE_MAIN_MENU;
            LCD_Clear();
            LCD_SetCursor(1, 0);
            LCD_String("Menu>A:Ope B:PWD");
            LCD_SetCursor(2, 0);
            LCD_String(" C:TMO  D:Reset");
            continue;
        }

        if(!key) 
        {
            // The timeout adjustment state must continue reading the pot even if no key is pressed
            if (state == STATE_ADJUST_TIMEOUT) {
                // FALL THROUGH to the STATE_ADJUST_TIMEOUT logic below
            } else {
                continue; // Skip the rest if no key and not in adjustment state
            }
        }
        
        // ====================================================
        // PHASE 1: MAIN MENU HANDLING
        // ====================================================
        if (state == STATE_MAIN_MENU)
        {
            // --- Action 'A': Open / Access ---
            if (key == 'A') {
                if (lock_system) {
                    LCD_Clear();
                    LCD_String("SYSTEM LOCKED");
                    LCD_SetCursor(2, 0);
                    LCD_String("Press '*' to Menu");
                } else {
                    index = 0; 
                    state = STATE_VERIFY_PASS_A;
                    LCD_Clear();
                    LCD_SetCursor(1, 0);
                    LCD_String("Enter Pwd:");
                }
            }
            // --- Action 'B': Change Password ---
            else if (key == 'B') {
                if (lock_system) {
                    LCD_Clear();
                    LCD_String("SYSTEM LOCKED");
                    LCD_SetCursor(2, 0);
                    LCD_String("Press '*' to Menu");
                } else {
                    index = 0; 
                    attempts_B = 0; 
                    state = STATE_CHANGE_PASS_VERIFY;
                    LCD_Clear();
                    LCD_SetCursor(1, 0);
                    LCD_String("Enter Old Pwd:");
                    LCD_SetCursor(2, 0);
                    index = 0;
                }
            }
            // --- Action 'C': Set Auto-Lock Timeout (TMO) ---
            else if (key == 'C') {
                if (lock_system) {
                    LCD_Clear();
                    LCD_String("SYSTEM LOCKED");
                    LCD_SetCursor(2, 0);
                    LCD_String("Press '*' to Menu");
                } else {
                    state = STATE_ADJUST_TIMEOUT;
                    LCD_Clear();
                    LCD_String("Adjust Timeout:");
                    LCD_SetCursor(2, 0);
                    LCD_String("Value: "); // Prepare to display the value
                }
            }
            // --- Action 'D': Lock System (Logout) / Reset Password ---
            else if (key == 'D') {
                if (lock_system) {
                    LCD_Clear();
                    LCD_String("SYSTEM LOCKED");
                    LCD_SetCursor(2, 0);
                    LCD_String("Press '*' to Menu");
                } else {
                    index = 0;
                    attempts_D = 0;
                    state = STATE_RESET_PASS_VERIFY;
                    LCD_Clear();
                    LCD_SetCursor(1, 0);
                    LCD_String("Verify Old Pwd:");
                    LCD_SetCursor(2, 0);
                }
            }
            
            continue;
        }
        
        // ====================================================
        // PHASE 2: TIMEOUT ADJUSTMENT LOGIC (C)
        // ====================================================
        if (state == STATE_ADJUST_TIMEOUT)
        {
            // pot_value declaration REMOVED from here to solve scope error.
            
            // Read ADC and scale the value
            pot_value = ADC_ReadValue(); // <-- USE NEW ADC FUNCTION NAME
            // Map the ADC range (e.g., 0-4095) to the required range (5-30)
            int raw_timeout = (25 * pot_value) / 4095 + 5;
            
            if (raw_timeout < 5) raw_timeout = 5;
            if (raw_timeout > 30) raw_timeout = 30;
            
            // Update the TEMPORARY adjusted_timeout variable (NOT the global auto_lock_timeout)
            adjusted_timeout = raw_timeout;
            
            // Update display: Line 2 - "Value: XXs"
            LCD_SetCursor(2, 7); 
            LCD_String("  "); // Clear previous value
            LCD_SetCursor(2, 7);
            
            char timeout_str[4];
            sprintf(timeout_str, "%d", adjusted_timeout); 
            LCD_String(timeout_str);
            LCD_Char('s');

            // Handle Keypad input
            if(key == '#') // '#' is used to confirm the timeout value ("Save")
            {
                index = 0;
                state = STATE_VERIFY_PASS_C; // Move to password verification
                LCD_Clear();
                LCD_SetCursor(1, 0);
                LCD_String("Enter Pwd:");
                LCD_SetCursor(2, 0);
            }
            else if(key == '*') // '*' is used to cancel and return to menu
            {
                adjusted_timeout = 0; // Clear the temporary timeout value
                state = STATE_MAIN_MENU; 
                LCD_Clear();
                LCD_SetCursor(1, 0);
                LCD_String("Menu>A:Opn B:PWD");
                LCD_SetCursor(2, 0);
                LCD_String(" C:TMO  D:Reset"); 
            }
            
            continue; 
        }
        
        // ====================================================
        // PHASE 3: PASSWORD VERIFICATION FOR TIMEOUT (C)
        // ====================================================
        if (state == STATE_VERIFY_PASS_C)
        {
            // A. Handle input characters ('0' to '9')
            if(key >= '0' && key <= '9')
            {
                if(index < 5)
                {
                    Confirmpass[index++] = key;
                    LCD_Char('*');
                }
            }
            // B. Handle '#' (Confirm)
            else if(key == '#' && index == 5)
            {
                Confirmpass[index] = '\0';
                index = 0; 
                
                if(strcmp(pass, Confirmpass) == 0) // Correct Password
                {
                    attempts_C = 0; // Reset attempts on success
                    // Password is correct - NOW update the global auto_lock_timeout with the adjusted value
                    auto_lock_timeout = adjusted_timeout;
                    
                    LCD_Clear();
                    LCD_String("Saving Timeout...");
                    
                    // First, authenticate with Control ECU using VERIFYPWD
                    char verify_cmd[20];
                    sprintf(verify_cmd, "VERIFYPWD:%s", pass);
                    UART2_SendString(verify_cmd);
                    UART2_SendChar('\n');
                    delayMs(100);
                    
                    // Wait for AUTH_OK response
                    char* auth_response = ReceiveResponseFromControl();
                    
                    if(strcmp(auth_response, "AUTH_OK") == 0)
                    {
                        // Now send TIMEOUT command with authenticated session
                        char timeout_cmd[20];
                        sprintf(timeout_cmd, "TIMEOUT:%d", auto_lock_timeout);
                        UART2_SendString(timeout_cmd);
                        UART2_SendChar('\n');
                        delayMs(100);
                        
                        // Wait for response from Control ECU
                        char* timeout_response = ReceiveResponseFromControl();
                        
                        if(strcmp(timeout_response, "TIMEOUT_SAVED") == 0)
                        {
                            LCD_Clear();
                            LCD_String("Timeout Saved!");
                            DIO_WritePin(PORTF, PIN3, HIGH); // Green LED for success
                            delayMs(1500);
                            DIO_WritePin(PORTF, PIN3, LOW);
                        }
                        else
                        {
                            LCD_Clear();
                            LCD_String("Error Saving TMO");
                            LCD_SetCursor(2, 0);
                            LCD_String("Please Retry");
                            DIO_WritePin(PORTF, PIN1, HIGH); // Red LED for error
                            delayMs(1500);
                            DIO_WritePin(PORTF, PIN1, LOW);
                        }
                    }
                    else
                    {
                        // Authentication failed on Control side
                        LCD_Clear();
                        LCD_String("Auth Failed!");
                        LCD_SetCursor(2, 0);
                        LCD_String("Timeout NOT Saved");
                        DIO_WritePin(PORTF, PIN1, HIGH); // Red LED for error
                        delayMs(1500);
                        DIO_WritePin(PORTF, PIN1, LOW);
                    }

                    adjusted_timeout = 0; // Clear temporary timeout after save attempt
                    state = STATE_MAIN_MENU;
                    LCD_Clear();
                    LCD_SetCursor(1, 0);
                    LCD_String("Menu>A:Opn B:PWD");
                    LCD_SetCursor(2, 0);
                    LCD_String(" C:TMO  D:Reset");
                }
                else // Incorrect Password
                {
                    attempts_C++;
                    
                    if (attempts_C >= 3)
                    {
                        // 3 failed attempts - System lockout
                        adjusted_timeout = 0; // Clear the temporary timeout
                        state = STATE_LOCKOUT;
                        UART2_SendChar('L'); // Send lockout signal to Control
                        
                        LCD_Clear();
                        LCD_String("3 Failed Attempts");
                        LCD_SetCursor(2, 0);
                        LCD_String("SYSTEM LOCKED!");
                        delayMs(5000);
                        
                        // Reset after lockout
                        attempts_C = 0;
                        lock_system = false;
                        state = STATE_MAIN_MENU;
                        LCD_Clear();
                        LCD_SetCursor(1, 0);
                        LCD_String("Menu>A:Opn B:PWD");
                        LCD_SetCursor(2, 0);
                        LCD_String(" C:TMO  D:Reset");
                    }
                    else
                    {
                        // Show retry message with remaining attempts
                        LCD_Clear();
                        LCD_String("TMO NOT Saved");
                        LCD_SetCursor(2, 0);
                        LCD_String("Attempts Left: ");
                        LCD_Char('3' - attempts_C); // Display remaining attempts (e.g., '2', '1')
                        delayMs(2000);
                        
                        // Ask for password again
                        LCD_Clear();
                        LCD_SetCursor(1, 0);
                        LCD_String("Enter Pwd:");
                        LCD_SetCursor(2, 0);
                        index = 0;
                        memset(Confirmpass, 0, sizeof(Confirmpass));
                    }
                }
            }
            // C. Handle '*' (Clear/Cancel)
            else if(key == '*')
            {
                index = 0; 
                LCD_Clear();
                LCD_SetCursor(1, 0);
                LCD_String("Enter Pwd: ");
                LCD_SetCursor(2, 0);
            }
            
            continue;
        }

        // ====================================================
        // PHASE 4: PASSWORD CHANGE LOGIC (B)
        // ====================================================
        if (state == STATE_CHANGE_PASS_VERIFY || 
            state == STATE_CHANGE_PASS_NEW || 
            state == STATE_CHANGE_PASS_CONFIRM)
        {
            char* active_buffer; 

            if (state == STATE_CHANGE_PASS_VERIFY) {
                active_buffer = Confirmpass; // Verify old password
            } else if (state == STATE_CHANGE_PASS_NEW) {
                active_buffer = new_pass;    // Enter new password
            } else { // STATE_CHANGE_PASS_CONFIRM
                active_buffer = Confirmpass; // Confirm new password
            }

            // A. Handle input characters ('0' to '9')
            if(key >= '0' && key <= '9')
            {
                if(index < 5)
                {
                    active_buffer[index++] = key;
                    LCD_Char(key);
                }
            }
            // B. Handle '#' (Confirm)
            else if(key == '#' && index == 5)
            {
                active_buffer[index] = '\0';
                index = 0;
                
                // --- 1. VERIFY OLD PASSWORD ---
                if (state == STATE_CHANGE_PASS_VERIFY)
                {
                    if (strcmp(pass, Confirmpass) == 0) // Correct Old Password
                    {
                        attempts_B = 0; 
                        state = STATE_CHANGE_PASS_NEW; 
                        LCD_Clear();
                        LCD_SetCursor(1, 0);
                        LCD_String("Enter New Pwd:");
                        LCD_SetCursor(2, 0);
                    }
                    else // Incorrect Old Password
                    {
                        attempts_B++;
                        if (attempts_B >= 3) {
                            lock_system = true;
                            state = STATE_LOCKOUT;
                            UART2_SendChar('L');
                            LCD_Clear();
                            LCD_String("3 Failed Attempts");
                            LCD_SetCursor(2, 0);
                            LCD_String("SYSTEM LOCKED!");
                            delayMs(5000); 

                            lock_system = false; // Reset lock after delay
                            attempts_B = 0;
                            state = STATE_MAIN_MENU;
                            LCD_Clear();
                            LCD_SetCursor(1, 0);
                            LCD_String("Menu>A:Opn B:PWD");
                            LCD_SetCursor(2, 0);
                            LCD_String(" C:TMO  D:Reset");
                        } else {
                            LCD_Clear();
                            LCD_String("Old PWD Incorrect");
                            LCD_SetCursor(2, 0);
                            LCD_String("Attempts Left: ");
                            // '3' - attempts_B will result in a char value, e.g., '1' for 2 attempts left
                            LCD_Char('3' - attempts_B); 
                            delayMs(2000);
                            
                            index = 0;
                            LCD_Clear();
                            LCD_SetCursor(1, 0);
                            LCD_String("Enter Old Pwd: ");
                            LCD_SetCursor(2, 0);
                        }
                    }
                }
                // --- 2. ENTER NEW PASSWORD ---
                else if (state == STATE_CHANGE_PASS_NEW)
                {
                    state = STATE_CHANGE_PASS_CONFIRM; 
                    LCD_Clear();
                    LCD_SetCursor(1, 0);
                    LCD_String("Confirm New Pwd: ");
                    LCD_SetCursor(2, 0);
                }
                // --- 3. CONFIRM NEW PASSWORD ---
                else if (state == STATE_CHANGE_PASS_CONFIRM)
                {
                    if (strcmp(new_pass, Confirmpass) == 0) // New Password confirmed
                    {
                        strcpy(pass, new_pass);
                        
                        // Send the new password to Control ECU to update EEPROM
                        LCD_Clear();
                        LCD_String("Saving to EEPROM...");
                        SendPasswordToControl(new_pass, "SETPWD");
                        
                        // Receive response from Control ECU
                        char* save_response = ReceiveResponseFromControl();
                        
                        if(strcmp(save_response, "PWD_SAVED") == 0) // Password saved successfully
                        {
                            LCD_Clear();
                            LCD_String("Password Changed!");
                            LCD_SetCursor(2, 0);
                            LCD_String("Saved to EEPROM");
                            DIO_WritePin(PORTF, PIN3, HIGH);
                            delayMs(2000);
                            DIO_WritePin(PORTF, PIN3, LOW);
                        }
                        else // Failed to save password
                        {
                            LCD_Clear();
                            LCD_String("Error Saving PWD");
                            LCD_SetCursor(2, 0);
                            LCD_String("Please Retry");
                            DIO_WritePin(PORTF, PIN1, HIGH);
                            delayMs(2000);
                            DIO_WritePin(PORTF, PIN1, LOW);
                        }
                        
                        state = STATE_MAIN_MENU;
                        LCD_Clear();
                        LCD_SetCursor(1, 0);
                        LCD_String("Menu>A:Opn B:PWD");
                        LCD_SetCursor(2, 0);
                        LCD_String(" C:TMO  D:Reset");
                    }
                    else // New Passwords Mismatch
                    {
                        LCD_Clear();
                        LCD_String("Mismatch! Restart");
                        delayMs(2000);
                        
                        index = 0;
                        state = STATE_CHANGE_PASS_NEW;
                        LCD_Clear();
                        LCD_SetCursor(1, 0);
                        LCD_String("Enter New Pwd:");
                        LCD_SetCursor(2, 0);
                    }
                }
            }
            // C. Handle '*' (Clear/Cancel)
            else if(key == '*')
            {
                index = 0; 
                LCD_Clear();
                if (state == STATE_CHANGE_PASS_VERIFY) {
                    LCD_SetCursor(1, 0); 
                    LCD_String("Enter Old Pwd:"); 
                    LCD_SetCursor(2, 0);
                } else if (state == STATE_CHANGE_PASS_NEW) {
                    LCD_SetCursor(1, 0); 
                    LCD_String("Enter New Pwd:"); 
                    LCD_SetCursor(2, 0);
                } else if (state == STATE_CHANGE_PASS_CONFIRM) {
                    LCD_SetCursor(1, 0); 
                    LCD_String("Confirm New Pwd:"); 
                    LCD_SetCursor(2, 0);
                }
            }
            continue; 
        }
        
        // ====================================================
        // PHASE 4.5: RESET PASSWORD LOGIC (D)
        // ====================================================
        if (state == STATE_RESET_PASS_VERIFY || 
            state == STATE_RESET_CREATE_PASS || 
            state == STATE_RESET_CONFIRM_PASS)
        {
            char* active_buffer; 

            if (state == STATE_RESET_PASS_VERIFY) {
                active_buffer = Confirmpass; // Verify old password
            } else if (state == STATE_RESET_CREATE_PASS) {
                active_buffer = new_pass;    // Enter new password
            } else { // STATE_RESET_CONFIRM_PASS
                active_buffer = Confirmpass; // Confirm new password
            }

            // A. Handle input characters ('0' to '9')
            if(key >= '0' && key <= '9')
            {
                if(index < 5)
                {
                    active_buffer[index++] = key;
                    LCD_Char(key);
                }
            }
            // B. Handle '#' (Confirm)
            else if(key == '#' && index == 5)
            {
                active_buffer[index] = '\0';
                index = 0;
                
                // --- 1. VERIFY OLD PASSWORD (Reset) ---
                if (state == STATE_RESET_PASS_VERIFY)
                {
                    if (strcmp(pass, Confirmpass) == 0) // Correct Old Password
                    {
                        attempts_D = 0;
                        state = STATE_RESET_CREATE_PASS; 
                        LCD_Clear();
                        LCD_SetCursor(1, 0);
                        LCD_String("Enter New Pwd:");
                        LCD_SetCursor(2, 0);
                    }
                    else // Incorrect Old Password
                    {
                        attempts_D++;
                        if (attempts_D >= 3) {
                            lock_system = true;
                            state = STATE_LOCKOUT;
                            UART2_SendChar('L');
                            LCD_Clear();
                            LCD_String("3 Failed Attempts");
                            LCD_SetCursor(2, 0);
                            LCD_String("SYSTEM LOCKED!");
                            delayMs(5000); 

                            lock_system = false; // Reset lock after delay
                            attempts_D = 0;
                            state = STATE_MAIN_MENU;
                            LCD_Clear();
                            LCD_SetCursor(1, 0);
                            LCD_String("Menu>A:Opn B:PWD");
                            LCD_SetCursor(2, 0);
                            LCD_String(" C:TMO  D:Reset");
                        } else {
                            LCD_Clear();
                            LCD_String("Old PWD Incorrect");
                            LCD_SetCursor(2, 0);
                            LCD_String("Attempts Left: ");
                            LCD_Char('3' - attempts_D); 
                            delayMs(2000);
                            
                            index = 0;
                            LCD_Clear();
                            LCD_SetCursor(1, 0);
                            LCD_String("Verify Old Pwd: ");
                            LCD_SetCursor(2, 0);
                            memset(Confirmpass, 0, sizeof(Confirmpass));
                        }
                    }
                }
                // --- 2. ENTER NEW PASSWORD (Reset) ---
                else if (state == STATE_RESET_CREATE_PASS)
                {
                    state = STATE_RESET_CONFIRM_PASS; 
                    LCD_Clear();
                    LCD_SetCursor(1, 0);
                    LCD_String("Confirm New Pwd: ");
                    LCD_SetCursor(2, 0);
                }
                // --- 3. CONFIRM NEW PASSWORD (Reset) ---
                else if (state == STATE_RESET_CONFIRM_PASS)
                {
                    if (strcmp(new_pass, Confirmpass) == 0) // New Password confirmed
                    {
                        strcpy(pass, new_pass);
                        
                        // Send the new password to Control ECU to update EEPROM
                        LCD_Clear();
                        LCD_String("Saving to EEPROM...");
                        SendPasswordToControl(new_pass, "SETPWD");
                        
                        // Receive response from Control ECU
                        char* save_response = ReceiveResponseFromControl();
                        
                        if(strcmp(save_response, "PWD_SAVED") == 0) // Password saved successfully
                        {
                            LCD_Clear();
                            LCD_String("Password Reset!");
                            LCD_SetCursor(2, 0);
                            LCD_String("Saved to EEPROM");
                            DIO_WritePin(PORTF, PIN3, HIGH);
                            delayMs(1500);
                            DIO_WritePin(PORTF, PIN3, LOW);
                            
                            // Now reset timeout to 10 seconds
                            LCD_Clear();
                            LCD_String("Resetting TMO...");
                            
                            // Authenticate with VERIFYPWD for timeout reset
                            char verify_cmd[20];
                            sprintf(verify_cmd, "VERIFYPWD:%s", new_pass);
                            UART2_SendString(verify_cmd);
                            UART2_SendChar('\n');
                            delayMs(100);
                            
                            char* auth_response = ReceiveResponseFromControl();
                            
                            if(strcmp(auth_response, "AUTH_OK") == 0)
                            {
                                // Send timeout reset command
                                char timeout_cmd[20];
                                sprintf(timeout_cmd, "TIMEOUT:10");
                                UART2_SendString(timeout_cmd);
                                UART2_SendChar('\n');
                                delayMs(100);
                                
                                char* timeout_response = ReceiveResponseFromControl();
                                
                                if(strcmp(timeout_response, "TIMEOUT_SAVED") == 0)
                                {
                                    auto_lock_timeout = 10; // Update local variable
                                    LCD_Clear();
                                    LCD_String("Complete!");
                                    LCD_SetCursor(2, 0);
                                    LCD_String("TMO Reset to 10s");
                                    DIO_WritePin(PORTF, PIN3, HIGH);
                                    delayMs(1500);
                                    DIO_WritePin(PORTF, PIN3, LOW);
                                }
                                else
                                {
                                    LCD_Clear();
                                    LCD_String("Warning: TMO");
                                    LCD_SetCursor(2, 0);
                                    LCD_String("reset failed");
                                    delayMs(1500);
                                }
                            }
                            else
                            {
                                LCD_Clear();
                                LCD_String("Warning: TMO");
                                LCD_SetCursor(2, 0);
                                LCD_String("reset failed");
                                delayMs(1500);
                            }
                        }
                        else // Failed to save password
                        {
                            LCD_Clear();
                            LCD_String("Error Saving PWD");
                            LCD_SetCursor(2, 0);
                            LCD_String("Please Retry");
                            DIO_WritePin(PORTF, PIN1, HIGH);
                            delayMs(2000);
                            DIO_WritePin(PORTF, PIN1, LOW);
                        }
                        
                        state = STATE_MAIN_MENU;
                        LCD_Clear();
                        LCD_SetCursor(1, 0);
                        LCD_String("Menu>A:Opn B:PWD");
                        LCD_SetCursor(2, 0);
                        LCD_String(" C:TMO  D:Reset");
                    }
                    else // New Passwords Mismatch
                    {
                        LCD_Clear();
                        LCD_String("Mismatch! Restart");
                        delayMs(2000);
                        
                        index = 0;
                        state = STATE_RESET_CREATE_PASS;
                        LCD_Clear();
                        LCD_SetCursor(1, 0);
                        LCD_String("Enter New Pwd:");
                        LCD_SetCursor(2, 0);
                    }
                }
            }
            // C. Handle '*' (Clear/Cancel)
            else if(key == '*')
            {
                index = 0; 
                LCD_Clear();
                if (state == STATE_RESET_PASS_VERIFY) {
                    LCD_SetCursor(1, 0); 
                    LCD_String("Verify Old Pwd:"); 
                    LCD_SetCursor(2, 0);
                } else if (state == STATE_RESET_CREATE_PASS) {
                    LCD_SetCursor(1, 0); 
                    LCD_String("Enter New Pwd:"); 
                    LCD_SetCursor(2, 0);
                } else if (state == STATE_RESET_CONFIRM_PASS) {
                    LCD_SetCursor(1, 0); 
                    LCD_String("Confirm New Pwd:"); 
                    LCD_SetCursor(2, 0);
                }
            }
            continue; 
        }
        if (state == STATE_VERIFY_PASS_A)
        {
            // A. Handle input characters ('0' to '9')
            if(key >= '0' && key <= '9')
            {
                if(index < 5)
                {
                    Confirmpass[index++] = key; 
                    LCD_Char('*');              
                }
            }
            // B. Handle '#' (Confirm)
            else if(key == '#' && index == 5)
            {
                Confirmpass[index] = '\0';
                index = 0; 
                
                LCD_Clear();
                LCD_String("Verifying..."); // Tell user we are verifying with Control
                
                // Send the entered password to Control ECU for verification
                SendPasswordToControl(Confirmpass, "VERIFY");
                
                // Receive response from Control ECU
                char* control_response = ReceiveResponseFromControl();
                
                if(strcmp(control_response, "ALLOW") == 0) // Correct Password
                {
                    attempts_A = 0; 
                    
                    LCD_Clear();
                    LCD_String("Access Granted");
                    LCD_SetCursor(2, 0);
                    LCD_String("Closing in ");
                    
                    // Display the dynamic timeout value
                    char timeout_display[3];
                    sprintf(timeout_display, "%d", auto_lock_timeout);
                    LCD_String(timeout_display);
                    LCD_String("s...");

                    DIO_WritePin(PORTF, PIN3, HIGH); 
                    delayMs(auto_lock_timeout * 1000); 
                    UART2_SendString("CLOSE\n");
                    DIO_WritePin(PORTF, PIN3, LOW); 
                    
                    state = STATE_MAIN_MENU;
                    LCD_Clear();
                    LCD_SetCursor(1, 0);
                    LCD_String("Menu>A:Opn B:PWD");
                    LCD_SetCursor(2, 0);
                    LCD_String(" C:TMO  D:Reset");
                }
                else // Incorrect Password (DENY)
                {
                    attempts_A++;
                    
                    if (attempts_A >= 3) 
                    {
                        lock_system = true;
                        state = STATE_LOCKOUT;
                        UART2_SendChar('L');
                        LCD_Clear();
                        LCD_String("3 Failed Attempts");
                        LCD_SetCursor(2, 0);
                        LCD_String("SYSTEM LOCKED!");
                        delayMs(5000); 

                        state = STATE_MAIN_MENU; 
                        LCD_Clear();
                        LCD_SetCursor(1, 0);
                        LCD_String("Menu>A:Opn B:PWD");
                        LCD_SetCursor(2, 0);
                        LCD_String(" C:TMO  D:Reset");
                    }
                    else 
                    {
                        LCD_Clear();
                        LCD_String("Incorrect PWD.");
                        LCD_SetCursor(2, 0);
                        LCD_String("Attempts Left: ");
                        LCD_Char('3' - attempts_A); 
                        delayMs(2000);
                        
                        index = 0;
                        LCD_Clear();
                        LCD_SetCursor(1, 0);
                        LCD_String("Enter Pwd:");
                    }
                }
            }
            // C. Handle '*' (Clear/Cancel)
            else if(key == '*')
            {
                index = 0; 
                LCD_Clear();
                LCD_SetCursor(1, 0);
                LCD_String("Enter Pwd:");
            }
            continue; 
        }
        
        // ====================================================
        // PHASE 6: INITIAL PASSWORD SETUP & CONFIRMATION
        // ====================================================
        if (state == STATE_CREATE_PASS || state == STATE_CONFIRM_PASS)
        {
            // A. Handle input characters ('0' to '9')
            if(key >= '0' && key <= '9')
            {
                if(index < 5)
                {
                    if (state == STATE_CREATE_PASS) {
                        pass[index++] = key;
                    } else if (state == STATE_CONFIRM_PASS) {
                        Confirmpass[index++] = key;
                    }
                    LCD_Char(key); // Show key during initial setup
                }
            }
            // B. Handle '#' (Enter/Confirm)
            else if(key == '#' && index == 5)
            {
                if (state == STATE_CREATE_PASS) 
                {
                    pass[index] = '\0'; 
                    index = 0; 
                    state = STATE_CONFIRM_PASS;

                    LCD_Clear();
                    LCD_String("Confirm:");
                } 
                else if (state == STATE_CONFIRM_PASS) 
                {
                    Confirmpass[index] = '\0';
                    index = 0; 
                    
                    LCD_Clear();
                    
                    // 1. Check if the two local typings match
                    if(strcmp(pass, Confirmpass) == 0) 
                    {
                        LCD_String("Saving PWD..."); // Tell user we are sending password to Control
                        
                        // 2. Send password to Control ECU for EEPROM storage
                        SendPasswordToControl(pass, "SETPWD");
                        
                        // 3. Wait for confirmation from Control ECU
                        char* control_response = ReceiveResponseFromControl();
                        
                        if(strcmp(control_response, "PWD_SAVED") == 0) {
                            // SUCCESS: Password saved in EEPROM
                            LCD_Clear();
                            LCD_String("Password Created!");
                            DIO_WritePin(PORTF, PIN3, HIGH); // Green LED - success
                            delayMs(2000);
                            DIO_WritePin(PORTF, PIN3, LOW);

                            password_created = true; // Mark password as created
                            
                            // Go to Main Menu
                            state = STATE_MAIN_MENU;
                            LCD_Clear();
                            LCD_SetCursor(1, 0);
                            LCD_String("Menu>A:Opn B:PWD"); 
                            LCD_SetCursor(2, 0);
                            LCD_String(" C:TMO  D:Reset");      
                        } 
                        else if(strcmp(control_response, "TIMEOUT") == 0) {
                            // TIMEOUT: No response from Control ECU
                            LCD_Clear();
                            LCD_String("No Response"); 
                            LCD_SetCursor(2, 0);
                            LCD_String("from Control");
                            delayMs(2000);
                            
                            // Restart password creation
                            state = STATE_CREATE_PASS;
                            LCD_Clear();
                            LCD_String("CreatePass:");
                        }
                        else {
                            // ERROR: Password not saved or unexpected response
                            LCD_Clear();
                            LCD_String("Error Saving PWD"); 
                            delayMs(2000);
                            
                            // Restart password creation
                            state = STATE_CREATE_PASS;
                            LCD_Clear();
                            LCD_String("CreatePass:");
                        }
                    } 
                    else 
                    {
                        LCD_String("Mismatch! Retry"); // Local typing didn't match
                        delayMs(2000); 
                        
                        state = STATE_CREATE_PASS;
                        LCD_Clear();
                        LCD_String("CreatePass:");
                    }
                }
            }
            // C. Handle '*' (Clear/Cancel)
            else if(key == '*')
            {
                index = 0; 
                LCD_Clear();
                if (state == STATE_CREATE_PASS) {
                     LCD_String("CreatePass:");
                } else if (state == STATE_CONFIRM_PASS) {
                     LCD_String("Confirm:");
                }
            }
        }
    }
}