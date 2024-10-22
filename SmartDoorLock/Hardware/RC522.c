#include "stm32f10x.h"                  // Device header
#include "RC522.h"
#include "Delay.h"
#include "spi_driver.h"
#include "stdio.h"
#include "string.h"
/*
CS：	PA4
SCK：	PA5
MISO：	PA6
MOSI：	PA7
RST：	PA11
*/

void RC522_IO_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);   //开启AFIO时钟
	// 关闭JTAG，因为需要使用PB3和PB4引脚
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);
	
	//配置RST引脚：设置为推挽输出模式
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11| GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;       
    GPIO_Init(GPIOA, &GPIO_InitStruct); 

	//配置SPI引脚
	SPI_Config(SPI1);
}

/*
功能：	写RC522寄存器
参数：	Address[IN]：寄存器地址
		value[IN]：	 写入的值
*/
void WriteRawRC(uint8_t Address, uint8_t value)
{
	//找出真正的设备地址
	uint8_t ucAddr;
	//Bit1-Bit6：表示真正的寄存器地址
	ucAddr=((Address<<1)&0x7E);//0x7E=0111 1110 
	//左移一位是因为在RC522的SPI协议中最低位（Bit0）用于指示操作类型（读或写），而高位用于地址。
	
	//发送数据
	uint8_t write_buffer[2]={0};
	write_buffer[0]=ucAddr;//地址
	write_buffer[1]=value;//值
	
	Delay_ms(1);
	RC522_ENABLE;
	SPI_WriteNBytes(SPI1,write_buffer,2);
	RC522_DISABLE;
}

/*
功能：	读RC522寄存器
参数：	Address[IN]：  寄存器地址
		ucResult[OUT]：读到的值
*/
uint8_t ReadRawRC(uint8_t Address)
{
	//找出真正的设备地址
	uint8_t ucAddr;
	//Bit1-Bit6：表示真正的寄存器地址,Bit7=1表示读操作，Bit7=0表示写操作
	ucAddr=((Address<<1)&0x7E)|0x80;//0x7E=0111 1110
							//在SPI通信中，Bit7被用作读写标志位。设置Bit7为1表示这是一个读操作。
	//读数据
	uint8_t ucResult=0;
	Delay_ms(1);
	RC522_ENABLE;
	SPI_WriteByte(SPI1,ucAddr);//发送地址
	SPI_ReadByte(SPI1,&ucResult);
	RC522_DISABLE;
	
	return ucResult;
}

/*
功能：	置RC522寄存器的某个字节的某一位
参数：	reg[IN]：  寄存器地址
		mask[IN]： 置位值（要置哪一位为1），如0x80就是置第8位为1
*/
void SetBitMask(unsigned char reg,unsigned char mask) 
{
	//先把寄存器当前的值读出来
	uint8_t temp;
	temp=ReadRawRC(reg);
	//通过mask置某一位为1
	WriteRawRC(reg,temp|mask);
}

/*
功能：	复位RC522寄存器的某个字节的某一位
参数：	reg[IN]：  寄存器地址
		mask[IN]： 置位值（要置哪一位为0），如0x80就是置第8位为0
*/
void ClearBitMask(unsigned char reg,unsigned char mask) 
{
	//先把寄存器当前的值读出来
	uint8_t temp;
	temp=ReadRawRC(reg);
	//通过mask置某一位为1
	WriteRawRC(reg,temp&~mask);
}

