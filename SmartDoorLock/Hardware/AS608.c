#include "stm32f10x.h"                  // Device header
#include "String.h"
#include "Delay.h"
#include "AS608.h"
#include "usart2.h"
//AS608指纹模块引脚：
//	TX-->	PA3
//	RX-->	PA2
//	WAK-->	PA1

// 默认指纹模块的芯片地址
u32 AS608Addr = 0XFFFFFFFF; 

//配置与WAK相连的PA1引脚：
//WAK：检测到有指纹，输出高电平，所以PA1要配置成下拉输入
void AS608_IO_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	//配置PA1引脚
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
}

//static修饰的函数，只能在本文件内使用

//串口发送一个字节
static void MYUSART_SendData(u8 data)
{
	//发送寄存器为空时，才可以发送字节
	while((USART2->SR&0X40)==0); //状态寄存器SR的第6位TXE
	USART2->DR = data;
//	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==0);
//	USART_SendData(USART2,data);
}

//发送包头：2个字节，0xEF01
static void SendHead(void)
{
	MYUSART_SendData(0xEF);
	MYUSART_SendData(0x01);
}

//发送芯片地址：有4个字节
static void SendAddr(void)
{
	//4个字节要分成4次发
	//发送最高8位，将最高8位右移到低8位
	MYUSART_SendData(AS608Addr>>24);
	MYUSART_SendData(AS608Addr>>16);
	MYUSART_SendData(AS608Addr>>8);
	MYUSART_SendData(AS608Addr);
}

//发送包标识：有1个字节，命令包是0x01；数据包是0x02；结束包是0x08
static void SendFlag(u8 flag)
{
	MYUSART_SendData(flag);
}

//发送包长度：有2个字节
static void SendLength(int length)
{
	MYUSART_SendData(length>>8);
	MYUSART_SendData(length);
}

//发送指令码：1个字节
static void Sendcmd(u8 cmd)
{
	MYUSART_SendData(cmd);
}

//发送校验和:2个字节
static void SendCheck(u16 check)
{
	MYUSART_SendData(check>>8);
	MYUSART_SendData(check);
}

/*
判断接收缓冲区的数组中有没有包含应答包
参数	：WaitTime：为等待中断接收数据的时间（单位 1ms）
返回值	：数据包首地址
*/
static u8* JudgeStr(u16 WaitTime)
{
	char* ask_p;//ask_p指针：匹配到的字符串的首地址
	
	//拼接出预期的应答包格式
	u8 ask[8];
	//包头：2字节，0xef01
	ask[0]=0xef;
	ask[1]=0x01;
	//芯片地址：4字节
	ask[2]=AS608Addr>>24;
	ask[3]=AS608Addr>>16;
	ask[4]=AS608Addr>>8;
	ask[5]=AS608Addr;
	//包标识
	ask[6]=0x07;
	
	//再往后就是：包长度、确认码等等，这些不同应答包都不一样，
	//所以不能再写下去，匹配前面一样的格式就行了
	
	//字符串结束符（在此处并不发挥作用）
	ask[7] = '\0';   

	//重置接收状态标志，准备开始新的数据接收过程
	//第0-14位：接收到的数据的字节数
	//第15位：为1，接收完成（在一次中断接收完毕后置1，具体去看中断服务函数）
	USART2_RX_STA = 0;
	
	//等待时间走完
	while(--WaitTime)
	{
		//延时1毫秒
		Delay_ms(1);
		
		//USART2_RX_STA第15位变成了1，说明接收到了一批数据且接收完成
//		if( (USART2_RX_STA&(1<<15) )==1)
		if(USART2_RX_STA&0X8000)//接收到一次数据
		{
			//清除接收完毕标志位,串口接收状态
			USART2_RX_STA=0;
			
			//判断接收缓冲区中能不能匹配到应答包
			//strstr(const char* serached,const char* serach)函数：
			//第一个参数serached：被查找的字符串，第二个参数serach：要查找/匹配的字符串
			//如果匹配的了，返回serached里匹配到的子串的首地址；否则返回0
			
			ask_p=strstr((const char*)USART2_RX_BUF,(const char*)ask);
			
			if(ask_p)
			{
				return (u8*)ask_p;
			}
		}
	}
	return 0;
}

