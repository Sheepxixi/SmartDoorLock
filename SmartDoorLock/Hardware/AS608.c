#include "stm32f10x.h"                  // Device header
#include "String.h"
#include "Delay.h"
#include "AS608.h"
#include "usart2.h"
//AS608ָ��ģ�����ţ�
//	TX-->	PA3
//	RX-->	PA2
//	WAK-->	PA1

// Ĭ��ָ��ģ���оƬ��ַ
u32 AS608Addr = 0XFFFFFFFF; 

//������WAK������PA1���ţ�
//WAK����⵽��ָ�ƣ�����ߵ�ƽ������PA1Ҫ���ó���������
void AS608_IO_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	//����PA1����
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
}

//static���εĺ�����ֻ���ڱ��ļ���ʹ��

//���ڷ���һ���ֽ�
static void MYUSART_SendData(u8 data)
{
	//���ͼĴ���Ϊ��ʱ���ſ��Է����ֽ�
	while((USART2->SR&0X40)==0); //״̬�Ĵ���SR�ĵ�6λTXE
	USART2->DR = data;
//	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==0);
//	USART_SendData(USART2,data);
}

//���Ͱ�ͷ��2���ֽڣ�0xEF01
static void SendHead(void)
{
	MYUSART_SendData(0xEF);
	MYUSART_SendData(0x01);
}

//����оƬ��ַ����4���ֽ�
static void SendAddr(void)
{
	//4���ֽ�Ҫ�ֳ�4�η�
	//�������8λ�������8λ���Ƶ���8λ
	MYUSART_SendData(AS608Addr>>24);
	MYUSART_SendData(AS608Addr>>16);
	MYUSART_SendData(AS608Addr>>8);
	MYUSART_SendData(AS608Addr);
}

//���Ͱ���ʶ����1���ֽڣ��������0x01�����ݰ���0x02����������0x08
static void SendFlag(u8 flag)
{
	MYUSART_SendData(flag);
}

//���Ͱ����ȣ���2���ֽ�
static void SendLength(int length)
{
	MYUSART_SendData(length>>8);
	MYUSART_SendData(length);
}

//����ָ���룺1���ֽ�
static void Sendcmd(u8 cmd)
{
	MYUSART_SendData(cmd);
}

//����У���:2���ֽ�
static void SendCheck(u16 check)
{
	MYUSART_SendData(check>>8);
	MYUSART_SendData(check);
}

