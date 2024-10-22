#include "stm32f10x.h"                  // Device header
#include "stdio.h"
/*
����ʹ�ô���3�뵥Ƭ������ͨ�š�
����3��TX:PB10,RX:PB11
*/
//��ʼ������3
void uart3_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
	
	USART_DeInit(USART3);
	
	//GPIO
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	//���ô��ڿ�����
	USART_InitStructure.USART_BaudRate=9600;  // ���ò�����Ϊ 9600
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;  // ��ʹ��Ӳ��������
	USART_InitStructure.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;  // ����Ϊ�շ�ģʽ
	USART_InitStructure.USART_Parity=USART_Parity_No;  // ����żУ��
	USART_InitStructure.USART_StopBits=USART_StopBits_1;  // ����ֹͣλΪ 1 λ
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;  // �����ֳ�Ϊ 8 λ
	USART_Init(USART3,&USART_InitStructure);  // ��ʼ�� USART3���������ýṹ��
	
	//�򿪴���ͨ��,���� USART_IT_RXNE ʹ�� USART3 �ڽ��յ�����ʱ�ܹ������ж�
	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);

	//����NVIC
	NVIC_InitStructure.NVIC_IRQChannel=USART3_IRQn;  // �����ж�ͨ��Ϊ USART3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;  // ʹ���ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;  // ������ռ���ȼ�Ϊ 0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=1;  // ���������ȼ�Ϊ 1
	NVIC_Init(&NVIC_InitStructure);  // ��ʼ�� NVIC ����
	
	//ʹ��USART3
	USART_Cmd(USART3,ENABLE);
}

u8 res = 0;
u8 res_flag = 0;
void USART3_IRQHandler()
{
	if(USART_GetITStatus(USART3,USART_IT_RXNE)!=RESET)//����3���յ�������
	{
		res =  USART_ReceiveData(USART3);//��ȡ���յ�������
		printf("IRQ:res == %x\r\n",res);
		res_flag = 1;//��ʾ���յ�������
		USART_ClearITPendingBit(USART3,USART_IT_RXNE);
		//while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==Bit_RESET);	//�ȴ����ͼĴ����գ�ȷ�����԰�ȫ����������
	}
}
