#include "stm32f10x.h"                  // Device header
#include "RC522.h"
#include "Delay.h"
#include "spi_driver.h"
#include "stdio.h"
#include "string.h"
/*
CS��	PA4
SCK��	PA5
MISO��	PA6
MOSI��	PA7
RST��	PA11
*/

void RC522_IO_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);   //����AFIOʱ��
	// �ر�JTAG����Ϊ��Ҫʹ��PB3��PB4����
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);
	
	//����RST���ţ�����Ϊ�������ģʽ
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11| GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;       
    GPIO_Init(GPIOA, &GPIO_InitStruct); 

	//����SPI����
	SPI_Config(SPI1);
}

/*
���ܣ�	дRC522�Ĵ���
������	Address[IN]���Ĵ�����ַ
		value[IN]��	 д���ֵ
*/
void WriteRawRC(uint8_t Address, uint8_t value)
{
	//�ҳ��������豸��ַ
	uint8_t ucAddr;
	//Bit1-Bit6����ʾ�����ļĴ�����ַ
	ucAddr=((Address<<1)&0x7E);//0x7E=0111 1110 
	//����һλ����Ϊ��RC522��SPIЭ�������λ��Bit0������ָʾ�������ͣ�����д��������λ���ڵ�ַ��
	
	//��������
	uint8_t write_buffer[2]={0};
	write_buffer[0]=ucAddr;//��ַ
	write_buffer[1]=value;//ֵ
	
	Delay_ms(1);
	RC522_ENABLE;
	SPI_WriteNBytes(SPI1,write_buffer,2);
	RC522_DISABLE;
}

/*
���ܣ�	��RC522�Ĵ���
������	Address[IN]��  �Ĵ�����ַ
		ucResult[OUT]��������ֵ
*/
uint8_t ReadRawRC(uint8_t Address)
{
	//�ҳ��������豸��ַ
	uint8_t ucAddr;
	//Bit1-Bit6����ʾ�����ļĴ�����ַ,Bit7=1��ʾ��������Bit7=0��ʾд����
	ucAddr=((Address<<1)&0x7E)|0x80;//0x7E=0111 1110
							//��SPIͨ���У�Bit7��������д��־λ������Bit7Ϊ1��ʾ����һ����������
	//������
	uint8_t ucResult=0;
	Delay_ms(1);
	RC522_ENABLE;
	SPI_WriteByte(SPI1,ucAddr);//���͵�ַ
	SPI_ReadByte(SPI1,&ucResult);
	RC522_DISABLE;
	
	return ucResult;
}

/*
���ܣ�	��RC522�Ĵ�����ĳ���ֽڵ�ĳһλ
������	reg[IN]��  �Ĵ�����ַ
		mask[IN]�� ��λֵ��Ҫ����һλΪ1������0x80�����õ�8λΪ1
*/
void SetBitMask(unsigned char reg,unsigned char mask) 
{
	//�ȰѼĴ�����ǰ��ֵ������
	uint8_t temp;
	temp=ReadRawRC(reg);
	//ͨ��mask��ĳһλΪ1
	WriteRawRC(reg,temp|mask);
}

/*
���ܣ�	��λRC522�Ĵ�����ĳ���ֽڵ�ĳһλ
������	reg[IN]��  �Ĵ�����ַ
		mask[IN]�� ��λֵ��Ҫ����һλΪ0������0x80�����õ�8λΪ0
*/
void ClearBitMask(unsigned char reg,unsigned char mask) 
{
	//�ȰѼĴ�����ǰ��ֵ������
	uint8_t temp;
	temp=ReadRawRC(reg);
	//ͨ��mask��ĳһλΪ1
	WriteRawRC(reg,temp&~mask);
}

