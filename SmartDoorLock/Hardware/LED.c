#include "stm32f10x.h"                  // Device header

void LED_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef GPIO_Structure;
	GPIO_Structure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Structure.GPIO_Pin=GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_Structure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_Structure);
	
	GPIO_SetBits(GPIOA,GPIO_Pin_1|GPIO_Pin_2);//先让灯灭
}

void LED1_ON(void)
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_1);
}

void LED1_OFF(void)
{
	GPIO_SetBits(GPIOA,GPIO_Pin_1);
}

void LED2_ON(void)
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_2);
}

void LED2_OFF(void)
{
	GPIO_SetBits(GPIOA,GPIO_Pin_2);
}

void LED1_Turn(void)//翻转状态
{
	if(GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_1))
	{
		GPIO_ResetBits(GPIOA,GPIO_Pin_1);
	}
	else
	{
		GPIO_SetBits(GPIOA,GPIO_Pin_1);
	}
}

void LED2_Turn(void)//翻转状态
{
	if(GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_2))
	{
		GPIO_ResetBits(GPIOA,GPIO_Pin_2);
	}
	else
	{
		GPIO_SetBits(GPIOA,GPIO_Pin_2);
	}
}