/*
�жϽ��ջ���������������û�а���Ӧ���
����	��WaitTime��Ϊ�ȴ��жϽ������ݵ�ʱ�䣨��λ 1ms��
����ֵ	�����ݰ��׵�ַ
*/
static u8* JudgeStr(u16 WaitTime)
{
	char* ask_p;//ask_pָ�룺ƥ�䵽���ַ������׵�ַ
	
	//ƴ�ӳ�Ԥ�ڵ�Ӧ�����ʽ
	u8 ask[8];
	//��ͷ��2�ֽڣ�0xef01
	ask[0]=0xef;
	ask[1]=0x01;
	//оƬ��ַ��4�ֽ�
	ask[2]=AS608Addr>>24;
	ask[3]=AS608Addr>>16;
	ask[4]=AS608Addr>>8;
	ask[5]=AS608Addr;
	//����ʶ
	ask[6]=0x07;
	
	//��������ǣ������ȡ�ȷ����ȵȣ���Щ��ͬӦ�������һ����
	//���Բ�����д��ȥ��ƥ��ǰ��һ���ĸ�ʽ������
	
	//�ַ������������ڴ˴������������ã�
	ask[7] = '\0';   

	//���ý���״̬��־��׼����ʼ�µ����ݽ��չ���
	//��0-14λ�����յ������ݵ��ֽ���
	//��15λ��Ϊ1��������ɣ���һ���жϽ�����Ϻ���1������ȥ���жϷ�������
	USART2_RX_STA = 0;
	
	//�ȴ�ʱ������
	while(--WaitTime)
	{
		//��ʱ1����
		Delay_ms(1);
		
		//USART2_RX_STA��15λ�����1��˵�����յ���һ�������ҽ������
//		if( (USART2_RX_STA&(1<<15) )==1)
		if(USART2_RX_STA&0X8000)//���յ�һ������
		{
			//���������ϱ�־λ,���ڽ���״̬
			USART2_RX_STA=0;
			
			//�жϽ��ջ��������ܲ���ƥ�䵽Ӧ���
			//strstr(const char* serached,const char* serach)������
			//��һ������serached�������ҵ��ַ������ڶ�������serach��Ҫ����/ƥ����ַ���
			//���ƥ����ˣ�����serached��ƥ�䵽���Ӵ����׵�ַ�����򷵻�0
			
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
ָ��ģ����غ��������������ܽᣬ����ָ��ͺ�Ӧ����Ĳ��裺
1.��ʼ�����ͣ�
	SendHead()�����Ͱ�ͷ��0xEF01����
	SendAddr()������оƬ��ַ��4�ֽڣ���
	SendFlag(0x01)�����Ͱ���ʶ��ͨ��Ϊ 0x01����
	SendLength(X)�����Ͱ����ȣ�ָ���������ݵ��ֽ���������У��ͣ���
	Sendcmd(X)�����;����ָ���루�� 0x01, 0x02, �ȣ���
2.�������ݣ�
	���ݺ�������Ҫ��ͨ�� MYUSART_SendData() ���Ͷ�������ݲ������� BufferID��PageID �ȣ���
3.����У��ͣ�
	����Ӱ���ʶ��У���֮ǰ�����ֽڵĺͣ�ʹ�� SendCheck(CheckSum) ����У�����ȷ�����������ԡ�
4.�ȴ�Ӧ��
	���� JudgeStr(2000) �ȴ�Ӧ��������ȴ� 2000 ���룬����Ӧ�����ݵ�ָ�롣
5.����Ӧ��
	��� ask ָ���Ƿ���Ч��
	�����Ч����ȡȷ�������Ӧ�Ĳ�������ָ����������ȫ�ȼ����豸��ַ�ȣ���
	�����Ч������ȷ����Ϊ�����־���� 0xff����
6.���ؽ����
	����ȷ���룬��ָʾ�����Ľ�����ɹ�����󣩡�
*/

/*
¼��ͼ�� PS_GetImage
����:̽����ָ��̽�⵽��¼��ָ��ͼ�����ImageBuffer
�����������
���ز�����ȷ���� ��=00H��ʾ¼��ɹ�
ָ����룺01H 
*/
u8 PS_GetImage(void)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x03);//������	=������֮��У���(����У���)�����ֽ�����
	Sendcmd(0x01);//ָ���룬��ʾ����¼��ͼ�������
	
	//����У��ͣ�=�Ӱ���ʶ��ʼ(��������ʶ)��У���֮�������ֽ�֮��
	u16 CheckSum=0x01+0x03+0x01;//У���
	SendCheck(CheckSum);
	
	u8* ask = JudgeStr(2000);//ָ��Ӧ������׵�ַ��ָ��
	u8 ensure;//ȷ����
	//�ж�����Ӧ����Ӧ�����ȡȷ����
	if(ask)//��Ӧ���
	{
		ensure=ask[9];//��ȡȷ����
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
��������  PS_GenChar
����:��ImageBuffer�е�ԭʼͼ������ָ�������ļ�������CharBuffer1��CharBuffer2  
��������� BufferID(������������) 
���ز����� ȷ���֣�ȷ����=00H��ʾ���������ɹ�
ָ����룺 02H 
*/
u8 PS_GenChar(u8 BufferID)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x04);//������
	Sendcmd(0x02);//ָ����
	
	MYUSART_SendData(BufferID);//�������ݣ�BufferID(������������)
	
	u16 CheckSum=0x01+0x04+0x02+BufferID;//У���
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
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
��ȷ�ȶ���öָ������ PS_Match
����: ��ȷ�ȶ�CharBuffer1��CharBuffer2�е������ļ� �� 
��������� �� 
���ز����� ȷ���֣��ȶԵ÷�
ָ����룺 03H 
*/
u8 PS_Match(void)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x03);//������
	Sendcmd(0x03);//ָ����
	
	u16 CheckSum=0x01+0x03+0x03;//У���
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
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
����ָ��  PS_Search
���ܣ���CharBuffer1��CharBuffer2�е������ļ����������򲿷�ָ�ƿ⡣�����������򷵻�ҳ�� 
��������� BufferID��StartPage(��ʼҳ)��PageNum��ҳ����  
���ز����� ȷ���֣�ҳ�루����ָ��ģ�壩
ָ����룺 04H 
*/
u8 PS_Search(u8 BufferID,u16 StartPage,u16 PageNum,SearchResult *p)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x08);//������
	Sendcmd(0x04);//ָ����
	
	//��������
	MYUSART_SendData(BufferID);
	MYUSART_SendData(StartPage>>8);
	MYUSART_SendData(StartPage);
	MYUSART_SendData(PageNum>>8);
	MYUSART_SendData(PageNum);
	
	u16 CheckSum=0x01+0x08+0x04+BufferID
				+(StartPage>>8)+(u8)StartPage
				+(PageNum>>8)+(u8)PageNum;//У���
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
	if(ask)
	{
		ensure=ask[9];//����ȷ����
		p->pageID=(ask[10]<<8)+ask[11];//ҳ����Ӧ�����11��12���ֽ�(һ��2���ֽ�)
		p->mathscore=(ask[12]<<8)+ask[13];//�Աȵ÷���12��13���ֽ�
	}
	else
	{
		ensure=0xff;
	}
	return ensure;
}