/*
���ܣ�	��Ƭ��MCUͨ��RC522��ISO14443��ͨѶ
������	Command[IN]		:	RC522������
		pIn [IN]		:	ͨ��RC522���͵���Ƭ������
		InLenByte[IN]	:	�������ݵ��ֽڳ���
		pOut [OUT]		:	���յ��Ŀ�Ƭ��������
		*pOutLenBit[OUT]:	�������ݵ�λ����
���أ�	�������MI_OK����ʾͨ�ųɹ�
*/
char PcdComMF522(unsigned char Command, unsigned char *pInData, unsigned char InLenByte,unsigned char *pOutData, unsigned int  *pOutLenBit)
{
	
	// ��ʼ��״̬Ϊ����
	char status=MI_ERR;
	unsigned char irqEn   = 0x00;  // �ж����üĴ���ֵ
    unsigned char waitFor = 0x00;  // �ȴ���־λ
    unsigned char lastBits;        // �����յ���λ��
    unsigned char n;               // ���յ��ֽ���
    unsigned int i;                // �����������ڳ�ʱ����
	
	// �����������������ж����ú͵ȴ���־
    switch (Command)
    {
       case PCD_AUTHENT:  // ��֤��Կ����
          irqEn   = 0x12;   // ������ֹ�жϺʹ����ж�
          waitFor = 0x10;   // �ȴ���ֹ�жϱ�־λ
          break;
       case PCD_TRANSCEIVE:  // ���ͺͽ�������
          irqEn   = 0x77;   // ���������ж�
          waitFor = 0x30;   // �ȴ����ͺͽ������
          break;
       default:
         break;
    }
   
    // �����ж������������ܵ��жϱ�־
    WriteRawRC(ComIEnReg, irqEn | 0x80);  // ����RC522���ж�����
    ClearBitMask(ComIrqReg, 0x80);        // ���RC522�жϱ�־
    WriteRawRC(CommandReg, PCD_IDLE);     // ʹRC522���ڿ���ģʽ
    SetBitMask(FIFOLevelReg, 0x80);       // ���FIFO������

    // ��FIFO������д��Ҫ���͵�����
    for (i = 0; i < InLenByte; i++)
    {   
        WriteRawRC(FIFODataReg, pInData[i]);  // ������д��FIFO
    }
    WriteRawRC(CommandReg, Command);  // ִ��ָ��������
    
    // ����Ƿ��ͺͽ����������������
    if (Command == PCD_TRANSCEIVE)
    {    
        SetBitMask(BitFramingReg, 0x80);  // ��������
    }
    
    // �ȴ�RC522ִ����ɻ��߳�ʱ
    i = 800; // �趨��ʱʱ��Ϊ800��ѭ����Լ25ms
    do 
    {
         n = ReadRawRC(ComIrqReg);  // ��ȡ�ж�״̬�Ĵ���
         i--;  // �ݼ�������������Ƿ�ʱ
    }
    while ((i != 0) && !(n & 0x01) && !(n & waitFor));  // ѭ��ֱ����ʱ���⵽�����ж�

    // �����������λ
    ClearBitMask(BitFramingReg, 0x80);
	      
    // ����Ƿ�ʱ
    if (i != 0)
    {    
         // ����Ƿ������κδ���
         if (!(ReadRawRC(ErrorReg) & 0x1B))
         {
             status = MI_OK;  // û�д���ͨ�ųɹ�
             if (n & irqEn & 0x01)
             {   
                 status = MI_NOTAGERR;  // �޿�����δ��⵽��Ƭ
             }
             if (Command == PCD_TRANSCEIVE)
             {
                n = ReadRawRC(FIFOLevelReg);  // ��ȡFIFO�е��ֽ���
                lastBits = ReadRawRC(ControlReg) & 0x07;  // ��ȡ�����յ���λ��
                if (lastBits)
                {   
                    *pOutLenBit = (n - 1) * 8 + lastBits;  // ������յ���λ��
                }
                else
                {   
                    *pOutLenBit = n * 8;  // û��ʣ���λ��ֱ�Ӱ��ֽڼ���
                }
                if (n == 0)
                {   
                    n = 1;  // ������һ���ֽ�
                }
                if (n > MAXRLEN)
                {   
                    n = MAXRLEN;  // ��������ֽ���
                }
                for (i = 0; i < n; i++)
                {   
                    pOutData[i] = ReadRawRC(FIFODataReg);  // ��FIFO��ȡ���յ�������
                }
            }
         }
         else
         {   
             status = MI_ERR;  // ͨ�Ź����г��ִ���
         }
   }
   
   // ֹͣ��ʱ������ʹRC522���ڿ���ģʽ
   SetBitMask(ControlReg, 0x80);  // ֹͣRC522�ļ�ʱ��
   WriteRawRC(CommandReg, PCD_IDLE);  // ʹRC522�������ģʽ
   
   return status;  // ����ͨ��״̬
}

