#ifndef PASSWORD_DB_H_
#define PASSWORD_DB_H_

#define MAX_PASSWORD_LENGTH 5
#define CORRECT_PASSWORD "12345"
#define MAX_ATTEMPTS 3

void PasswordDB_Init(void);
uint8_t PasswordDB_Verify(const char* password);
void PasswordDB_ResetAttempts(void);
uint8_t PasswordDB_IsLocked(void);

#endif