/*
�ϲ�����������ģ�壩 PS_RegModel
����: ��CharBuffer1��CharBuffer2�е������ļ��ϲ����� 
ģ�壬�������CharBuffer1��CharBuffer2�� 
��������� �� 
���ز����� ȷ����
ָ����룺 05H 
*/
u8 PS_RegModel(void)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x03);//������
	Sendcmd(0x05);//ָ����
	
	u16 CheckSum=0x01+0x03+0x05;//У���
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
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
����ģ��  PS_StoreChar
����: ��CharBuffer1��CharBuffer2�е�ģ���ļ��浽PageID��
      ��flash���ݿ�λ�á� 
��������� BufferID(��������)��PageID��ָ�ƿ�λ�úţ� 
���ز����� ȷ����
ָ����룺 06H 
*/
u8 PS_StoreChar(u8 BufferID,u16 PageID)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x06);//������
	Sendcmd(0x06);//ָ����
	
	MYUSART_SendData(BufferID);
	MYUSART_SendData(PageID>>8);
	MYUSART_SendData(PageID);
	
	u16 CheckSum=0x01+0x06+0x06+BufferID+(PageID>>8)+(u8)PageID;//У���
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
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
(12) ɾ��ģ�� PS_DeletChar 
����    �� ɾ��flash���ݿ���ָ��ID�ſ�ʼ��N��ָ��ģ��
��������� PageID(ָ�ƿ�ģ���)��N��ɾ����ģ����� 
���ز����� ȷ����
ָ����룺 0cH 
*/
u8 PS_DeletChar(u16 PageID,u16 N)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x07);//������
	Sendcmd(0x0c);//ָ����
	
	MYUSART_SendData(PageID>>8);
	MYUSART_SendData(PageID);
	MYUSART_SendData(N>>8);
	MYUSART_SendData(N);
	
	u16 CheckSum=0x01+0x07+0x0c
				 +(PageID>>8)+(u8)PageID
				 +(N>>8)+(u8)N;//У���
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
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
(13) ���ָ�ƿ�  PS_Empty
����    �� ɾ��flash���ݿ�������ָ��ģ��
��������� �� 
���ز����� ȷ����
ָ����룺 0dH 
*/
u8 PS_Empty(void)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x03);//������
	Sendcmd(0x0d);//ָ����
	
	
	
	u16 CheckSum=0x01+0x03+0x0d;//У���
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
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
(14) дϵͳ�Ĵ��� PS_WriteReg
����    �� дģ��Ĵ���
��������� �Ĵ������ ��Ҫд��üĴ���������
���ز����� ȷ����
ָ����룺 0eH 
*/
u8 PS_WriteReg(u8 RegNum,u8 DATA)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x05);//������
	Sendcmd(0x0e);//ָ����
	
	MYUSART_SendData(RegNum);
	MYUSART_SendData(DATA);
	
	u16 CheckSum=0x01+0x05+0x0e+ RegNum+DATA;//У���
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0)
		printf("\r\n���ò����ɹ���");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(15) ��ϵͳ�������� PS_ReadSysPara
