#ifndef _AS608_USE_H_
#define _AS608_USE_H_
#include "stm32f10x.h"                  // Device header
#include "AS608.h"						/*指纹模块初始化*/
#include "Delay.h"						/*延时函数*/
void ShowErrorMessage(u8 ensure);
void Press_Finger(void);
void Add_Finger(u16 FG_ID);
void Del_FR(u16 FG_ID);
#endif