/*
指纹模块相关函数的总体流程总结，包括指令发送和应答处理的步骤：
1.初始化发送：
	SendHead()：发送包头（0xEF01）。
	SendAddr()：发送芯片地址（4字节）。
	SendFlag(0x01)：发送包标识（通常为 0x01）。
	SendLength(X)：发送包长度，指定后续数据的字节数（包括校验和）。
	Sendcmd(X)：发送具体的指令码（如 0x01, 0x02, 等）。
2.发送数据：
	根据函数的需要，通过 MYUSART_SendData() 发送额外的数据参数（如 BufferID、PageID 等）。
3.计算校验和：
	计算从包标识到校验和之前所有字节的和，使用 SendCheck(CheckSum) 发送校验和以确保数据完整性。
4.等待应答：
	调用 JudgeStr(2000) 等待应答包，最多等待 2000 毫秒，返回应答数据的指针。
5.处理应答：
	检查 ask 指针是否有效：
	如果有效，读取确认码和相应的参数（如指纹容量、安全等级、设备地址等）。
	如果无效，设置确认码为错误标志（如 0xff）。
6.返回结果：
	返回确认码，以指示操作的结果（成功或错误）。
*/

/*
录入图像 PS_GetImage
功能:探测手指，探测到后录入指纹图像存于ImageBuffer
输入参数：无
返回参数：确认字 ：=00H表示录入成功
指令代码：01H 
*/
u8 PS_GetImage(void)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x03);//包长度	=包长度之后到校验和(包括校验和)的总字节数：
	Sendcmd(0x01);//指令码，表示请求录入图像的命令
	
	//计算校验和：=从包标识开始(包括包标识)到校验和之间所有字节之和
	u16 CheckSum=0x01+0x03+0x01;//校验和
	SendCheck(CheckSum);
	
	u8* ask = JudgeStr(2000);//指向应答包的首地址的指针
	u8 ensure;//确认码
	//判断有无应答，有应答则读取确认码
	if(ask)//有应答包
	{
		ensure=ask[9];//读取确认码
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
生成特征  PS_GenChar
功能:将ImageBuffer中的原始图像生成指纹特征文件，存于CharBuffer1或CharBuffer2  
输入参数： BufferID(特征缓冲区号) 
返回参数： 确认字，确认码=00H表示生成特征成功
指令代码： 02H 
*/
u8 PS_GenChar(u8 BufferID)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x04);//包长度
	Sendcmd(0x02);//指令码
	
	MYUSART_SendData(BufferID);//发送数据：BufferID(特征缓冲区号)
	
	u16 CheckSum=0x01+0x04+0x02+BufferID;//校验和
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
精确比对两枚指纹特征 PS_Match
功能: 精确比对CharBuffer1与CharBuffer2中的特征文件 。 
输入参数： 无 
返回参数： 确认字；比对得分
指令代码： 03H 
*/
u8 PS_Match(void)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x03);//包长度
	Sendcmd(0x03);//指令码
	
	u16 CheckSum=0x01+0x03+0x03;//校验和
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
搜索指纹  PS_Search
功能：以CharBuffer1或CharBuffer2中的特征文件搜索整个或部分指纹库。若搜索到，则返回页码 
输入参数： BufferID，StartPage(起始页)，PageNum（页数）  
返回参数： 确认字；页码（相配指纹模板）
指令代码： 04H 
*/
u8 PS_Search(u8 BufferID,u16 StartPage,u16 PageNum,SearchResult *p)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x08);//包长度
	Sendcmd(0x04);//指令码
	
	//发送数据
	MYUSART_SendData(BufferID);
	MYUSART_SendData(StartPage>>8);
	MYUSART_SendData(StartPage);
	MYUSART_SendData(PageNum>>8);
	MYUSART_SendData(PageNum);
	
	u16 CheckSum=0x01+0x08+0x04+BufferID
				+(StartPage>>8)+(u8)StartPage
				+(PageNum>>8)+(u8)PageNum;//校验和
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];//接收确认码
		p->pageID=(ask[10]<<8)+ask[11];//页码在应答包第11、12个字节(一共2个字节)
		p->mathscore=(ask[12]<<8)+ask[13];//对比得分在12、13个字节
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
合并特征（生成模板） PS_RegModel
功能: 将CharBuffer1与CharBuffer2中的特征文件合并生成 
模板，结果存于CharBuffer1与CharBuffer2。 
输入参数： 无 
返回参数： 确认字
指令代码： 05H 
*/
u8 PS_RegModel(void)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x03);//包长度
	Sendcmd(0x05);//指令码
	
	u16 CheckSum=0x01+0x03+0x05;//校验和
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
储存模板  PS_StoreChar
功能: 将CharBuffer1或CharBuffer2中的模板文件存到PageID号
      的flash数据库位置。 