/*
功能：	单片机MCU通过RC522和ISO14443卡通讯
参数：	Command[IN]		:	RC522命令字
		pIn [IN]		:	通过RC522发送到卡片的数据
		InLenByte[IN]	:	发送数据的字节长度
		pOut [OUT]		:	接收到的卡片返回数据
		*pOutLenBit[OUT]:	返回数据的位长度
返回：	如果返回MI_OK，表示通信成功
*/
char PcdComMF522(unsigned char Command, unsigned char *pInData, unsigned char InLenByte,unsigned char *pOutData, unsigned int  *pOutLenBit)
{
	
	// 初始化状态为错误
	char status=MI_ERR;
	unsigned char irqEn   = 0x00;  // 中断启用寄存器值
    unsigned char waitFor = 0x00;  // 等待标志位
    unsigned char lastBits;        // 最后接收到的位数
    unsigned char n;               // 接收的字节数
    unsigned int i;                // 计数器，用于超时控制
	
	// 根据命令类型设置中断启用和等待标志
    switch (Command)
    {
       case PCD_AUTHENT:  // 验证密钥命令
          irqEn   = 0x12;   // 允许终止中断和错误中断
          waitFor = 0x10;   // 等待终止中断标志位
          break;
       case PCD_TRANSCEIVE:  // 发送和接收命令
          irqEn   = 0x77;   // 允许所有中断
          waitFor = 0x30;   // 等待发送和接收完成
          break;
       default:
         break;
    }
   
    // 启用中断请求和清除可能的中断标志
    WriteRawRC(ComIEnReg, irqEn | 0x80);  // 允许RC522的中断请求
    ClearBitMask(ComIrqReg, 0x80);        // 清除RC522中断标志
    WriteRawRC(CommandReg, PCD_IDLE);     // 使RC522处于空闲模式
    SetBitMask(FIFOLevelReg, 0x80);       // 清空FIFO缓冲区

    // 向FIFO缓冲区写入要发送的数据
    for (i = 0; i < InLenByte; i++)
    {   
        WriteRawRC(FIFODataReg, pInData[i]);  // 将数据写入FIFO
    }
    WriteRawRC(CommandReg, Command);  // 执行指定的命令
    
    // 如果是发送和接收命令，则启动传输
    if (Command == PCD_TRANSCEIVE)
    {    
        SetBitMask(BitFramingReg, 0x80);  // 启动传输
    }
    
    // 等待RC522执行完成或者超时
    i = 800; // 设定超时时间为800次循环，约25ms
    do 
    {
         n = ReadRawRC(ComIrqReg);  // 读取中断状态寄存器
         i--;  // 递减计数器，检测是否超时
    }
    while ((i != 0) && !(n & 0x01) && !(n & waitFor));  // 循环直到超时或检测到所需中断

    // 清除传输启动位
    ClearBitMask(BitFramingReg, 0x80);
	      
    // 检查是否超时
    if (i != 0)
    {    
         // 检查是否发生了任何错误
         if (!(ReadRawRC(ErrorReg) & 0x1B))
         {
             status = MI_OK;  // 没有错误，通信成功
             if (n & irqEn & 0x01)
             {   
                 status = MI_NOTAGERR;  // 无卡错误，未检测到卡片
             }
             if (Command == PCD_TRANSCEIVE)
             {
                n = ReadRawRC(FIFOLevelReg);  // 读取FIFO中的字节数
                lastBits = ReadRawRC(ControlReg) & 0x07;  // 读取最后接收到的位数
                if (lastBits)
                {   
                    *pOutLenBit = (n - 1) * 8 + lastBits;  // 计算接收到的位数
                }
                else
                {   
                    *pOutLenBit = n * 8;  // 没有剩余的位，直接按字节计算
                }
                if (n == 0)
                {   
                    n = 1;  // 至少有一个字节
                }
                if (n > MAXRLEN)
                {   
                    n = MAXRLEN;  // 限制最大字节数
                }
                for (i = 0; i < n; i++)
                {   
                    pOutData[i] = ReadRawRC(FIFODataReg);  // 从FIFO读取接收到的数据
                }
            }
         }
         else
         {   
             status = MI_ERR;  // 通信过程中出现错误
         }
   }
   
   // 停止计时器，并使RC522处于空闲模式
   SetBitMask(ControlReg, 0x80);  // 停止RC522的计时器
   WriteRawRC(CommandReg, PCD_IDLE);  // 使RC522进入空闲模式
   
   return status;  // 返回通信状态
}

/////////////////////////////////////////////////////////////////////
//开启天线  
//每次启动或关闭天险发射之间应至少有1ms的间隔
/////////////////////////////////////////////////////////////////////
void PcdAntennaOn(void)
{
    unsigned char i;
    i = ReadRawRC(TxControlReg);
    if (!(i & 0x03))
    {
        SetBitMask(TxControlReg, 0x03);//Bit0 和 Bit1：通常用于控制天线的发射功率级别。
									   //在 RC522 的数据手册中，这两位决定了天线的发射功率
    }
}

/////////////////////////////////////////////////////////////////////
//关闭天线
/////////////////////////////////////////////////////////////////////
void PcdAntennaOff(void)
{
    ClearBitMask(TxControlReg, 0x03);
}

