#include "Timer.h"
#include "stm32f10x.h"                  // Device header

extern vu16 USART2_RX_STA;

/*
TIM4初始化：
时钟默认使用APB1时钟，时钟频率为36MHZ，计数器计36个数为1us
定时中断时间：(arr+1)*((psc+1)/36) us，其中arr是自动重装载值，psc是预分频值
(arr+1)*((psc+1)/36) us = 10ms=10000us
(arr+1)*((psc+1)=360000,可令arr+1=3600,psc+1=100
*/
void TIM4_Int_Init(u16 arr,u16 psc)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	
	//时基单元
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	TIM_TimeBaseInitStruct.TIM_Period=arr;// 设置定时器的自动重装载值,即计数器到达此值时重新开始计数
	TIM_TimeBaseInitStruct.TIM_Prescaler=psc;// 设置定时器的预分频值,即时钟频率被除以 (psc + 1)
	TIM_TimeBaseInitStruct.TIM_CounterMode=TIM_CounterMode_Up;//向上计数模式
	TIM_TimeBaseInitStruct.TIM_ClockDivision=TIM_CKD_DIV1;//设置时钟分频因子,这里选择不分频，输入捕获功能相关，常用值为 TIM_CKD_DIV1
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseInitStruct);
	
	//开启中断通道
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
	
	TIM_Cmd(TIM4,ENABLE);//下一次接收开始时重新启动
	
	//配置NVIC
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//响应优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	
}

//定时器中断处理函数
void TIM4_IRQHandler(void)
{
	//计数器已经达到预设的重装值，产生了中断
	if(TIM_GetITStatus(TIM4,TIM_IT_Update)== SET)
	{
		//超时，判断为不是同一批数据
		USART2_RX_STA|=(1<<15);
		//清除中断标志位
		TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
		//同时关闭中断（这次接收完毕了，不需要再计时）
		TIM_Cmd(TIM4,DISABLE);
	}
}