/////////////////////////////////////////////////////////////////////
//��������  
//ÿ��������ر����շ���֮��Ӧ������1ms�ļ��
/////////////////////////////////////////////////////////////////////
void PcdAntennaOn(void)
{
    unsigned char i;
    i = ReadRawRC(TxControlReg);
    if (!(i & 0x03))
    {
        SetBitMask(TxControlReg, 0x03);//Bit0 �� Bit1��ͨ�����ڿ������ߵķ��书�ʼ���
									   //�� RC522 �������ֲ��У�����λ���������ߵķ��书��
    }
}

/////////////////////////////////////////////////////////////////////
//�ر�����
/////////////////////////////////////////////////////////////////////
void PcdAntennaOff(void)
{
    ClearBitMask(TxControlReg, 0x03);
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���λRC522
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdReset(void)
{
    RC522_RESET_SET();     //RST522_1;ȷ�� RC522 δ����λ��
    Delay_us(10);  //_NOP();
    RC522_RESET_RESET();   //RST522_0;���� RC522 ���븴λ״̬��
    Delay_ms(60);  //_NOP();_NOP();
    RC522_RESET_SET();     //RST522_1;������λ��RC522 ����������
    Delay_us(500);  //_NOP();_NOP();
    WriteRawRC(CommandReg,PCD_RESETPHASE);//���͸�λ����
    Delay_ms(2);  //_NOP();_NOP();
  
	WriteRawRC(ModeReg, 0x3D);            // ����MIFAREģʽ��CRC
	WriteRawRC(TReloadRegL, 30);         // �趨��ʱ������ֵ���ֽ�
	WriteRawRC(TReloadRegH, 0);          // �趨��ʱ������ֵ���ֽ�
	WriteRawRC(TModeReg, 0x8D);          // �趨��ʱ��ģʽ
	WriteRawRC(TPrescalerReg, 0x3E);     // �趨��ʱ��Ԥ��Ƶ��
	WriteRawRC(TxAutoReg, 0x40);         // �Զ���������

  
	ClearBitMask(TestPinEnReg, 0x80); // �ر� MX �� DTRQ ���
	WriteRawRC(TxAutoReg, 0x40);      // �ٴ������Զ�����

   
  return MI_OK;
}

void RC522_Init(void)
{
//		Card_Type1[0]=0x04;
//		Card_Type1[1]=0x00;
		RC522_IO_Init();
		PcdReset();  //��λRC522
		PcdAntennaOff();  //�ر�����
		Delay_ms(100);
		PcdAntennaOn();  //��������
		//printf("---------------------��������---------------------\r\n");
}

/////////////////////////////////////////////////////////////////////
//��MF522����CRC16����ֵ
/////////////////////////////////////////////////////////////////////
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData)
{
    unsigned char i,n;
    ClearBitMask(DivIrqReg,0x04);//�жϱ�־�������������ŵ�ǰ����
    WriteRawRC(CommandReg,PCD_IDLE);//�������״̬
    SetBitMask(FIFOLevelReg,0x80);//��� FIFO ������
    for (i=0; i<len; i++)
    {   
		WriteRawRC(FIFODataReg, *(pIndata+i)); //��ÿ���ֽ�д�� FIFO  
	}
    WriteRawRC(CommandReg, PCD_CALCCRC);//���� CRC ����
    i = 0xFF;
    do 
    {
        n = ReadRawRC(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));//!(n & 0x04)����δ��⵽ CRC ����ж�
    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}