/////////////////////////////////////////////////////////////////////
//功    能：复位RC522
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdReset(void)
{
    RC522_RESET_SET();     //RST522_1;确保 RC522 未被复位。
    Delay_us(10);  //_NOP();
    RC522_RESET_RESET();   //RST522_0;触发 RC522 进入复位状态。
    Delay_ms(60);  //_NOP();_NOP();
    RC522_RESET_SET();     //RST522_1;结束复位，RC522 重新启动。
    Delay_us(500);  //_NOP();_NOP();
    WriteRawRC(CommandReg,PCD_RESETPHASE);//发送复位命令
    Delay_ms(2);  //_NOP();_NOP();
  
	WriteRawRC(ModeReg, 0x3D);            // 配置MIFARE模式和CRC
	WriteRawRC(TReloadRegL, 30);         // 设定定时器重载值低字节
	WriteRawRC(TReloadRegH, 0);          // 设定定时器重载值高字节
	WriteRawRC(TModeReg, 0x8D);          // 设定定时器模式
	WriteRawRC(TPrescalerReg, 0x3E);     // 设定定时器预分频器
	WriteRawRC(TxAutoReg, 0x40);         // 自动发送配置

  
	ClearBitMask(TestPinEnReg, 0x80); // 关闭 MX 和 DTRQ 输出
	WriteRawRC(TxAutoReg, 0x40);      // 再次配置自动发送

   
  return MI_OK;
}

void RC522_Init(void)
{
//		Card_Type1[0]=0x04;
//		Card_Type1[1]=0x00;
		RC522_IO_Init();
		PcdReset();  //复位RC522
		PcdAntennaOff();  //关闭天线
		Delay_ms(100);
		PcdAntennaOn();  //开启天线
		//printf("---------------------智能门锁---------------------\r\n");
}

/////////////////////////////////////////////////////////////////////
//用MF522计算CRC16检验值
/////////////////////////////////////////////////////////////////////
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData)
{
    unsigned char i,n;
    ClearBitMask(DivIrqReg,0x04);//中断标志被清除，避免干扰当前操作
    WriteRawRC(CommandReg,PCD_IDLE);//进入空闲状态
    SetBitMask(FIFOLevelReg,0x80);//清空 FIFO 缓冲区
    for (i=0; i<len; i++)
    {   
		WriteRawRC(FIFODataReg, *(pIndata+i)); //将每个字节写入 FIFO  
	}
    WriteRawRC(CommandReg, PCD_CALCCRC);//启动 CRC 计算
    i = 0xFF;
    do 
    {
        n = ReadRawRC(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));//!(n & 0x04)：尚未检测到 CRC 完成中断
    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}

/*
寻卡命令：Transceive命令（0x0c，发送并接收应答命令）+ 要发送的数据（0x26/0x52）
返回值：	 返回卡类型：有2Byte ：(4,0)

功    能：寻卡
参数说明: req_code[IN]:寻卡方式
          值：  0x52 = 寻感应区内所有符合14443A标准的卡
                0x26 = 寻未进入休眠状态的卡
          pTagType[OUT]：卡片类型代码
                0x4400 = Mifare_UltraLight
                0x0400 = Mifare_One(S50)
                0x0200 = Mifare_One(S70)
                0x0800 = Mifare_Pro(X)
                0x4403 = Mifare_DESFire
返    回: 成功返回MI_OK
*/
char PcdRequest(unsigned char req_code,unsigned char *pTagType)
{
	char status;//用于存储操作状态
	unsigned int unlen;//返回的数据的字节数
	unsigned char ucComMF522Buf[MAXRLEN];//缓冲区
	
	//清零Status2Reg的MFAuthent Command（认证命令）执行成功标志位
	ClearBitMask(Status2Reg,0x08);
	//清零Transceive命令开始位
	WriteRawRC(BitFramingReg,0x07);
	//开启天线
	SetBitMask(TxControlReg,0x03);
	// 将寻卡命令代码（0x26 或 0x52）放入通信缓冲区的第一个字节
	ucComMF522Buf[0]=req_code;
	// 调用 PcdComMF522 函数发送命令并接收响应
    // 参数：
    // PCD_TRANSCEIVE：表示发送并接收数据
    // ucComMF522Buf：发送的数据缓冲区
    // 1：发送数据的字节长度
    // ucComMF522Buf：接收数据的缓冲区（复用发送缓冲区）
    // &unlen：接收数据的位长度
	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unlen);
	// 如果通信成功且接收到的数据位长度为16位（即2字节）
	if (status == MI_OK && (unlen == 0x10)) 
	{
		*pTagType     = ucComMF522Buf[0];
		*(pTagType+1) = ucComMF522Buf[1];
//		printf("request:%x %x\r\n",ucComMF522Buf[0],ucComMF522Buf[1]);
	}
	else 	
	{
		status = MI_ERR;
//		printf("PcdRequest Failed\r\n");
	}
	return status;
}

