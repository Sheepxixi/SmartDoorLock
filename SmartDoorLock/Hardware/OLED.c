#include "stm32f10x.h"
#include "Delay.h"
#include "OLED_Font.h"
/*
OLED所用引脚： 
SCL：PB12 		SDA：PB13
*/

/*引脚配置*/
#define OLED_W_SCL(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_12, (BitAction)(x))//写SCL，主机通过SCL来控制数据传输的时序
#define OLED_W_SDA(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_13, (BitAction)(x))//写SDA，主机通过SDA把数据发给从机
#define OLED_R_SDA()		GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)//读SDA，主机接收从机应答时用到

/*
OLED的引脚初始化：通过I2C协议实现通信,
软件模拟I2C：SCL：PB12 		SDA：PB13
*/
void OLED_IO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	//1、开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	//2、配置GPIO口
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_OD;//开漏输出
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_12|GPIO_Pin_13;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
	//3、空闲状态：SCL和SDA都为高电平
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C开始
  * @param  无
  * @retval 无
  */
void OLED_I2C_Start(void)
{
	//时序开始条件：SCL高电平下，SDA产生下降沿
	
	//先保证SDA为高电平
	OLED_W_SDA(1);
	
	//SCL高电平下，SDA产生下降沿
	OLED_W_SCL(1);
	Delay_us(1);
	
	OLED_W_SDA(0);
	Delay_us(1);
	
	OLED_W_SCL(0);
	Delay_us(1);
}

/**
  * @brief  I2C停止
  * @param  无
  * @retval 无
  */
void OLED_I2C_Stop(void)
{
	//时序结束条件：SCL高电平下，SDA产生上升沿
	
	//先保证SDA为低电平
	OLED_W_SDA(0);
	Delay_us(1);
	
	//SCL高电平下，SDA产生上升沿
	OLED_W_SCL(1);
	Delay_us(1);
	
	OLED_W_SDA(1);
	Delay_us(1);
}

/*
模拟从机反馈主机应答信号，主机接收应答
*/
char OLED_Wait_ACK(void) 
{
	//拉低电平，才能让从机发送应答到SDA线上。
	OLED_W_SCL(0);
	
	OLED_W_SDA(0);//主机释放SDA
	
	//从机发送应答到SDA
	
	
	//高电平下主机读取应答
	OLED_W_SCL(1);
	
	//读取应答
	if(OLED_R_SDA())
	{
		OLED_W_SCL(0);//拉低电平，方便接收下一个数据
		return 1;
	}
	else
	{
		OLED_W_SCL(0);//拉低电平，方便接收下一个数据
		return 0;
	}
}