/*
Ѱ�����Transceive���0x0c�����Ͳ�����Ӧ�����+ Ҫ���͵����ݣ�0x26/0x52��
����ֵ��	 ���ؿ����ͣ���2Byte ��(4,0)

��    �ܣ�Ѱ��
����˵��: req_code[IN]:Ѱ����ʽ
          ֵ��  0x52 = Ѱ��Ӧ�������з���14443A��׼�Ŀ�
                0x26 = Ѱδ��������״̬�Ŀ�
          pTagType[OUT]����Ƭ���ʹ���
                0x4400 = Mifare_UltraLight
                0x0400 = Mifare_One(S50)
                0x0200 = Mifare_One(S70)
                0x0800 = Mifare_Pro(X)
                0x4403 = Mifare_DESFire
��    ��: �ɹ�����MI_OK
*/
char PcdRequest(unsigned char req_code,unsigned char *pTagType)
{
	char status;//���ڴ洢����״̬
	unsigned int unlen;//���ص����ݵ��ֽ���
	unsigned char ucComMF522Buf[MAXRLEN];//������
	
	//����Status2Reg��MFAuthent Command����֤���ִ�гɹ���־λ
	ClearBitMask(Status2Reg,0x08);
	//����Transceive���ʼλ
	WriteRawRC(BitFramingReg,0x07);
	//��������
	SetBitMask(TxControlReg,0x03);
	// ��Ѱ��������루0x26 �� 0x52������ͨ�Ż������ĵ�һ���ֽ�
	ucComMF522Buf[0]=req_code;
	// ���� PcdComMF522 �����������������Ӧ
    // ������
    // PCD_TRANSCEIVE����ʾ���Ͳ���������
    // ucComMF522Buf�����͵����ݻ�����
    // 1���������ݵ��ֽڳ���
    // ucComMF522Buf���������ݵĻ����������÷��ͻ�������
    // &unlen���������ݵ�λ����
	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unlen);
	// ���ͨ�ųɹ��ҽ��յ�������λ����Ϊ16λ����2�ֽڣ�
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
����ͻ���Transceive���0x0c�����Ͳ�����Ӧ�����+ Ҫ���͵����ݣ�0x93+0x20��
����ֵ��	 ����4Byte ID�ţ�1Byte У�飨Check��

��    �ܣ�����ײ
����˵��: pSnr[OUT]:��Ƭ���кţ�4�ֽ�
��    ��: �ɹ�����MI_OK
*/
unsigned char snr_check=0;//У��λ
char PcdAnticoll(unsigned char *pSnr)
{
	char status;//���ڴ洢����״̬
	unsigned char temp_check=0;//У��λ
	unsigned int unlen;//���ص����ݵ��ֽ���
	unsigned char ucComMF522Buf[MAXRLEN];//������
	
	// ��� Status2Reg �Ĵ����ģ�CRC�����־λ����ȷ����һ�β����Ĵ���״̬��Ӱ�챾�β���
    ClearBitMask(Status2Reg, 0x08);
	
	//BitFramingReg �Ĵ������ڿ���λ�ķ��ͺͽ���֡�ĸ�ʽ,����ʹ��Ĭ�ϸ�ʽ
    // ���� BitFramingReg�Ĵ���Ϊ 0��׼����������
    WriteRawRC(BitFramingReg, 0x00);
	
	//CollReg �Ĵ������ڴ������ͻ������Ӧ
    // ��� CollReg �Ĵ��������λ���������ͻ���
    ClearBitMask(CollReg, 0x80);
	
	ucComMF522Buf[0]=PICC_ANTICOLL1;//����ͻ
	ucComMF522Buf[1]=0x20;//NVB����Ч�ֽ���Ϊ2
	
	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unlen);
	
