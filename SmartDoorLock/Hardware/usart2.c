#include "stm32f10x.h"                  // Device header
#include "usart2.h"
#include "Timer.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

//�������ݻ����� 	
u8 USART2_RX_BUF[USART2_MAX_RECV_LEN]; 				//���ջ���,���USART2_MAX_RECV_LEN���ֽ�.
u8 USART2_TX_BUF[USART2_MAX_SEND_LEN]; 			  //���ͻ���,���USART2_MAX_SEND_LEN�ֽ�

/*
ͨ���жϽ�������2���ַ�֮���ʱ������10ms�������ǲ���һ������������.
���2���ַ����ռ������10ms,����Ϊ����1����������.Ҳ���ǳ���10msû�н��յ��κ�����,���ʾ�˴ν������.
vu16:volatile unsigned 16-bit integer����ʧ���޷���16λ��������
���ݵ�15λ	:��=1,������ɣ�����������/������һ�����ݣ�
���ݵ�14-0λ:��ʾ���յ������ݳ���
*/
vu16 USART2_RX_STA=0; //USART2 ����״̬(status)��Ҳ�Ǹ�������

//����ʵʱ��Ҫ��ߣ����ڽ���Ҫ�����ж�,�жϴ��������£�
void USART2_IRQHandler(void)
{
	u8 res;
	//���ռĴ����ǿ�,˵��������δ��ȡ��
	if(USART_GetITStatus(USART2,USART_IT_RXNE)==SET)
	{
		//��������
		res=USART_ReceiveData(USART2);
		//�鿴��15λ������δ���
		if((USART2_RX_STA&(1<<15))==0)
		{
			//��������û��
			if(USART2_RX_STA<USART2_MAX_RECV_LEN)
			{
				//��ն�ʱ���ļ�����
				TIM_SetCounter(TIM4,0);
				//����ǽ��յ�һ�����ݣ���������ʱ��
				if(USART2_RX_STA==0)
				{
					TIM_Cmd(TIM4,ENABLE);
				}
				//��¼���յ���ֵ��ͬʱ��������1
				USART2_RX_BUF[USART2_RX_STA++]=res;		 
			}
			//�����������������
			else
			{
				USART2_RX_STA|=(1<<15);
			}
		}
	}
}

//���ڳ�ʼ����TX��PA2��RX��PA3
//������bound:������	  
void usart2_init(u32 bound)
{
	//����ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	//����2������APB1
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE); 
	
	USART_DeInit(USART2);  //��λ����2��ȷ����ʼ�����̵�һ���Ժ��ȶ���
	
	//����GPIO��
	GPIO_InitTypeDef GPIO_InitStructure;
	//TX�������������
	//���������ζ�����ſ����ڸߵ�ƽ�͵͵�ƽ֮������л�������ȷ���ź�����ȶ��������ʺϸ��ٴ���ͨ��
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	//RX��ͨ�ø�������
	//���ڽ������Ž�������֪�ⲿ���ݣ���������ȷ�����յ����ⲿ�źŲ����ڲ���·����
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_3;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	//����USART������
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_Mode=USART_Mode_Tx|USART_Mode_Rx;
	USART_InitStructure.USART_BaudRate=bound;//������
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;//���ݳ���
	USART_InitStructure.USART_StopBits=USART_StopBits_1;//ֹͣλ����
	USART_InitStructure.USART_Parity=USART_Parity_No;//У��λ
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;//��ʹ��Ӳ��������
	USART_Init(USART2, &USART_InitStructure); //��ʼ������2
	
	//ʹ�ܴ���
	USART_Cmd(USART2, ENABLE); 
	
	//ʹ�ܽ����ж�
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//�����ж�
	
	//����NVIC�������ж����ȼ�
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	
	
	TIM4_Int_Init(99,3599);
	USART2_RX_STA=0;//����
	TIM_Cmd(TIM4,DISABLE);//�رն�ʱ��4���õ�ʱ��������
}

void u2_printf(char* fmt,...)  
{  
    u16 i,j;                       // ��������16λ�޷����������� i �� j�����ڴ洢���ݳ��Ⱥ�ѭ��������
    va_list ap;                    // ����һ�� va_list ���͵ı��� ap�����ڴ���ɱ�����б�

    va_start(ap, fmt);             // ��ʼ�� va_list ���� ap��ʹ��ָ���ʽ���ַ����еĵ�һ���ɱ������

    // ʹ�� vsprintf �������ɱ�������� fmt ָ���ĸ�ʽ��ʽ�����ַ�����
    // ��������洢�� USART2_TX_BUF �������С�
    vsprintf((char*)USART2_TX_BUF, fmt, ap); 

    va_end(ap);                    // �����ɱ������������ va_list ���� ap���ͷ���Դ��

    // ��ȡ�Ѿ���ʽ���õ��ַ������ȣ���Ҫͨ�� USART2 ���͵��ֽ�����
    i = strlen((const char*)USART2_TX_BUF);		

    // ѭ�������������е�ÿ���ַ���������ͨ�� USART2 ���ֽڷ��ͳ�ȥ��
    for(j = 0; j < i; j++)						
    {
        // ��� USART2��ǰ�Ƿ��������ڷ���
        // �����û�з������ (USART_FLAG_TC(������ɱ�־)û����λ)����ȴ�
        while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET); 
        
        // ��ȷ�ϵ�ǰ���ݷ�����Ϻ󣬽��������е���һ���ַ����ͳ�ȥ
        USART_SendData(USART2, USART2_TX_BUF[j]); 
    } 
}