����    �� ��ģ��Ĵ���
��������� ��
���ز����� ȷ���� + ����������16bytes��
ָ����룺 0fH 
*/
u8 PS_ReadSysPara(SysPara *p)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x03);//������
	Sendcmd(0x0f);//ָ����
	
	u16 CheckSum=0x01+0x03+0x0f;//У���
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
	if(ask)
	{
		ensure=ask[9];
		p->PS_max=(ask[14]<<8)+ask[15];//ָ�����������2�ֽ�
		p->PS_level=ask[17];//��ȫ�ȼ���2�ֽڣ�����ֻ�õ��˵��ֽ�
		p->PS_addr=(ask[18]<<24)+(ask[19]<<16)+(ask[20]<<8)+ask[21];//�豸��ַ��4�ֽ�
		p->PS_size=ask[23];//���ݰ���С��2�ֽڣ�Ҳ��ֻ�õ����ֽ�
		p->PS_N=ask[25];//���������ã�2�ֽڣ�(������Ϊ9600*N bps) 
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0)
	{
		printf("\r\nģ�����ָ������=%d",p->PS_max);
		printf("\r\n�Աȵȼ�=%d",p->PS_level);
		printf("\r\n��ַ=%x",p->PS_addr);
		printf("\r\n������=%d",p->PS_N*9600);
	}
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(21) ����оƬ��ַ PS_SetChipAddr
����    �� ����оƬ��ַ
��������� PS_addr���豸��ַ
���ز����� ȷ����
ָ����룺 0fH 
*/
u8 PS_SetChipAddr(u32 PS_addr)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x07);//������
	Sendcmd(0x15);//ָ����
	
	MYUSART_SendData(PS_addr>>24);
	MYUSART_SendData(PS_addr>>16);
	MYUSART_SendData(PS_addr>>8);
	MYUSART_SendData(PS_addr);
	
	u16 CheckSum=0x01+0x07+0x15
				+(PS_addr>>24)+(PS_addr>>16)
				+(PS_addr>>8)+(u8)PS_addr;//У���
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0x00)
		printf("\r\n���õ�ַ�ɹ���");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(24) д���±�  PS_WriteNotepad 
����:ģ���ڲ�Ϊ�û�������256bytes��FLASH�ռ����ڴ���û����ݣ�
�ô洢�ռ��Ϊ�û����±����ü��±��߼��ϱ��ֳ�16��ҳ��д����
����������д���û���32bytes���ݵ�ָ���ļ��±�ҳ
���������  NotePageNum��ҳ�루0-15�� �� user content���û���Ϣ��32�ֽڣ�
���ز����� ȷ����
ָ����룺 18H 
*/
u8 PS_WriteNotepad(u8 NotePageNum,u8 * Byte32)//Byte32��ָ�룬ָ��һ��32���ֽڵ�����
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x36);//������
	Sendcmd(0x18);//ָ����
	
	MYUSART_SendData(NotePageNum);//����flashҳ��
	u16 CheckSum=0;//У���
	
	for(uint8_t i=0;i<32;++i)//����32�ֽڵ��û���Ϣ
	{
		MYUSART_SendData(Byte32[i]);
		CheckSum+=Byte32[i];
	}
	
	CheckSum=0x01+0x36+0x18+CheckSum+NotePageNum;//У���	
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
	if(ask)
	{
		ensure=ask[9];
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0x00)
		printf("\r\nд���±��ɹ���");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(25) �����±�  PS_ReadNotepad 
����    ����ȡFLASH�û�����128bytes����.
���������  NotePageNum��ҳ�루0-15�� �� Byte32���������ݻ�������32�ֽڣ�
���ز����� ȷ����
ָ����룺 19H 
*/
u8 PS_ReadNotepad(u8 NotePageNum,u8 * Byte32)//Byte32��ָ�룬ָ��һ��32���ֽڵ����飬���ڽ��ն�����flash�û���Ϣ
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x04);//������
	Sendcmd(0x19);//ָ����
	
	MYUSART_SendData(NotePageNum);//����flashҳ��
	
	u16 CheckSum=0x01+0x04+0x19+NotePageNum;//У���	
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
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
		printf("\r\n�����±��ɹ���");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(25) ��������ָ��  PS_HighSpeedSearch 
����    ���� CharBuffer1��CharBuffer2�е������ļ��������������򲿷�ָ�ƿ⡣
		  �����������򷵻�ҳ��,��ָ����ڵ�ȷ������ָ�ƿ��� ���ҵ�¼ʱ����
		  �ܺõ�ָ�ƣ���ܿ�������������
