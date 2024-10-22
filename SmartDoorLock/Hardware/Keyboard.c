#include "stm32f10x.h"                  // Device header
#include "Keyboard.h"
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"

u8 Key_Data=0;  //�������µ�����(8��λ(��4λ���У���4λ����)�ĸߵ͵�ƽ)
u8 Key_flag=0;  //�������±�־λ�����ְ�������һ�ε�

static uint8_t See_Flag=0;//�Ƿ���ʾ����ı�־λ
extern u8 buf[4];

void Keyboard_ScanRows_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
		
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;//��������
	GPIO_InitStructure.GPIO_Pin=C1_pin|C2_pin|C3_pin|C4_pin;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;//�������
    GPIO_InitStructure.GPIO_Pin=R1_pin|R2_pin|R3_pin|R4_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	keyR1_W_1;  keyR2_W_1;  keyR3_W_1;  keyR4_W_1;//ȫ��������1���������¶�Ӧ�л��Ϊ0
}

void Keyboard_ScanCols_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
		
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;//��������
	GPIO_InitStructure.GPIO_Pin=R1_pin|R2_pin|R3_pin|R4_pin;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;//�������
    GPIO_InitStructure.GPIO_Pin=C1_pin|C2_pin|C3_pin|C4_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	keyC1_W_1;  keyC2_W_1;  keyC3_W_1;  keyC4_W_1;//ȫ��������1���������¶�Ӧ�л��Ϊ0
}

void R_Keyboard(unsigned char z)
{
	Key_Data=0x00;//ÿ�ζ�Ҫ���㣬������һ�ε�ƴ�ӻ�Ӱ�쵽��һ��
	//׼���׶Σ���ʼ��
    if(z==0)
    {
        Keyboard_ScanRows_Init();//��ɨ��
    }
    else
    {
        Keyboard_ScanCols_Init();//��ɨ��
    }
	Delay_us(200);
    
    //�ٶ�ȡ8��IO�ڵĸߵ͵�ƽ
    if(keyC1_R==1)    Key_Data |= 0x80;
    else    Key_Data |= 0x00;
    
    if(keyC2_R==1)    Key_Data |= 0x40;
    else    Key_Data |= 0x00;
    
    if(keyC3_R==1)    Key_Data |= 0x20;
    else    Key_Data |= 0x00;
    
    if(keyC4_R==1)    Key_Data |= 0x10;
    else    Key_Data |= 0x00;
    
    if(keyR1_R==1)    Key_Data |= 0x08;
    else    Key_Data |= 0x00;
    
    if(keyR2_R==1)    Key_Data |= 0x04;
    else    Key_Data |= 0x00;
    
    if(keyR3_R==1)    Key_Data |= 0x02;
    else    Key_Data |= 0x00;
    
    if(keyR4_R==1)    Key_Data |= 0x01;
    else    Key_Data |= 0x00;
}

uint8_t Key_Scan(void)
{
	u8 num=16;//�ȸ�һ���������ֵİ���ֵ
	u8 i;
	u8 Key_x;//����������/�е�ֵ
	R_Keyboard(0);//��ɨ����
    Delay_us(100);
	if(Key_Data!=0x0f)
    {	
		Key_flag=1; //�а������£���־λ��1
        Key_x=Key_Data;//����ʱ�ļ�ֵ��ֵ
		R_Keyboard(1); //�л�״̬ 1 
		Delay_us(10);  //����ʱһ��
		Key_Data=Key_Data&Key_x;  //�������ݰ�λ�룬�Ϳ��Զ�λ����
		printf("Key_Data = %x\r\n",Key_Data);
		switch(Key_Data)
	    {
		 case 0x88 : num=1;   break; 
		 case 0x48 : num=2;   break;
		 case 0x28 : num=3;   break;
		 case 0x18 : num=CLEAR;  break;

		 case 0x84 : num=4;   break;
		 case 0x44 : num=5;   break;
		 case 0x24 : num=6;   break;
		 case 0x14 : num=UP;  break;

		 case 0x82 : num=7;   break;
		 case 0x42 : num=8;   break;
		 case 0x22 : num=9;   break;
		 case 0x12 : num=DOWN;  break;

		 case 0x81 : num=BACK;  break;
		 case 0x41 : num=0;   break;
		 case 0x21 : num=SEE;  
					 See_Flag=!See_Flag;
					 break;
		 case 0x11 : num=ACK;  break;
	    }
		//һֱ��ⰴ���Ƿ��ɿ�
		while(Key_Data!=0x0f)
		{
			R_Keyboard(0);
		}
		if(num == SEE)
		{
			for(i=0;i<4;i++)
			{
				if(See_Flag)
				{
					 OLED_ShowChar(51 + i * 19, 0, buf[i], 8);  // ��ʾ�����ַ�
				}
				else 
				{
					if(buf[i]!='\0')
						OLED_ShowChar(51+i*19,0,'*',8);
				}
										 
			}
		}
    }
	 return num;
}