输入参数： BufferID(缓冲区号)，PageID（指纹库位置号） 
返回参数： 确认字
指令代码： 06H 
*/
u8 PS_StoreChar(u8 BufferID,u16 PageID)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x06);//包长度
	Sendcmd(0x06);//指令码
	
	MYUSART_SendData(BufferID);
	MYUSART_SendData(PageID>>8);
	MYUSART_SendData(PageID);
	
	u16 CheckSum=0x01+0x06+0x06+BufferID+(PageID>>8)+(u8)PageID;//校验和
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
(12) 删除模板 PS_DeletChar 
功能    ： 删除flash数据库中指定ID号开始的N个指纹模板
输入参数： PageID(指纹库模板号)，N：删除的模板个数 
返回参数： 确认字
指令代码： 0cH 
*/
u8 PS_DeletChar(u16 PageID,u16 N)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x07);//包长度
	Sendcmd(0x0c);//指令码
	
	MYUSART_SendData(PageID>>8);
	MYUSART_SendData(PageID);
	MYUSART_SendData(N>>8);
	MYUSART_SendData(N);
	
	u16 CheckSum=0x01+0x07+0x0c
				 +(PageID>>8)+(u8)PageID
				 +(N>>8)+(u8)N;//校验和
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
(13) 清空指纹库  PS_Empty
功能    ： 删除flash数据库中所有指纹模板
输入参数： 无 
返回参数： 确认字
指令代码： 0dH 
*/
u8 PS_Empty(void)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x03);//包长度
	Sendcmd(0x0d);//指令码
	
	
	
	u16 CheckSum=0x01+0x03+0x0d;//校验和
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
(14) 写系统寄存器 PS_WriteReg
功能    ： 写模块寄存器
输入参数： 寄存器序号 ；要写入该寄存器的内容
返回参数： 确认字
指令代码： 0eH 
*/
u8 PS_WriteReg(u8 RegNum,u8 DATA)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x05);//包长度
	Sendcmd(0x0e);//指令码
	
	MYUSART_SendData(RegNum);
	MYUSART_SendData(DATA);
	
	u16 CheckSum=0x01+0x05+0x0e+ RegNum+DATA;//校验和
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0)
		printf("\r\n设置参数成功！");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(15) 读系统基本参数 PS_ReadSysPara