//	printf("Before Anticoll\r\n");
	if(status == MI_OK)
	{
		for(uint8_t i=0;i<4;++i)
		{
			// �����кű��浽 pSnr ָ����ָ����ڴ���
			*(pSnr+i)=ucComMF522Buf[i];
			// �����кŵ�ÿ���ֽڽ���������㣬���ں�����У��
			temp_check ^= ucComMF522Buf[i];
		}
		snr_check=ucComMF522Buf[4];
		//У�鲻ͨ��
		if(snr_check!=temp_check)
		{
			status=MI_ERR;
		}
	}
	
	// �� CollReg �Ĵ��������λ���Խ�ֹ��ͻ���
    SetBitMask(CollReg, 0x80);
	
	return status;
}

/*
ѡ�����Transceive���0x0c�����Ͳ�����Ӧ�����+ Ҫ���͵�����
		��0x93+0x20+4�ֽ�ID��+1�ֽ�У��Check+1�ֽ�CRC����
����ֵ	��0x08

��    �ܣ�ѡ����Ƭ
����˵��: pSnr[IN]:��Ƭ���кţ�4�ֽ�
��    ��: �ɹ�����MI_OK
*/
char PcdSelect(unsigned char *pSnr)
{
	char status;
    unsigned char i;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
	
	
	ucComMF522Buf[0] = PICC_ANTICOLL1;//ѡ��ָ����
    ucComMF522Buf[1] = 0x70;//NVB��Number of Valid Bits����ʾ���ݵ���Чλ��
	ucComMF522Buf[6] = snr_check;//У��Check
	
	//����ID��
	for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    }
	//����CRCУ���룺2���ֽ� �Է��ͻ�������ǰ7���ֽڼ���CRC16У���룬��������洢�ڻ������ĵ�8�͵�9���ֽ�
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
	
//	printf("Before Select\r\n");
								//9���������ݵ��ֽڳ��ȣ�0x93, 0x70, 4�ֽ�ID, У��Check, 2�ֽ�CRC����9�ֽڣ�
	status=PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
	
	// �����Ӧ�Ƿ���ȷ��״̬�Ƿ�Ϊ MI_OK �ҷ��ص����ݳ����Ƿ�Ϊ 0x18��
    if ((status == MI_OK) && (unLen == 0x18))
    {
        status = MI_OK;  // �ɹ�
    }
    else
    {
        status = MI_ERR;  // ʧ��
    }
    return status;
}

/*
��֤����֤��Կ�����Authent���0x0c�����Ͳ�����Ӧ�����+ Ҫ���͵�����
		��0x60+0x08����
����ֵ	���з���ֵ������һ�㲻����

��    �ܣ���֤��Ƭ����
����˵��: auth_mode[IN]: ������֤ģʽ
                0x60 = ��֤A��Կ
                0x61 = ��֤B��Կ 
         addr[IN]�����ַ(Ҫ��֤��һ��)��ȡֵΪ0-63
         pKey[IN]������
         pSnr[IN]����Ƭ���кţ�4�ֽ�
��    ��: �ɹ�����MI_OK
*/
char PcdAuthState(unsigned char auth_mode, unsigned char addr, unsigned char *pKey, unsigned char *pSnr)
{
	char status;
    unsigned int  unLen;
    unsigned char i,ucComMF522Buf[MAXRLEN]; 
	// ��ʼ���������
    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
	
	// �� 6 �ֽڵ����븴�Ƶ���������
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+2] = *(pKey+i);   }
	
	// �� 4 �ֽڵĿ�Ƭ���кŸ��Ƶ���������
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+8] = *(pSnr+i);   }
    
     // ������֤������ն���������Ӧ
    status = PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &unLen);

    // ��鷵��״̬����֤���
    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
    {
        status = MI_ERR;  // ��֤ʧ��
    }
    
    return status;  // ������֤״̬
}

