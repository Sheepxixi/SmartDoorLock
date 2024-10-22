#ifndef __OLED_H
#define __OLED_H
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ClearPlace(uint8_t x,uint8_t y,uint8_t width,uint8_t height);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t ch, uint8_t size);
void OLED_ShowString(uint8_t x,uint8_t y,char *ch,uint8_t temp);
int OLED_Getpos(unsigned char length);
void OLED_ShowNum(unsigned char x,unsigned char y,unsigned int Num,unsigned char length);
void OLED_WriteChinese(uint8_t x,uint8_t y,uint8_t pos,uint8_t count);
void OLED_Write_N_Chinese(uint8_t x,uint8_t y,uint8_t pos,uint8_t length);
#endif
