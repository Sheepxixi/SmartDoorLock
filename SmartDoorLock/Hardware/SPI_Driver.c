#include "stm32f10x.h"                  // Device header
/*
spiʹ�õ�����ӿڣ�
SDA-->	PA4(CS����)
SCK-->	PA5
MISO-->	PA6
MOSI-->	PA7
*/

//���ݴ����SPI���裨SPI1��SPI2��ʹ����Ӧ��ʱ��
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

// ���ݴ����SPI���裨SPI1��SPI2��������ָ��SPI��IO���ţ�����SCK��MISO��MOSI��Ƭѡ����
static void SPI_GPIO_Config(SPI_TypeDef* SPIx)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	if (SPIx == SPI1)
	{
        // ���� SCK (PA5) �� MOSI (PA7) Ϊ�����������ģʽ
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; 
        GPIO_Init(GPIOA, &GPIO_InitStruct);

        // ���� MISO (PA6) Ϊ��������ģʽ
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &GPIO_InitStruct);

        // ��ʼ��Ƭѡ (CS) ����Ϊ�������ģʽ
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(GPIOA, &GPIO_InitStruct);
        GPIO_SetBits(GPIOA, GPIO_Pin_4);//��ʼ��Ƭѡ����Ϊ�ߵ�ƽ
    } 
	else
	{
        // ���� SCK (PB13) �� MOSI (PB15) Ϊ�����������ģʽ
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; 
        GPIO_Init(GPIOB, &GPIO_InitStruct);

        // ���� MISO (PB14) Ϊ���ø�������ģʽ
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_14;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOB, &GPIO_InitStruct);

        // ��ʼ��Ƭѡ (SDA) ����Ϊ�������ģʽ
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(GPIOB, &GPIO_InitStruct);
        GPIO_SetBits(GPIOB, GPIO_Pin_12);
    } 
}

// ����ָ��SPI�������ز���
void SPI_Config(SPI_TypeDef* SPIx)
{
	SPI_InitTypeDef SPI_InitStructure;  // SPI���ýṹ��

	SPI_ClockCmd(SPIx);  // ʹ��SPI�����GPIO��ʱ��

	// ����SPI����
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  // ���ò�����Ԥ��ƵΪ8
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  // ����Ϊȫ˫��ģʽ
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;  // ����Ϊ����ģʽ
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;  // ��������֡����Ϊ8λ
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;  // ����ʱ�Ӽ���Ϊ��
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;  // ����ʱ����λΪ��1�����ز���
	SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;  // Ӳ������NSS����
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;  // ���ݴ����λ����
	SPI_InitStructure.SPI_CRCPolynomial = 7;  // ����CRC����ʽ
	SPI_Init(SPIx, &SPI_InitStructure);  // ��ʼ��SPI����
	
	SPI_GPIO_Config(SPIx);  // ����SPI��ص�GPIO����

	SPI_SSOutputCmd(SPIx, ENABLE);  // ʹ��SS���
	SPI_Cmd(SPIx, ENABLE);  // ʹ��SPI����
}

/**
  * @brief  д1�ֽ����ݵ�SPI����
  * @param  SPIx ʹ�õ�SPI
  * @param  TxData д�����ߵ�����
  * @retval ���ݷ���״̬
  *		@arg 0 ���ݷ��ͳɹ�
  * 	@arg -1 ���ݷ���ʧ��
  */
int32_t SPI_WriteByte(SPI_TypeDef* SPIx, uint8_t TxData)
{
	uint16_t retry=0;				 
	while((SPIx->SR&SPI_I2S_FLAG_TXE)==0)  //�ȴ���������	SPI_SR��״̬�Ĵ���
	{
		retry++;
		if(retry>20000)return -1;
	}			  
	SPIx->DR=TxData;	 	  				//����һ��byte  SPI_DR��״̬�Ĵ���
	retry=0;
	while((SPIx->SR&SPI_I2S_FLAG_RXNE)==0)				//�ȴ�������һ��byte  
	{
		retry++;
		if(retry>20000)return -1;
	}  
	SPIx->DR;	//���RXNE��־λ					    
	return 0;          				//�����յ�������
}

/**
  * @brief  ��SPI���߶�ȡ1�ֽ�����
  * @param  SPIx ��Ҫʹ�õ�SPI
  * @param  p_RxData ���ݴ����ַ
  * @retval ���ݶ�ȡ״̬
  *		@arg 0 ���ݶ�ȡ�ɹ�
  * 	@arg -1 ���ݶ�ȡʧ��
  */
int32_t SPI_ReadByte(SPI_TypeDef* SPIx, uint8_t *p_RxData)
{
	uint8_t retry=0;				 
	while((SPIx->SR&SPI_I2S_FLAG_TXE)==0);				//�ȴ���������	
	{
		retry++;
		if(retry>200)return -1;
	}			  
	SPIx->DR=0xFF;	 	  				//����һ��byte(���뷢��һ���ֽڲ��ܽ���)
	retry=0;
	while((SPIx->SR&SPI_I2S_FLAG_RXNE)==0); //�ȴ�������һ��byte  
	{
		retry++;
		if(retry>200)return -1;
	}
	*p_RxData = SPIx->DR;  						    
	return 0;          				//�����յ�������
}

/**
  * @brief  ��SPI����д���ֽ�����
  * @param  SPIx ��Ҫʹ�õ�SPI
  * @param  p_TxData �������ݻ������׵�ַ
  * @param	sendDataNum ���������ֽ���
  * @retval ���ݷ���״̬
  *		@arg 0 ���ݷ��ͳɹ�
  * 	@arg -1 ���ݷ���ʧ��
  */
int32_t SPI_WriteNBytes(SPI_TypeDef* SPIx, uint8_t *p_TxData,uint32_t sendDataNum)
{
	uint8_t retry=0;
	while(sendDataNum--){
		while((SPIx->SR&SPI_I2S_FLAG_TXE)==0);				//�ȴ���������	
		{
			retry++;
			if(retry>20000)return -1;
		}			  
		SPIx->DR=*p_TxData++;	 	  				//����һ��byte 
		retry=0;
		while((SPIx->SR&SPI_I2S_FLAG_RXNE)==0); 				//�ȴ�������һ��byte  
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
  * @brief  ��SPI���߶�ȡ���ֽ�����
  * @param  SPIx ��Ҫʹ�õ�SPI
  * @param  p_RxData ���ݴ����ַ
  * @param	readDataNum ��ȡ�����ֽ���
  * @retval ���ݶ�ȡ״̬
  *		@arg 0 ���ݶ�ȡ�ɹ�
  * 	@arg -1 ���ݶ�ȡʧ��
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
		*p_RxData++ = SPIx->DR; //SPIx->DR��������ݣ����ǵ�ַ����
	}	
	return 0;
}