���������  NotePageNum��ҳ�루0-15�� �� Byte32���������ݻ�������32�ֽڣ�
���ز����� ȷ���֣�ҳ�루����ָ��ģ�壩���Աȵ÷�
ָ����룺 1bH 
*/
u8 PS_HighSpeedSearch(u8 BufferID,u16 StartPage,u16 PageNum,SearchResult *p)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x08);//������
	Sendcmd(0x1b);//ָ����
	
	MYUSART_SendData(BufferID);
	MYUSART_SendData(StartPage>>8);
	MYUSART_SendData(StartPage);
	MYUSART_SendData(PageNum>>8);
	MYUSART_SendData(PageNum);
	
	u16 CheckSum=0x01+0x08+0x1b+BufferID
				+(StartPage>>8)+(u8)StartPage
				+(PageNum>>8)+(u8)PageNum;//У���	
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
	if(ask)
	{
		ensure=ask[9];
		p->pageID=(ask[10]<<8)+(u8)ask[11];//ҳ�룬��flash��һҳ
		p->mathscore=(ask[12]<<8)+(u8)ask[13];//�Աȵȼ�
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0x00)
		printf("\r\n�����±��ɹ���");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

/*
(29) ����Чģ�����  PS_ValidTempleteNum 
����    �� ����Чģ�����
��������� ��
���ز����� ȷ���֣���Чģ�����ValidN
ָ����룺 1dH 
*/
u8 PS_ValidTempleteNum(u16 *ValidN)
{
	
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x03);//������
	Sendcmd(0x1d);//ָ����
	
	u16 CheckSum=0x01+0x03+0x1d;//У���	
	SendCheck(CheckSum);
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
	if(ask)
	{
		ensure=ask[9];
		*ValidN=(ask[10]<<8)+ask[11];//����u8�ϳ�u16 ����0x1200 + 0x34 = 0x1234
	}
	else
	{
		ensure=0xff;
	}
	
	if(ensure==0x00)
		printf("\r\n��Чָ�Ƹ���=%d",(ask[10]<<8)+ask[11]);
	else
		printf("\r\n%s",EnsureMessage(ensure));
	
	return ensure;
}

// �� AS608 ģ������
// ����: PS_Addr ��ַָ�루���ڴ洢ģ�鷵�صĵ�ַ��
// ����ֵ: 0 ��ʾ�ɹ���1 ��ʾʧ��
u8 PS_HandShake(u32 *PS_Addr)
{
    // ���Ͱ�ͷ
    SendHead();
    
    // �����豸��ַ
    SendAddr();
    SendFlag(0x01);//����ʶ
	SendLength(0x03);//������
	Sendcmd(0X01);//ָ����
    
    // ��������������� 0x00, 0x00������Ϊ 0��
	/*
	���������з��͵� 0x00, 0x00�����Ƿ��� AS608 ģ��ͨ��Э���Ҫ��������Ϊռλ��������������������ĸ�ʽ��
	�������������Ҫ����Ĳ�������˷����������ֽں󼴿��������ķ��͹��̡�
	*/
    MYUSART_SendData(0X00);
    MYUSART_SendData(0X00);  
    
    // �ӳ� 200ms���ȴ�ģ�鷵������
    Delay_ms(200);
    
    // ��� USART2_RX_STA ��־λ���ж��Ƿ���յ�����
    if(USART2_RX_STA & 0X8000) // ���յ�����
    {       
        // �жϽ��յ��������Ƿ���ģ�鷵�ص�Ӧ���
        if(
            USART2_RX_BUF[0] == 0XEF &&  // ��ͷ
            USART2_RX_BUF[1] == 0X01 &&  // ����ʶ��
            USART2_RX_BUF[6] == 0X07     // Ӧ�����ʶ��
        )
        {
            // �������صĵ�ַ���洢�� PS_Addr ָ��ָ��ı�����
            *PS_Addr = (USART2_RX_BUF[2] << 24) + 
                       (USART2_RX_BUF[3] << 16) + 
                       (USART2_RX_BUF[4] << 8) + 
                       USART2_RX_BUF[5];  
            // ������ձ�־
            USART2_RX_STA = 0;
            
            // ���� 0����ʾ���ֳɹ�
            return 0;
        }
        // ������ձ�־
        USART2_RX_STA = 0;                 
    }
    
    // ���� 1����ʾ����ʧ��
    return 1;       
}

