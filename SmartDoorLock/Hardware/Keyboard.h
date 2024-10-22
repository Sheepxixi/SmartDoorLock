#ifndef __KEYBOARD_H
#define __KEYBOARD_H

/*
矩阵按键引脚：
C4		PB9
C3		PB8
C2		PB7
C1		PB6
R4		PB5
R3		PB4
R2		PB1
R1		PB0
*/

//引脚别名定义
#define C4_port GPIOB
#define C3_port GPIOB
#define C2_port GPIOB
#define C1_port GPIOB  

#define R4_port GPIOB
#define R3_port GPIOB
#define R2_port GPIOB
#define R1_port GPIOB

#define C4_pin GPIO_Pin_9
#define C3_pin GPIO_Pin_8
#define C2_pin GPIO_Pin_7
#define C1_pin GPIO_Pin_6

#define R4_pin GPIO_Pin_5
#define R3_pin GPIO_Pin_4
#define R2_pin GPIO_Pin_1
#define R1_pin GPIO_Pin_0

//置引脚为高/低电平
#define keyC1_W_1 GPIO_SetBits(C1_port, C1_pin)
#define keyC1_W_0 GPIO_ResetBits(C1_port, C1_pin)

#define keyC2_W_1 GPIO_SetBits(C2_port, C2_pin)
#define keyC2_W_0 GPIO_ResetBits(C2_port, C2_pin)

#define keyC3_W_1 GPIO_SetBits(C3_port, C3_pin)
#define keyC3_W_0 GPIO_ResetBits(C3_port, C3_pin)

#define keyC4_W_1 GPIO_SetBits(C4_port, C4_pin)
#define keyC4_W_0 GPIO_ResetBits(C4_port, C4_pin)

#define keyR1_W_1 GPIO_SetBits(R1_port, R1_pin)
#define keyR1_W_0 GPIO_ResetBits(R1_port, R1_pin)

#define keyR2_W_1 GPIO_SetBits(R2_port, R2_pin)
#define keyR2_W_0 GPIO_ResetBits(R2_port, R2_pin)

#define keyR3_W_1 GPIO_SetBits(R3_port, R3_pin)
#define keyR3_W_0 GPIO_ResetBits(R3_port, R3_pin)

#define keyR4_W_1 GPIO_SetBits(R4_port, R4_pin)
#define keyR4_W_0 GPIO_ResetBits(R4_port, R4_pin)

#define keyC1_R GPIO_ReadInputDataBit(C1_port,C1_pin)
#define keyC2_R GPIO_ReadInputDataBit(C2_port,C2_pin)
#define keyC3_R GPIO_ReadInputDataBit(C3_port,C3_pin)
#define keyC4_R GPIO_ReadInputDataBit(C4_port,C4_pin)
#define keyR1_R GPIO_ReadInputDataBit(R1_port,R1_pin)
#define keyR2_R GPIO_ReadInputDataBit(R2_port,R2_pin)
#define keyR3_R GPIO_ReadInputDataBit(R3_port,R3_pin)
#define keyR4_R GPIO_ReadInputDataBit(R4_port,R4_pin) 


#define SEE 10
#define CLEAR 11
#define UP 12 
#define DOWN 13
#define BACK 14
#define ACK 15 
	
uint8_t Key_Scan(void);
#endif
