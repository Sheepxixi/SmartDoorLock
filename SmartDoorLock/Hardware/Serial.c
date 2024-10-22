#include "stm32f10x.h"                  // Device header
#include "stdio.h"
#include "stdarg.h" //让函数能够接受可变参数

//先初始化USART
void Serial_Init(void)
{
	//开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	//GPIO初始化：TX配置为复用输出模式，RX为输入
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	//配置USART：通过结构体的方式
	USART_InitTypeDef USART_InitStructure;					//定义结构体变量
	USART_InitStructure.USART_BaudRate = 115200;				//波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制，不需要
	USART_InitStructure.USART_Mode = USART_Mode_Tx;			//模式，选择为发送模式
	USART_InitStructure.USART_Parity = USART_Parity_No;		//奇偶校验，不需要
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//停止位，选择1位
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长，选择8位
	USART_Init(USART1, &USART_InitStructure);				//将结构体变量交给USART_Init，配置USART1
	
	//如果需要接收的功能，还可以配置中断
	//接收有2种方法：查询 和 中断，使用中断就要配置一下
	
	
	//开启USART
	USART_Cmd(USART1, ENABLE);								//使能USART1，串口开始运行

}

//初始化完成之后，如果要收发数据，就调用接收/发送的库函数；如果要获取发送或接收的状态，就调用获取标志位的函数

//发送字节函数
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1,(uint16_t)Byte);
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE)==RESET);//TXE == SET：发送寄存器为空,这里是等待发送完毕
	//TXE在下一次发送前会自动清零，不需要手动清零
}
//接下来的函数其实都是对Serial_SendByte函数的封装

void Serial_SendArray(uint8_t *Array,uint16_t Length)
{
	uint16_t i;
	for(i=0;i<Length;++i)
	{
		Serial_SendByte(Array[i]);
	}
}

void Serial_SendString(char *String)
{
	uint16_t i;
	for(i=0;String[i]!='\0';++i)
	{
		Serial_SendByte(String[i]);
	}
//	while(*String)
//	{
//		Serial_SendByte(String[i]);
//		++i;
//	}
}

uint32_t Serial_Pow(uint32_t X , uint32_t Y)
{
	uint32_t result=1;
	while(Y--)
	{
		result*=X;
	}
	return result;
}

void Serial_SendNumber(uint32_t Number,uint8_t Length)
{
	uint8_t i;
	for(i=0;i<Length;++i)
	{
		//取出Number的每一位：如12345
		Serial_SendByte(Number / Serial_Pow(10,Length-i-1)%10+'0');
	}
}

//fputc是printf函数的底层，printf函数打印时就是在不断调用fputc函数
int fputc(int ch,FILE *f)
{
	Serial_SendByte(ch);//重定向到串口
	return ch;
}

//封装sprintf的过程,C语言的可变函数参数
void Serial_Printf(char *format,...)
{
	char String[100];
	va_list arg;
	va_start(arg,format);//从format位置开始接收参数表，放在arg里
	vsprintf(String,format,arg);//sprintf只能接收直接写的参数，封装格式的参数要用vsprintf
	va_end(arg);
	Serial_SendString(String);
}