功能    ： 读模块寄存器
输入参数： 无
返回参数： 确认字 + 基本参数（16bytes）
指令代码： 0fH 
*/
u8 PS_ReadSysPara(SysPara *p)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x03);//包长度
	Sendcmd(0x0f);//指令码
	
	u16 CheckSum=0x01+0x03+0x0f;//校验和
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
		p->PS_max=(ask[14]<<8)+ask[15];//指纹最大容量：2字节
		p->PS_level=ask[17];//安全等级：2字节，这里只用到了低字节
		p->PS_addr=(ask[18]<<24)+(ask[19]<<16)+(ask[20]<<8)+ask[21];//设备地址，4字节
		p->PS_size=ask[23];//数据包大小：2字节，也是只用到低字节
		p->PS_N=ask[25];//波特率设置：2字节；(波特率为9600*N bps) 
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0)
	{
		printf("\r\n模块最大指纹容量=%d",p->PS_max);
		printf("\r\n对比等级=%d",p->PS_level);
		printf("\r\n地址=%x",p->PS_addr);
		printf("\r\n波特率=%d",p->PS_N*9600);
	}
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(21) 设置芯片地址 PS_SetChipAddr
功能    ： 设置芯片地址
输入参数： PS_addr：设备地址
返回参数： 确认字
指令代码： 0fH 
*/
u8 PS_SetChipAddr(u32 PS_addr)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x07);//包长度
	Sendcmd(0x15);//指令码
	
	MYUSART_SendData(PS_addr>>24);
	MYUSART_SendData(PS_addr>>16);
	MYUSART_SendData(PS_addr>>8);
	MYUSART_SendData(PS_addr);
	
	u16 CheckSum=0x01+0x07+0x15
				+(PS_addr>>24)+(PS_addr>>16)
				+(PS_addr>>8)+(u8)PS_addr;//校验和
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0x00)
		printf("\r\n设置地址成功！");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(24) 写记事本  PS_WriteNotepad 
功能:模块内部为用户开辟了256bytes的FLASH空间用于存放用户数据，
该存储空间称为用户记事本，该记事本逻辑上被分成16个页，写记事
本命令用于写入用户的32bytes数据到指定的记事本页
输入参数：  NotePageNum：页码（0-15） ； user content：用户信息（32字节）
返回参数： 确认字
指令代码： 18H 
*/
u8 PS_WriteNotepad(u8 NotePageNum,u8 * Byte32)//Byte32：指针，指向一个32个字节的内容
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x36);//包长度
	Sendcmd(0x18);//指令码
	
	MYUSART_SendData(NotePageNum);//发送flash页码
	u16 CheckSum=0;//校验和
	
	for(uint8_t i=0;i<32;++i)//发送32字节的用户信息
	{
		MYUSART_SendData(Byte32[i]);
		CheckSum+=Byte32[i];
	}
	
	CheckSum=0x01+0x36+0x18+CheckSum+NotePageNum;//校验和	
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0x00)
		printf("\r\n写记事本成功！");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(25) 读记事本  PS_ReadNotepad 
功能    ：读取FLASH用户区的128bytes数据.
输入参数：  NotePageNum：页码（0-15） ； Byte32：接收数据缓冲区（32字节）
返回参数： 确认字
指令代码： 19H 
*/
u8 PS_ReadNotepad(u8 NotePageNum,u8 * Byte32)//Byte32：指针，指向一个32个字节的数组，用于接收读到的flash用户信息
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x04);//包长度
	Sendcmd(0x19);//指令码
	
	MYUSART_SendData(NotePageNum);//发送flash页码
	
	u16 CheckSum=0x01+0x04+0x19+NotePageNum;//校验和	
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
		for(uint8_t i=0;i<32;++i)
		{
			Byte32[i]=ask[10+i];
		}
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0x00)
		printf("\r\n读记事本成功！");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(25) 高速搜索指纹  PS_HighSpeedSearch 
功能    ：以 CharBuffer1或CharBuffer2中的特征文件高速搜索整个或部分指纹库。
		  若搜索到，则返回页码,该指令对于的确存在于指纹库中 ，且登录时质量
		  很好的指纹，会很快给出搜索结果。
