#include "stm32f10x.h"                  // Device header
#include "Keyboard.h"
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"

u8 Key_Data=0;  //按键按下的数据(8个位(高4位是列，低4位是行)的高低电平)
u8 Key_flag=0;  //按键按下标志位，区分按键按下一次的

static uint8_t See_Flag=0;//是否显示密码的标志位
extern u8 buf[4];

void Keyboard_ScanRows_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
		
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;//下拉输入
	GPIO_InitStructure.GPIO_Pin=C1_pin|C2_pin|C3_pin|C4_pin;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;//推挽输出
    GPIO_InitStructure.GPIO_Pin=R1_pin|R2_pin|R3_pin|R4_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	keyR1_W_1;  keyR2_W_1;  keyR3_W_1;  keyR4_W_1;//全部行先置1，按键按下对应行会改为0
}

void Keyboard_ScanCols_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
		
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;//下拉输入
	GPIO_InitStructure.GPIO_Pin=R1_pin|R2_pin|R3_pin|R4_pin;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;//推挽输出
    GPIO_InitStructure.GPIO_Pin=C1_pin|C2_pin|C3_pin|C4_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	keyC1_W_1;  keyC2_W_1;  keyC3_W_1;  keyC4_W_1;//全部列先置1，按键按下对应列会改为0
}

void R_Keyboard(unsigned char z)
{
	Key_Data=0x00;//每次都要清零，否则上一次的拼接会影响到这一次
	//准备阶段，初始化
    if(z==0)
    {
        Keyboard_ScanRows_Init();//行扫描
    }
    else
    {
        Keyboard_ScanCols_Init();//列扫描
    }
	Delay_us(200);
    
    //再读取8个IO口的高低电平
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
	u8 num=16;//先赋一个不是数字的按键值
	u8 i;
	u8 Key_x;//按键所在行/列的值
	R_Keyboard(0);//先扫描行
    Delay_us(100);
	if(Key_Data!=0x0f)
    {	
		Key_flag=1; //有按键按下，标志位置1
        Key_x=Key_Data;//将此时的键值赋值
		R_Keyboard(1); //切换状态 1 
		Delay_us(10);  //短延时一下
		Key_Data=Key_Data&Key_x;  //行列数据按位与，就可以定位按键
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
		//一直检测按键是否松开
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
					 OLED_ShowChar(51 + i * 19, 0, buf[i], 8);  // 显示密码字符
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

