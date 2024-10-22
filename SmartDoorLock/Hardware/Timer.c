#include "Timer.h"
#include "stm32f10x.h"                  // Device header

extern vu16 USART2_RX_STA;

/*
TIM4��ʼ����
ʱ��Ĭ��ʹ��APB1ʱ�ӣ�ʱ��Ƶ��Ϊ36MHZ����������36����Ϊ1us
��ʱ�ж�ʱ�䣺(arr+1)*((psc+1)/36) us������arr���Զ���װ��ֵ��psc��Ԥ��Ƶֵ
(arr+1)*((psc+1)/36) us = 10ms=10000us
(arr+1)*((psc+1)=360000,����arr+1=3600,psc+1=100
*/
void TIM4_Int_Init(u16 arr,u16 psc)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	
	//ʱ����Ԫ
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	TIM_TimeBaseInitStruct.TIM_Period=arr;// ���ö�ʱ�����Զ���װ��ֵ,�������������ֵʱ���¿�ʼ����
	TIM_TimeBaseInitStruct.TIM_Prescaler=psc;// ���ö�ʱ����Ԥ��Ƶֵ,��ʱ��Ƶ�ʱ����� (psc + 1)
	TIM_TimeBaseInitStruct.TIM_CounterMode=TIM_CounterMode_Up;//���ϼ���ģʽ
	TIM_TimeBaseInitStruct.TIM_ClockDivision=TIM_CKD_DIV1;//����ʱ�ӷ�Ƶ����,����ѡ�񲻷�Ƶ�����벶������أ�����ֵΪ TIM_CKD_DIV1
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseInitStruct);
	
	//�����ж�ͨ��
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
	
	TIM_Cmd(TIM4,ENABLE);//��һ�ν��տ�ʼʱ��������
	
	//����NVIC
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//��Ӧ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	
}

//��ʱ���жϴ�����
void TIM4_IRQHandler(void)
{
	//�������Ѿ��ﵽԤ�����װֵ���������ж�
	if(TIM_GetITStatus(TIM4,TIM_IT_Update)== SET)
	{
		//��ʱ���ж�Ϊ����ͬһ������
		USART2_RX_STA|=(1<<15);
		//����жϱ�־λ
		TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
		//ͬʱ�ر��жϣ���ν�������ˣ�����Ҫ�ټ�ʱ��
		TIM_Cmd(TIM4,DISABLE);
	}
}