输入参数：  NotePageNum：页码（0-15） ； Byte32：接收数据缓冲区（32字节）
返回参数： 确认字；页码（相配指纹模板）；对比得分
指令代码： 1bH 
*/
u8 PS_HighSpeedSearch(u8 BufferID,u16 StartPage,u16 PageNum,SearchResult *p)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x08);//包长度
	Sendcmd(0x1b);//指令码
	
	MYUSART_SendData(BufferID);
	MYUSART_SendData(StartPage>>8);
	MYUSART_SendData(StartPage);
	MYUSART_SendData(PageNum>>8);
	MYUSART_SendData(PageNum);
	
	u16 CheckSum=0x01+0x08+0x1b+BufferID
				+(StartPage>>8)+(u8)StartPage
				+(PageNum>>8)+(u8)PageNum;//校验和	
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
		p->pageID=(ask[10]<<8)+(u8)ask[11];//页码，在flash哪一页
		p->mathscore=(ask[12]<<8)+(u8)ask[13];//对比等级
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0x00)
		printf("\r\n读记事本成功！");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(29) 读有效模板个数  PS_ValidTempleteNum 
功能    ： 读有效模板个数
输入参数： 无
返回参数： 确认字；有效模板个数ValidN
指令代码： 1dH 
*/
u8 PS_ValidTempleteNum(u16 *ValidN)
{
	
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x03);//包长度
	Sendcmd(0x1d);//指令码
	
	u16 CheckSum=0x01+0x03+0x1d;//校验和	
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
		*ValidN=(ask[10]<<8)+ask[11];//两个u8合成u16 比如0x1200 + 0x34 = 0x1234
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0x00)
		printf("\r\n有效指纹个数=%d",(ask[10]<<8)+ask[11]);
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

// 与 AS608 模块握手
// 参数: PS_Addr 地址指针（用于存储模块返回的地址）
// 返回值: 0 表示成功，1 表示失败
u8 PS_HandShake(u32 *PS_Addr)
{
    // 发送包头
    SendHead();
    
    // 发送设备地址
    SendAddr();
    SendFlag(0x01);//包标识
	SendLength(0x03);//包长度
	Sendcmd(0X01);//指令码
    
    // 发送握手命令参数 0x00, 0x00（参数为 0）
	/*
	握手命令中发送的 0x00, 0x00参数是符合 AS608 模块通信协议的要求，它们作为占位符用于完整握手命令包的格式。
	由于握手命令不需要额外的参数，因此发送这两个字节后即可完成命令的发送过程。
	*/
    MYUSART_SendData(0X00);
    MYUSART_SendData(0X00);  
    
    // 延迟 200ms，等待模块返回数据
    Delay_ms(200);
    
    // 检查 USART2_RX_STA 标志位，判断是否接收到数据
    if(USART2_RX_STA & 0X8000) // 接收到数据
    {       
        // 判断接收到的数据是否是模块返回的应答包
        if(
            USART2_RX_BUF[0] == 0XEF &&  // 包头
            USART2_RX_BUF[1] == 0X01 &&  // 包标识符
            USART2_RX_BUF[6] == 0X07     // 应答包标识符
        )
        {
            // 解析返回的地址并存储到 PS_Addr 指针指向的变量中
            *PS_Addr = (USART2_RX_BUF[2] << 24) + 
                       (USART2_RX_BUF[3] << 16) + 
                       (USART2_RX_BUF[4] << 8) + 
                       USART2_RX_BUF[5];  
            // 清除接收标志
            USART2_RX_STA = 0;
            
            // 返回 0，表示握手成功
            return 0;
        }
        // 清除接收标志
        USART2_RX_STA = 0;                 
    }
    
    // 返回 1，表示握手失败
    return 1;       
}