/**
  * @brief  I2C发送一个字节
  * @param  Byte 要发送的一个字节
  * @retval 无
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for(i=0;i<8;++i)
	{
		//起始条件已经把SCL拉低了，所以这里不用重复操作
		
		//SCL低电平下，写数据
		OLED_W_SDA(Byte&(0x80>>i));
		//SCL置高电平
		OLED_W_SCL(1);
		//从机读数据，这里是从机的事，与我们主机无关
		
		//拉低电平，方便下一次写数据
		OLED_W_SCL(0);
	}
	while(OLED_Wait_ACK());      //等待从机应答
}


//OLED通过特定的控制字节来区分命令和数据的传输,0x00表示写命令，0x40表示写数据
/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x00);		//写命令控制字符
	OLED_I2C_SendByte(Command); 	//写命令
	OLED_I2C_Stop();
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);//从机地址
	OLED_I2C_SendByte(0x40);//写数据控制字符
	OLED_I2C_SendByte(Data);//写数据
	OLED_I2C_Stop();
}

/*
OLED上电的设备初始化
*/
void OLED_Device_Init(void)
{
	//上电延时，必须延时
	Delay_ms(200);
	
	//厂家提供，无需深究。
	OLED_WriteCommand(0xAE); //display off
	OLED_WriteCommand(0x20);	//Set Memory Addressing Mode	
	OLED_WriteCommand(0x10);	//00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	OLED_WriteCommand(0xb0);	//Set Page Start Address for Page Addressing Mode,0-7
	OLED_WriteCommand(0xc8);	//Set COM Output Scan Direction
	OLED_WriteCommand(0x00); //---set low column address
	OLED_WriteCommand(0x10); //---set high column address
	OLED_WriteCommand(0x40); //--set start line address
	OLED_WriteCommand(0x81); //--set contrast control register
	OLED_WriteCommand(0xff); //áá?èμ÷?ú 0x00~0xff
	OLED_WriteCommand(0xa1); //--set segment re-map 0 to 127
	OLED_WriteCommand(0xa6); //--set normal display
	OLED_WriteCommand(0xa8); //--set multiplex ratio(1 to 64)
	OLED_WriteCommand(0x3F); //
	OLED_WriteCommand(0xa4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	OLED_WriteCommand(0xd3); //-set display offset
	OLED_WriteCommand(0x00); //-not offset
	OLED_WriteCommand(0xd5); //--set display clock divide ratio/oscillator frequency
	OLED_WriteCommand(0xf0); //--set divide ratio
	OLED_WriteCommand(0xd9); //--set pre-charge period
	OLED_WriteCommand(0x22); //
	OLED_WriteCommand(0xda); //--set com pins hardware configuration
	OLED_WriteCommand(0x12);
	OLED_WriteCommand(0xdb); //--set vcomh
	OLED_WriteCommand(0x20); //0x20,0.77xVcc
	OLED_WriteCommand(0x8d); //--set DC-DC enable
	OLED_WriteCommand(0x14); //
	OLED_WriteCommand(0xaf); //--turn on oled panel
}

//打开OLED
void OLED_ON()
{
	OLED_WriteCommand(0x8D);//设置电荷泵
	OLED_WriteCommand(0x14);//开启电荷泵
	OLED_WriteCommand(0xAF);//OLED唤醒
}

//关闭OLED
void OLED_OFF()
{
	OLED_WriteCommand(0x8D);//设置电荷泵
	OLED_WriteCommand(0x10);//关闭电荷泵
	OLED_WriteCommand(0xAE);//关闭OLED
}

void OLED_Init(void)
{
	OLED_IO_Init();
	OLED_Device_Init();
	OLED_ON();
	Delay_ms(500);
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无


该OLED分辨率为128*64(像素)，分为128列和8页(每页8个像素)
0xB0是基础命令，用于设置光标页地址；命令格式：0xB0|page；例：0xB0|3=0xB3，表示设置页地址位第3页。
0x10用于设置光标列地址的高四位；命令格式：0x10|((X&0xF0)>>4)；例：当X=20=0x14，X&0xF0=0x10(取出了高四位)；再右移4位=0x01；0x10|0x01=0x11，所以命令是0x11。
0x00用于设置光标列地址的低四位，命令格式：0x00&(X&0x0F)。
*/
void OLED_SetCursor(uint8_t X, uint8_t Y)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}


/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	uint8_t i, j;
	//一页一页清除
	for (j = 0; j < 8; j++)
	{
		//设置光标为当前页的起始地址
		OLED_SetCursor(0, j);
		//再在页里一列一列清除
		for(i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);// 写入数据，同时oled的地址指针(光标)会自动移动到下一列
		}
	}
}
//清除指定位置
void OLED_ClearPlace(uint8_t x,uint8_t y,uint8_t width,uint8_t height)
{
	uint8_t m,n;
	for(m=y;m<height;m++)
	{
		 OLED_SetCursor(0,m);
	   for(n=x;n<width;n++)
		{
		   OLED_WriteData(0x00);
		}
	}
}
/**
  * @brief  OLED显示一个字符
  * @param  x：		列地址，范围：0-127
  * @param  y：		页地址，范围：0-7
  * @param  ch：	要显示的一个字符，范围：ASCII可见字符
  *@param   size：	字体大小；6：6x8字体  8：8x16字体
  * @retval 无
  */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t ch, uint8_t size)
{      	
	uint8_t i,c=0;
	if(ch!='\0')
	{
		//字符索引计算：F6x8字模数组的起始字符是空格sp，其ASCII码=32
		c=ch-32;//求出字符在数组中的偏移量
		//设置光标
		OLED_SetCursor(x,y);
		if(size==6)//6*8
		{
			for(i=0;i<6;i++)
			{
				OLED_WriteData(F6x8[c][i]);
			}
		}
		else if(size==8)//8*16
		{
			//每个字符的高为16像素(字模为16个字节)，而每页只能存8个像素，所以要分2次存
			for(i=0;i<8;i++)
			{
				OLED_WriteData(F8X16[c*16+i]);
			}
			//光标移动到下一页
			OLED_SetCursor(x,++y);
			for(i=0;i<8;i++)
			{
				OLED_WriteData(F8X16[c*16+i+8]);
			}
		}
	}
}
/**
  * @brief  OLED显示字符串
  * @param  x：		列地址，范围：0-127
  * @param  y：		页地址，范围：0-7
  * @param  ch：	要显示的字符串地址
  *@param   temp：	字体大小；6：6x8字体  8：8x16字体
  * @retval 无
  */
