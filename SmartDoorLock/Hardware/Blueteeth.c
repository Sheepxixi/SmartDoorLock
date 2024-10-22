#include "stm32f10x.h"                  // Device header
#include "stdio.h"
/*
蓝牙使用串口3与单片机进行通信。
串口3的TX:PB10,RX:PB11
*/
//初始化串口3
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
	
	//配置串口控制器
	USART_InitStructure.USART_BaudRate=9600;  // 设置波特率为 9600
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;  // 不使用硬件流控制
	USART_InitStructure.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;  // 设置为收发模式
	USART_InitStructure.USART_Parity=USART_Parity_No;  // 无奇偶校验
	USART_InitStructure.USART_StopBits=USART_StopBits_1;  // 设置停止位为 1 位
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;  // 设置字长为 8 位
	USART_Init(USART3,&USART_InitStructure);  // 初始化 USART3，传入配置结构体
	
	//打开串口通道,启用 USART_IT_RXNE 使得 USART3 在接收到数据时能够触发中断
	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);

	//配置NVIC
	NVIC_InitStructure.NVIC_IRQChannel=USART3_IRQn;  // 设置中断通道为 USART3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;  // 使能中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;  // 设置抢占优先级为 0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=1;  // 设置子优先级为 1
	NVIC_Init(&NVIC_InitStructure);  // 初始化 NVIC 配置
	
	//使能USART3
	USART_Cmd(USART3,ENABLE);
}

u8 res = 0;
u8 res_flag = 0;
void USART3_IRQHandler()
{
	if(USART_GetITStatus(USART3,USART_IT_RXNE)!=RESET)//串口3接收到了数据
	{
		res =  USART_ReceiveData(USART3);//读取接收到的数据
		printf("IRQ:res == %x\r\n",res);
		res_flag = 1;//表示接收到了数据
		USART_ClearITPendingBit(USART3,USART_IT_RXNE);
		//while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==Bit_RESET);	//等待发送寄存器空，确保可以安全发送新数据
	}
}
