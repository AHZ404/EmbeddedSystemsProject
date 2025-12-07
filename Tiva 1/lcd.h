#ifndef LCD_H_
#define LCD_H_

void LCD_Init(void);
void LCD_Command(unsigned char cmd);
void LCD_Char(unsigned char data);
void LCD_String(char *str);
void LCD_Clear(void);
void LCD_SetCursor(unsigned char row, unsigned char col);

#endif
