#include "stm32f10x.h"                  // Device header
#include "usart2.h"
#include "Timer.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

//串口数据缓存区 	
u8 USART2_RX_BUF[USART2_MAX_RECV_LEN]; 				//接收缓冲,最大USART2_MAX_RECV_LEN个字节.
u8 USART2_TX_BUF[USART2_MAX_SEND_LEN]; 			  //发送缓冲,最大USART2_MAX_SEND_LEN字节

/*
通过判断接收连续2个字符之间的时间差不大于10ms来决定是不是一次连续的数据.
如果2个字符接收间隔超过10ms,则认为不是1次连续数据.也就是超过10ms没有接收到任何数据,则表示此次接收完毕.
vu16:volatile unsigned 16-bit integer（易失的无符号16位整数）。
数据第15位	:若=1,接收完成（缓冲区已满/不属于一批数据）
数据第14-0位:表示接收到的数据长度
*/
vu16 USART2_RX_STA=0; //USART2 接收状态(status)，也是个数据量

//这里实时性要求高，串口接收要采用中断,中断处理函数如下：
void USART2_IRQHandler(void)
{
	u8 res;
	//接收寄存器非空,说明有数据未被取走
	if(USART_GetITStatus(USART2,USART_IT_RXNE)==SET)
	{
		//接收数据
		res=USART_ReceiveData(USART2);
		//查看第15位，接收未完成
		if((USART2_RX_STA&(1<<15))==0)
		{
			//缓冲区还没满
			if(USART2_RX_STA<USART2_MAX_RECV_LEN)
			{
				//清空定时器的计数器
				TIM_SetCounter(TIM4,0);
				//如果是接收第一个数据，则启动定时器
				if(USART2_RX_STA==0)
				{
					TIM_Cmd(TIM4,ENABLE);
				}
				//记录接收到的值，同时数据量加1
				USART2_RX_BUF[USART2_RX_STA++]=res;		 
			}
			//缓冲区满，接收完毕
			else
			{
				USART2_RX_STA|=(1<<15);
			}
		}
	}
}

//串口初始化：TX：PA2，RX：PA3
//参数：bound:波特率	  
void usart2_init(u32 bound)
{
	//开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	//串口2挂载在APB1
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE); 
	
	USART_DeInit(USART2);  //复位串口2，确保初始化过程的一致性和稳定性
	
	//配置GPIO口
	GPIO_InitTypeDef GPIO_InitStructure;
	//TX：复用推挽输出
	//推挽输出意味着引脚可以在高电平和低电平之间快速切换，可以确保信号输出稳定，尤其适合高速串行通信
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	//RX：通用浮空输入
	//串口接收引脚仅用来感知外部数据，浮空输入确保接收到的外部信号不被内部电路干扰
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_3;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	//配置USART控制器
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_Mode=USART_Mode_Tx|USART_Mode_Rx;
	USART_InitStructure.USART_BaudRate=bound;//波特率
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;//数据长度
	USART_InitStructure.USART_StopBits=USART_StopBits_1;//停止位长度
	USART_InitStructure.USART_Parity=USART_Parity_No;//校验位
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;//不使用硬件控制流
	USART_Init(USART2, &USART_InitStructure); //初始化串口2
	
	//使能串口
	USART_Cmd(USART2, ENABLE); 
	
	//使能接收中断
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启中断
	
	//配置NVIC，设置中断优先级
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	
	
	TIM4_Int_Init(99,3599);
	USART2_RX_STA=0;//清零
	TIM_Cmd(TIM4,DISABLE);//关闭定时器4，用的时候再启用
}

void u2_printf(char* fmt,...)  
{  
    u16 i,j;                       // 定义两个16位无符号整数变量 i 和 j，用于存储数据长度和循环计数。
    va_list ap;                    // 定义一个 va_list 类型的变量 ap，用于处理可变参数列表。

    va_start(ap, fmt);             // 初始化 va_list 变量 ap，使其指向格式化字符串中的第一个可变参数。

    // 使用 vsprintf 函数将可变参数按照 fmt 指定的格式格式化成字符串，
    // 并将结果存储在 USART2_TX_BUF 缓冲区中。
    vsprintf((char*)USART2_TX_BUF, fmt, ap); 

    va_end(ap);                    // 结束可变参数处理，清理 va_list 变量 ap，释放资源。

    // 获取已经格式化好的字符串长度，即要通过 USART2 发送的字节数。
    i = strlen((const char*)USART2_TX_BUF);		

    // 循环遍历缓冲区中的每个字符，将它们通过 USART2 逐字节发送出去。
    for(j = 0; j < i; j++)						
    {
        // 检查 USART2当前是否有数据在发送
        // 如果还没有发送完毕 (USART_FLAG_TC(传输完成标志)没有置位)，则等待
        while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET); 
        
        // 当确认当前数据发送完毕后，将缓冲区中的下一个字符发送出去
        USART_SendData(USART2, USART2_TX_BUF[j]); 
    } 
}