/*
Transceive���0x0c��+ Ҫ���͵����ݣ�0x30+addr����

��    �ܣ���ȡM1��һ������,ֻ�ܶ���1��������ݣ���16���ֽ�
����˵��: addr[IN]�����ַ
          pData[OUT]�����������ݣ�16�ֽ�
��    ��: �ɹ�����MI_OK
*/
char PcdRead(unsigned char addr,unsigned char *pData)
{
    char status;
    unsigned int unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN]; 

    // ׼����������ַ
    ucComMF522Buf[0] = PICC_READ;  // ������ 0x30
    ucComMF522Buf[1] = addr;       // Ҫ��ȡ�Ŀ��ַ
    
    // ���� CRC У����
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
    
    // �������ݰ�����Ƭ��������Ӧ
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
    
    // ���״̬�ͽ��յ����ݳ����Ƿ���ȷ
    if ((status == MI_OK) && (unLen == 0x90)) // 0x90 ��ʾ�յ� 16 �ֽڵ����ݺ� 2 �ֽڵ� CRC У�飨144bit��
    {
        // �����յ��� 16 �ֽ����ݴ��� pData
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
Transceive���0x0c��+ Ҫ���͵����ݣ�0xA0+addr��

��    �ܣ�д���ݵ�M1��һ��
����˵��: addr[IN]�����ַ
          pData[IN]��д������ݣ�16�ֽ�
��    ��: �ɹ�����MI_OK
*/
char PcdWrite(unsigned char addr,unsigned char *pData)
{
    char status;
    unsigned int unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN]; 
    
    // ׼��д������ַ
    ucComMF522Buf[0] = PICC_WRITE;  // д���� 0xA0
    ucComMF522Buf[1] = addr;        // Ҫд��Ŀ��ַ
    
    // ���� CRC У���룬���������ӵ�2~3��������ַ
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
    
    // �����������ݰ�����Ƭ��������Ӧ
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

	//((ucComMF522Buf[0] & 0x0F) != 0x0A) ��������鿨Ƭ�ڽ���д�����ݺ󷵻ص�״̬�ֽڣ�ȷ��д�����ɹ�
    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }
    
    // �����Ƭ��׼���ý������ݣ����� 16 �ֽ�����
    if (status == MI_OK)
    {
        // ׼��Ҫд�������
        for (i = 0; i < 16; i++)
        {
            ucComMF522Buf[i] = *(pData + i);
        }

        // �������ݵ� CRC У���벢���ӵ�ĩβ
        CalulateCRC(ucComMF522Buf, 16, &ucComMF522Buf[16]);

        // �������ݺ� CRC У����
        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &unLen);

        // ���д����
		//((ucComMF522Buf[0] & 0x0F) != 0x0A) ��������鿨Ƭ�ڽ���д�����ݺ󷵻ص�״̬�ֽڣ�ȷ��д�����ɹ�
        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {
            status = MI_ERR;
        }
    }
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ����Ƭ��������״̬
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdHalt(void)
{
    unsigned int unLen; // ���ڽ������ݳ��ȵı���
    unsigned char ucComMF522Buf[MAXRLEN]; // ���ڴ洢���ͺͽ������ݵĻ�����

    ucComMF522Buf[0] = PICC_HALT; // ���͵ĵ�һ���ֽ��� PICC_HALT �����ʾ��Ƭ����
    ucComMF522Buf[1] = 0; // �ڶ����ֽ�ͨ��Ϊ 0����ʾû�ж��������

    // ����ǰ�����ֽڵ� CRC У���룬��������洢�� ucComMF522Buf �ĺ��������ֽ���
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
 
    // ����������տ�Ƭ����Ӧ
    PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    return MI_OK; // �̶����� MI_OK ��ʾ�ɹ�
}
