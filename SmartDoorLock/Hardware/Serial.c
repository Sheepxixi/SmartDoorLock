#include "stm32f10x.h"                  // Device header
#include "stdio.h"
#include "stdarg.h" //�ú����ܹ����ܿɱ����

//�ȳ�ʼ��USART
void Serial_Init(void)
{
	//����ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	//GPIO��ʼ����TX����Ϊ�������ģʽ��RXΪ����
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	//����USART��ͨ���ṹ��ķ�ʽ
	USART_InitTypeDef USART_InitStructure;					//����ṹ�����
	USART_InitStructure.USART_BaudRate = 115200;				//������
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//Ӳ�������ƣ�����Ҫ
	USART_InitStructure.USART_Mode = USART_Mode_Tx;			//ģʽ��ѡ��Ϊ����ģʽ
	USART_InitStructure.USART_Parity = USART_Parity_No;		//��żУ�飬����Ҫ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//ֹͣλ��ѡ��1λ
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//�ֳ���ѡ��8λ
	USART_Init(USART1, &USART_InitStructure);				//���ṹ���������USART_Init������USART1
	
	//�����Ҫ���յĹ��ܣ������������ж�
	//������2�ַ�������ѯ �� �жϣ�ʹ���жϾ�Ҫ����һ��
	
	
	//����USART
	USART_Cmd(USART1, ENABLE);								//ʹ��USART1�����ڿ�ʼ����

}

//��ʼ�����֮�����Ҫ�շ����ݣ��͵��ý���/���͵Ŀ⺯�������Ҫ��ȡ���ͻ���յ�״̬���͵��û�ȡ��־λ�ĺ���

//�����ֽں���
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1,(uint16_t)Byte);
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE)==RESET);//TXE == SET�����ͼĴ���Ϊ��,�����ǵȴ��������
	//TXE����һ�η���ǰ���Զ����㣬����Ҫ�ֶ�����
}
//�������ĺ�����ʵ���Ƕ�Serial_SendByte�����ķ�װ

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
		//ȡ��Number��ÿһλ����12345
		Serial_SendByte(Number / Serial_Pow(10,Length-i-1)%10+'0');
	}
}

//fputc��printf�����ĵײ㣬printf������ӡʱ�����ڲ��ϵ���fputc����
int fputc(int ch,FILE *f)
{
	Serial_SendByte(ch);//�ض��򵽴���
	return ch;
}

//��װsprintf�Ĺ���,C���ԵĿɱ亯������
void Serial_Printf(char *format,...)
{
	char String[100];
	va_list arg;
	va_start(arg,format);//��formatλ�ÿ�ʼ���ղ���������arg��
	vsprintf(String,format,arg);//sprintfֻ�ܽ���ֱ��д�Ĳ�������װ��ʽ�Ĳ���Ҫ��vsprintf
	va_end(arg);
	Serial_SendString(String);
}