// 根据确认码的值，返回相应的错误信息字符串
// 参数: ensure 确认码
// 返回值: 对应的错误信息字符串
const char *EnsureMessage(u8 ensure) //保证返回的字符串不被修改
{
    const char *p;
    
    //根据确认码的值，返回相应的错误信息
    switch(ensure)
    {
        case  0x00:
            p = "OK"; break; // 操作成功
        case  0x01:
            p = "数据包接收错误"; break; // 接收数据包时出错
        case  0x02:
            p = "传感器上没有手指"; break; // 没有检测到手指
        case  0x03:
            p = "录入指纹图像失败"; break; // 录入指纹图像失败
        case  0x04:
            p = "指纹图像太干、太淡而生不成特征"; break; // 图像过干或过淡
        case  0x05:
            p = "指纹图像太湿、太糊而生不成特征"; break; // 图像过湿或过糊
        case  0x06:
            p = "指纹图像太乱而生不成特征"; break; // 图像过乱
        case  0x07:
            p = "指纹图像正常，但特征点太少（或面积太小）而生不成特征"; break; // 特征点过少或面积过小
        case  0x08:
            p = "指纹不匹配"; break; // 指纹不匹配
        case  0x09:
            p = "没搜索到指纹"; break; // 没有找到匹配的指纹
        case  0x0a:
            p = "特征合并失败"; break; // 合并特征失败
        case  0x0b:
            p = "访问指纹库时地址序号超出指纹库范围"; break; // 地址超出指纹库范围
        case  0x10:
            p = "删除模板失败"; break; // 删除指纹模板失败
        case  0x11:
            p = "清空指纹库失败"; break; // 清空指纹库失败
        case  0x15:
            p = "缓冲区内没有有效原始图而生不成图像"; break; // 没有有效的原始图像
        case  0x18:
            p = "读写 FLASH 出错"; break; // 读写 FLASH 错误
        case  0x19:
            p = "未定义错误"; break; // 未定义错误
        case  0x1a:
            p = "无效寄存器号"; break; // 无效寄存器号
        case  0x1b:
            p = "寄存器设定内容错误"; break; // 寄存器设定内容错误
        case  0x1c:
            p = "记事本页码指定错误"; break; // 记事本页码指定错误
        case  0x1f:
            p = "指纹库满"; break; // 指纹库已满
        case  0x20:
            p = "地址错误"; break; // 地址错误
        default:
            p = "模块返回确认码有误"; break; // 确认码错误
    }
    
    // 返回错误信息字符串
    return p;    
}

/*
⑺ 读出模板  PS_LoadChar
功能: ： 将flash数据库中指定ID号的指纹模板读入到模板缓冲区CharBuffer1或CharBuffer2 
输入参数： BufferID(缓冲区号)，PageID（指纹库模板号） 
返回参数： 确认字
指令代码： 07H 
*/
u8 PS_LoadChar(u8 BufferID,u16 PageID)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x06);//包长度
	Sendcmd(0x07);//指令码
	
	MYUSART_SendData(BufferID);
	MYUSART_SendData(PageID>>8);
	MYUSART_SendData(PageID);
	
	u16 CheckSum=0x01+0x06+0x07+BufferID+(PageID>>8)+(u8)PageID;//校验和
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
⑻ 上传特征或模板  PS_UpChar （这个好像不需要也行）
功能    ： 将特征缓冲区中的特征文件上传给上位机。
输入参数： BufferID(缓冲区号) 
返回参数： 确认字
指令代码： 08H 
*/
u8 PS_UpChar(u8 BufferID,u16 PageID)
{
	SendHead();//包头
	SendAddr();//芯片地址
	SendFlag(0x01);//包标识
	SendLength(0x04);//包长度
	Sendcmd(0x08);//指令码
	
	MYUSART_SendData(BufferID);
	
	u16 CheckSum=0x01+0x04+0x08+BufferID;//校验和
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//应答包首地址的指针
	u8 ensure;//确认码
	//有应答包
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}
