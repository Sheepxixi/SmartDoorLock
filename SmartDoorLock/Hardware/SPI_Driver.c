#include "stm32f10x.h"                  // Device header
/*
spi使用的外设接口：
SDA-->	PA4(CS引脚)
SCK-->	PA5
MISO-->	PA6
MOSI-->	PA7
*/

//根据传入的SPI外设（SPI1或SPI2）使能相应的时钟
static void SPI_ClockCmd(SPI_TypeDef* SPIx)
{
	if(SPIx==SPI1)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_SPI1,ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	}
	else
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	}
}

// 根据传入的SPI外设（SPI1或SPI2），配置指定SPI的IO引脚，包括SCK、MISO、MOSI和片选引脚
static void SPI_GPIO_Config(SPI_TypeDef* SPIx)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	if (SPIx == SPI1)
	{
        // 配置 SCK (PA5) 和 MOSI (PA7) 为复用推挽输出模式
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; 
        GPIO_Init(GPIOA, &GPIO_InitStruct);

        // 配置 MISO (PA6) 为浮空输入模式
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &GPIO_InitStruct);

        // 初始化片选 (CS) 引脚为推挽输出模式
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(GPIOA, &GPIO_InitStruct);
        GPIO_SetBits(GPIOA, GPIO_Pin_4);//初始化片选引脚为高电平
    } 
	else
	{
        // 配置 SCK (PB13) 和 MOSI (PB15) 为复用推挽输出模式
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; 
        GPIO_Init(GPIOB, &GPIO_InitStruct);

        // 配置 MISO (PB14) 为复用浮空输入模式
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_14;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOB, &GPIO_InitStruct);

        // 初始化片选 (SDA) 引脚为推挽输出模式
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(GPIOB, &GPIO_InitStruct);
        GPIO_SetBits(GPIOB, GPIO_Pin_12);
    } 
}

// 配置指定SPI外设的相关参数
void SPI_Config(SPI_TypeDef* SPIx)
{
	SPI_InitTypeDef SPI_InitStructure;  // SPI配置结构体

	SPI_ClockCmd(SPIx);  // 使能SPI和相关GPIO的时钟

	// 配置SPI参数
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  // 设置波特率预分频为8
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  // 设置为全双工模式
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;  // 设置为主机模式
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;  // 设置数据帧长度为8位
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;  // 设置时钟极性为低
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;  // 设置时钟相位为第1个边沿采样
	SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;  // 硬件控制NSS引脚
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;  // 数据从最高位传输
	SPI_InitStructure.SPI_CRCPolynomial = 7;  // 设置CRC多项式
	SPI_Init(SPIx, &SPI_InitStructure);  // 初始化SPI外设
	
	SPI_GPIO_Config(SPIx);  // 配置SPI相关的GPIO引脚

	SPI_SSOutputCmd(SPIx, ENABLE);  // 使能SS输出
	SPI_Cmd(SPIx, ENABLE);  // 使能SPI外设
}

/**
  * @brief  写1字节数据到SPI总线
  * @param  SPIx 使用的SPI
  * @param  TxData 写到总线的数据
  * @retval 数据发送状态
  *		@arg 0 数据发送成功
  * 	@arg -1 数据发送失败
  */
int32_t SPI_WriteByte(SPI_TypeDef* SPIx, uint8_t TxData)
{
	uint16_t retry=0;				 
	while((SPIx->SR&SPI_I2S_FLAG_TXE)==0)  //等待发送区空	SPI_SR是状态寄存器
	{
		retry++;
		if(retry>20000)return -1;
	}			  
	SPIx->DR=TxData;	 	  				//发送一个byte  SPI_DR是状态寄存器
	retry=0;
	while((SPIx->SR&SPI_I2S_FLAG_RXNE)==0)				//等待接收完一个byte  
	{
		retry++;
		if(retry>20000)return -1;
	}  
	SPIx->DR;	//清空RXNE标志位					    
	return 0;          				//返回收到的数据
}

/**
  * @brief  从SPI总线读取1字节数据
  * @param  SPIx 需要使用的SPI
  * @param  p_RxData 数据储存地址
  * @retval 数据读取状态
  *		@arg 0 数据读取成功
  * 	@arg -1 数据读取失败
  */
int32_t SPI_ReadByte(SPI_TypeDef* SPIx, uint8_t *p_RxData)
{
	uint8_t retry=0;				 
	while((SPIx->SR&SPI_I2S_FLAG_TXE)==0);				//等待发送区空	
	{
		retry++;
		if(retry>200)return -1;
	}			  
	SPIx->DR=0xFF;	 	  				//发送一个byte(必须发送一个字节才能接收)
	retry=0;
	while((SPIx->SR&SPI_I2S_FLAG_RXNE)==0); //等待接收完一个byte  
	{
		retry++;
		if(retry>200)return -1;
	}
	*p_RxData = SPIx->DR;  						    
	return 0;          				//返回收到的数据
}

/**
  * @brief  向SPI总线写多字节数据
  * @param  SPIx 需要使用的SPI
  * @param  p_TxData 发送数据缓冲区首地址
  * @param	sendDataNum 发送数据字节数
  * @retval 数据发送状态
  *		@arg 0 数据发送成功
  * 	@arg -1 数据发送失败
  */
int32_t SPI_WriteNBytes(SPI_TypeDef* SPIx, uint8_t *p_TxData,uint32_t sendDataNum)
{
	uint8_t retry=0;
	while(sendDataNum--){
		while((SPIx->SR&SPI_I2S_FLAG_TXE)==0);				//等待发送区空	
		{
			retry++;
			if(retry>20000)return -1;
		}			  
		SPIx->DR=*p_TxData++;	 	  				//发送一个byte 
		retry=0;
		while((SPIx->SR&SPI_I2S_FLAG_RXNE)==0); 				//等待接收完一个byte  
		{
			SPIx->SR = SPIx->SR;
			retry++;
			if(retry>20000)return -1;
		} 
		SPIx->DR;
	}
	return 0;
}

/**
  * @brief  从SPI总线读取多字节数据
  * @param  SPIx 需要使用的SPI
  * @param  p_RxData 数据储存地址
  * @param	readDataNum 读取数据字节数
  * @retval 数据读取状态
  *		@arg 0 数据读取成功
  * 	@arg -1 数据读取失败
  */
int32_t SPI_ReadNBytes(SPI_TypeDef* SPIx, uint8_t *p_RxData,uint32_t readDataNum)
{
	uint16_t retry=0;
	while(readDataNum--)
	{
		SPIx->DR = 0xFF;
		while(!(SPIx->SR&SPI_I2S_FLAG_TXE))
		{
			retry++;
			if(retry>20000)return -1;
		}
		retry = 0;
		while(!(SPIx->SR&SPI_I2S_FLAG_RXNE))
		{
			retry++;
			if(retry>20000)return -1;
		}
		*p_RxData++ = SPIx->DR; //SPIx->DR会更新数据，但是地址不变
	}	
	return 0;
}