void OLED_ShowString(uint8_t x,uint8_t y,char *ch,uint8_t temp)
{
	uint8_t pos=0;
	while(ch[pos]!='\0')
	{
		//先判断当前行是否够显示
		if(temp==6)
		{
			if(x>=122)
			{
				x=0;
				y++;
			}
		}
		else if(temp==8)
		{
			if(x>=120)
			{
				x=0;
				y+=2;//注意这里要加2，因为高度是16像素，占2页
			}
		}
		//显示该字符
		OLED_ShowChar(x,y,ch[pos],temp);
		pos++;//下一个字符在字符串的位置
		x+=temp;//oled下一个可以显示的位置
	}
}

////次方函数：计算 10 的 length 次方
//int OLED_Getpos(unsigned char length)
//{
//	unsigned int pos = 1;
//	while(length--)
//	{
//	   pos *= 10;
//	}
//	return pos;
//}

////显示数字函数：数字宽为8
////x:0 ~ 127
////y:0 ~ 63（像素点）
////Num:要显示的数字
////length:数字的位数 0,1,2,3...
//void OLED_ShowNum(unsigned char x,unsigned char y,unsigned int Num,unsigned char length)
//{
//	unsigned char i;
//	for(i=length;i>0;i--)
//	{
//		OLED_ShowChar(x,y,'0'+Num/OLED_Getpos(--length)%10,8);
//		x+=8;
//	}
//}

void showNum(uint8_t x,uint8_t y,uint8_t num,uint8_t size)
{
	uint8_t a[10]={0},i=0;
	while(num!=0)
	{
		a[i]=num%10;
		num/=10;
		i++;
	}
	while(i--)
	{
		OLED_ShowChar(x,y,'0'+a[i],size);
		x+=8;
	}
}

/*
显示单个文字函数：文字大小是16*16
x,y:坐标
pos:第几个数组
count:数组第几个文字
*/
void OLED_WriteChinese(uint8_t x,uint8_t y,uint8_t pos,uint8_t count)
{
	uint8_t i;
	if(x>112)
	{
		x=0;
		y+=2;
	}
	OLED_SetCursor(x,y);
	//要分成2页来写，一页写16个像素
	for(i=0;i<16;++i)
	{
		OLED_WriteData(F16x16[pos][(count-1)*32+i]);//(count-1)*32是第几个汉字的偏移量
	}
	OLED_SetCursor(x,++y);
	for(i=0;i<16;++i)
	{	
		OLED_WriteData(F16x16[pos][(count-1)*32+i+16]);
	}
}

/*
显示多个文字函数：文字大小是16*16，这里是显示一个数组里的多个汉字
x,y:坐标
pos:第几个数组
length:要显示几个文字
*/
void OLED_Write_N_Chinese(uint8_t x,uint8_t y,uint8_t pos,uint8_t length)
{
	if(x>112)
	{
		x=0;
		y+=2;
	}
	OLED_SetCursor(x,y);
	uint8_t i,j=0;
	while(length--)
	{
		//每个汉字要分成2页来写，一页写16个像素(宽)
		for(i=0;i<16;++i)
		{
			OLED_WriteData(F16x16[pos][j*32+i]);//(count-1)*32是第几个汉字的偏移量
		}
		OLED_SetCursor(x,++y);
		for(i=0;i<16;++i)
		{
			OLED_WriteData(F16x16[pos][j*32+i+16]);
		}
		x+=16;//下一个列位置
		//用于在显示汉字的下半部分后，恢复到上半部分的行位置
		OLED_SetCursor(x,--y);
		j++;//下一个汉字
	}
}

//显示BMP图片函数
//(x0,y0):图片显示的开始地址
//(x1,y1):图片显示的结束地址
//BMP:要显示的图片的首地址，具体看fontstr.h文件，这里的BMP图片的大小是128*64像素
//num0：关锁 num1：开锁
void OLED_ShowBMP(uint8_t x0,uint8_t y0, uint8_t x1,uint8_t y1,const uint8_t* BMP)
{
		uint8_t i,j;
		for(j=y0;j<y1;j++)//一行一行显示
		{
			 OLED_SetCursor(x0,j);//将光标定位到当前行
			
			 for(i=x0;i<x1;i++)//在当前行显示每一列
			{
				 OLED_WriteData(BMP[(x1-x0)*j+i]);//当前行索引j * 每行占了几个像素（x1-x0）+ 当前是几列 
			}
		}
}