// ����ȷ�����ֵ��������Ӧ�Ĵ�����Ϣ�ַ���
// ����: ensure ȷ����
// ����ֵ: ��Ӧ�Ĵ�����Ϣ�ַ���
const char *EnsureMessage(u8 ensure) //��֤���ص��ַ��������޸�
{
    const char *p;
    
    //����ȷ�����ֵ��������Ӧ�Ĵ�����Ϣ
    switch(ensure)
    {
        case  0x00:
            p = "OK"; break; // �����ɹ�
        case  0x01:
            p = "���ݰ����մ���"; break; // �������ݰ�ʱ����
        case  0x02:
            p = "��������û����ָ"; break; // û�м�⵽��ָ
        case  0x03:
            p = "¼��ָ��ͼ��ʧ��"; break; // ¼��ָ��ͼ��ʧ��
        case  0x04:
            p = "ָ��ͼ��̫�ɡ�̫��������������"; break; // ͼ����ɻ����
        case  0x05:
            p = "ָ��ͼ��̫ʪ��̫��������������"; break; // ͼ���ʪ�����
        case  0x06:
            p = "ָ��ͼ��̫�Ҷ�����������"; break; // ͼ�����
        case  0x07:
            p = "ָ��ͼ����������������̫�٣������̫С��������������"; break; // ��������ٻ������С
        case  0x08:
            p = "ָ�Ʋ�ƥ��"; break; // ָ�Ʋ�ƥ��
        case  0x09:
            p = "û������ָ��"; break; // û���ҵ�ƥ���ָ��
        case  0x0a:
            p = "�����ϲ�ʧ��"; break; // �ϲ�����ʧ��
        case  0x0b:
            p = "����ָ�ƿ�ʱ��ַ��ų���ָ�ƿⷶΧ"; break; // ��ַ����ָ�ƿⷶΧ
        case  0x10:
            p = "ɾ��ģ��ʧ��"; break; // ɾ��ָ��ģ��ʧ��
        case  0x11:
            p = "���ָ�ƿ�ʧ��"; break; // ���ָ�ƿ�ʧ��
        case  0x15:
            p = "��������û����Чԭʼͼ��������ͼ��"; break; // û����Ч��ԭʼͼ��
        case  0x18:
            p = "��д FLASH ����"; break; // ��д FLASH ����
        case  0x19:
            p = "δ�������"; break; // δ�������
        case  0x1a:
            p = "��Ч�Ĵ�����"; break; // ��Ч�Ĵ�����
        case  0x1b:
            p = "�Ĵ����趨���ݴ���"; break; // �Ĵ����趨���ݴ���
        case  0x1c:
            p = "���±�ҳ��ָ������"; break; // ���±�ҳ��ָ������
        case  0x1f:
            p = "ָ�ƿ���"; break; // ָ�ƿ�����
        case  0x20:
            p = "��ַ����"; break; // ��ַ����
        default:
            p = "ģ�鷵��ȷ��������"; break; // ȷ�������
    }
    
    // ���ش�����Ϣ�ַ���
    return p;    
}

/*
�� ����ģ��  PS_LoadChar
����: �� ��flash���ݿ���ָ��ID�ŵ�ָ��ģ����뵽ģ�建����CharBuffer1��CharBuffer2 
��������� BufferID(��������)��PageID��ָ�ƿ�ģ��ţ� 
���ز����� ȷ����
ָ����룺 07H 
*/
u8 PS_LoadChar(u8 BufferID,u16 PageID)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x06);//������
	Sendcmd(0x07);//ָ����
	
	MYUSART_SendData(BufferID);
	MYUSART_SendData(PageID>>8);
	MYUSART_SendData(PageID);
	
	u16 CheckSum=0x01+0x06+0x07+BufferID+(PageID>>8)+(u8)PageID;//У���
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
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
�� �ϴ�������ģ��  PS_UpChar �����������ҪҲ�У�
����    �� �������������е������ļ��ϴ�����λ����
��������� BufferID(��������) 
���ز����� ȷ����
ָ����룺 08H 
*/
u8 PS_UpChar(u8 BufferID,u16 PageID)
{
	SendHead();//��ͷ
	SendAddr();//оƬ��ַ
	SendFlag(0x01);//����ʶ
	SendLength(0x04);//������
	Sendcmd(0x08);//ָ����
	
	MYUSART_SendData(BufferID);
	
	u16 CheckSum=0x01+0x04+0x08+BufferID;//У���
	SendCheck(CheckSum);
	
	
	u8* ask=JudgeStr(2000);//Ӧ����׵�ַ��ָ��
	u8 ensure;//ȷ����
	//��Ӧ���
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