/*
防冲突命令：Transceive命令（0x0c，发送并接收应答命令）+ 要发送的数据（0x93+0x20）
返回值：	 返回4Byte ID号；1Byte 校验（Check）

功    能：防冲撞
参数说明: pSnr[OUT]:卡片序列号，4字节
返    回: 成功返回MI_OK
*/
unsigned char snr_check=0;//校验位
char PcdAnticoll(unsigned char *pSnr)
{
	char status;//用于存储操作状态
	unsigned char temp_check=0;//校验位
	unsigned int unlen;//返回的数据的字节数
	unsigned char ucComMF522Buf[MAXRLEN];//缓冲区
	
	// 清除 Status2Reg 寄存器的（CRC错误标志位），确保上一次操作的错误状态而影响本次操作
    ClearBitMask(Status2Reg, 0x08);
	
	//BitFramingReg 寄存器用于控制位的发送和接收帧的格式,这里使用默认格式
    // 设置 BitFramingReg寄存器为 0，准备发送数据
    WriteRawRC(BitFramingReg, 0x00);
	
	//CollReg 寄存器用于处理防冲突检测和响应
    // 清除 CollReg 寄存器的最高位，以允许冲突检测
    ClearBitMask(CollReg, 0x80);
	
	ucComMF522Buf[0]=PICC_ANTICOLL1;//防冲突
	ucComMF522Buf[1]=0x20;//NVB：有效字节数为2
	
	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unlen);
	
//	printf("Before Anticoll\r\n");
	if(status == MI_OK)
	{
		for(uint8_t i=0;i<4;++i)
		{
			// 将序列号保存到 pSnr 指针所指向的内存中
			*(pSnr+i)=ucComMF522Buf[i];
			// 对序列号的每个字节进行异或运算，用于后续的校验
			temp_check ^= ucComMF522Buf[i];
		}
		snr_check=ucComMF522Buf[4];
		//校验不通过
		if(snr_check!=temp_check)
		{
			status=MI_ERR;
		}
	}
	
	// 置 CollReg 寄存器的最高位，以禁止冲突检测
    SetBitMask(CollReg, 0x80);
	
	return status;
}

/*
选卡命令：Transceive命令（0x0c，发送并接收应答命令）+ 要发送的数据
		（0x93+0x20+4字节ID号+1字节校验Check+1字节CRC）。
返回值	：0x08

功    能：选定卡片
参数说明: pSnr[IN]:卡片序列号，4字节
返    回: 成功返回MI_OK
*/
char PcdSelect(unsigned char *pSnr)
{
	char status;
    unsigned char i;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
	
	
	ucComMF522Buf[0] = PICC_ANTICOLL1;//选卡指令码
    ucComMF522Buf[1] = 0x70;//NVB（Number of Valid Bits）表示数据的有效位数
	ucComMF522Buf[6] = snr_check;//校验Check
	
	//卡的ID号
	for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    }
	//生成CRC校验码：2个字节 对发送缓冲区的前7个字节计算CRC16校验码，并将结果存储在缓冲区的第8和第9个字节
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
	
//	printf("Before Select\r\n");
								//9：发送数据的字节长度（0x93, 0x70, 4字节ID, 校验Check, 2字节CRC，共9字节）
	status=PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
	
	// 检查响应是否正确（状态是否为 MI_OK 且返回的数据长度是否为 0x18）
    if ((status == MI_OK) && (unLen == 0x18))
    {
        status = MI_OK;  // 成功
    }
    else
    {
        status = MI_ERR;  // 失败
    }
    return status;
}

/*
认证（验证密钥）命令：Authent命令（0x0c，发送并接收应答命令）+ 要发送的数据
		（0x60+0x08）。
返回值	：有返回值，但是一般不处理

功    能：验证卡片密码
参数说明: auth_mode[IN]: 密码验证模式
                0x60 = 验证A密钥
                0x61 = 验证B密钥 
         addr[IN]：块地址(要验证哪一块)：取值为0-63
         pKey[IN]：密码
         pSnr[IN]：卡片序列号，4字节
返    回: 成功返回MI_OK
*/
char PcdAuthState(unsigned char auth_mode, unsigned char addr, unsigned char *pKey, unsigned char *pSnr)
{
	char status;
    unsigned int  unLen;
    unsigned char i,ucComMF522Buf[MAXRLEN]; 
	// 初始化命令缓冲区
    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
	
	// 将 6 字节的密码复制到缓冲区中
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+2] = *(pKey+i);   }
	
	// 将 4 字节的卡片序列号复制到缓冲区中
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+8] = *(pSnr+i);   }
    
     // 发送验证命令并接收读卡器的响应
    status = PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &unLen);

    // 检查返回状态和验证结果
    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
    {
        status = MI_ERR;  // 验证失败
    }
    
    return status;  // 返回验证状态
}

/*
Transceive命令（0x0c）+ 要发送的数据（0x30+addr）。

功    能：读取M1卡一块数据,只能读出1整块的内容，共16个字节
参数说明: addr[IN]：块地址
          pData[OUT]：读出的数据，16字节
返    回: 成功返回MI_OK
*/
char PcdRead(unsigned char addr,unsigned char *pData)
{
    char status;
    unsigned int unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN]; 

    // 准备读命令及块地址
    ucComMF522Buf[0] = PICC_READ;  // 读命令 0x30
    ucComMF522Buf[1] = addr;       // 要读取的块地址
    
    // 计算 CRC 校验码
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
    
    // 发送数据包给卡片并接收响应
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
    
    // 检查状态和接收的数据长度是否正确
    if ((status == MI_OK) && (unLen == 0x90)) // 0x90 表示收到 16 字节的数据和 2 字节的 CRC 校验（144bit）
    {
        // 将接收到的 16 字节数据存入 pData
        for (i = 0; i < 16; i++)
        {
            *(pData + i) = ucComMF522Buf[i];
        }
    }
    else
    {
        status = MI_ERR;
    }
    
    return status;
}

/*
Transceive命令（0x0c）+ 要发送的数据（0xA0+addr）

功    能：写数据到M1卡一块
参数说明: addr[IN]：块地址
          pData[IN]：写入的数据，16字节
返    回: 成功返回MI_OK
*/
char PcdWrite(unsigned char addr,unsigned char *pData)
{
    char status;
    unsigned int unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN]; 
    
    // 准备写命令及块地址
    ucComMF522Buf[0] = PICC_WRITE;  // 写命令 0xA0
    ucComMF522Buf[1] = addr;        // 要写入的块地址
    
    // 计算 CRC 校验码，并将结果添加到2~3的索引地址
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
    
    // 发送命令数据包给卡片并接收响应
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

	//((ucComMF522Buf[0] & 0x0F) != 0x0A) 是用来检查卡片在接收写入数据后返回的状态字节，确保写操作成功
    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }
    
    // 如果卡片已准备好接收数据，则发送 16 字节数据
    if (status == MI_OK)
    {
        // 准备要写入的数据
        for (i = 0; i < 16; i++)
        {
            ucComMF522Buf[i] = *(pData + i);
        }

        // 计算数据的 CRC 校验码并附加到末尾
        CalulateCRC(ucComMF522Buf, 16, &ucComMF522Buf[16]);

        // 发送数据和 CRC 校验码
        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &unLen);

        // 检查写入结果
		//((ucComMF522Buf[0] & 0x0F) != 0x0A) 是用来检查卡片在接收写入数据后返回的状态字节，确保写操作成功
        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {
            status = MI_ERR;
        }
    }
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：命令卡片进入休眠状态
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdHalt(void)
{
    unsigned int unLen; // 用于接收数据长度的变量
    unsigned char ucComMF522Buf[MAXRLEN]; // 用于存储发送和接收数据的缓冲区

    ucComMF522Buf[0] = PICC_HALT; // 发送的第一个字节是 PICC_HALT 命令，表示卡片休眠
    ucComMF522Buf[1] = 0; // 第二个字节通常为 0，表示没有额外的数据

    // 计算前两个字节的 CRC 校验码，并将结果存储在 ucComMF522Buf 的后续两个字节中
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
 
    // 发送命令并接收卡片的响应
    PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    return MI_OK; // 固定返回 MI_OK 表示成功
}
