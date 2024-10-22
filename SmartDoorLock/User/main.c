#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "Keyboard.h"
#include "String.h"
#include "RC522.h"
#include "Blueteeth.h"
#include "AS608.h"
#include "AS608_use.h"
#include "usart2.h"

//ָ��ģ�����
#define usart2_baund  57600/*����2�����ʣ�����ָ��ģ�鲨���ʸ���*/
SysPara AS608Para;/*ָ��ģ��AS608�Ļ����������ṹ�壩*/
u16 ValidN;/*ģ������Чָ�Ƹ���*/
u8 FingerNum = 1;
u8 ensure;
void AS608_Init(void);

//��������ģ�����
char buf[4] = {'\0'}; // ����������ŵ�ǰ�Ѿ����������
char admin_password[4] = {'1', '2', '4', '5'}; // ����Ա����
char password[4] = {'1', '2', '3', '4'}; //��������
uint8_t key_value; // ��ǰ���µİ����ļ�ֵ
uint8_t key_index = 0; // ��ǰ����ڼ�λ������
uint8_t key_lock=0;//key_lock=1�������޸�����
void Password_unlock(void);
u8 Offset;
volatile int admin_unlocked = 0; // �Ƿ�������Աģʽ
volatile int password_unlocked = 0;//�Ƿ����
void delate_finger_oled(void);
void add_finger_oled(void);

//RC522ģ�����
uint8_t Card_Type1[2];//������
uint8_t Card_ID[4];//����ҳ�����ʱɨ�赽�Ŀ�ID��
uint8_t Stored_Card_ID[4];//�Ѵ洢�Ŀ�ƬID��
uint8_t Card_KEY[6] = {0xff,0xff,0xff,0xff,0xff,0xff};//{0x11,0x11,0x11,0x11,0x11,0x11};   //��Կ
uint8_t Card_Data[16];//��ȡ���Ŀ��ַ���������
uint8_t status;//RFID��Ƶ��������ִ��״̬
static uint32_t ReadCard_Time=0;//��ǰ�������õ�ʱ�䣬��ʱ�˳�
static uint8_t ReadCard_Flag=0;
#define MAX_ALLOWED_CARDS 10  // �趨�������Ŀ�Ƭ����
uint8_t allowed_cards[MAX_ALLOWED_CARDS][4] = {
    {0x12, 0x34, 0x56, 0x78},  // ʾ����Ƭ1
    {0x87, 0x65, 0x43, 0x21},  // ʾ����Ƭ2
};
int num_allowed_cards = 2;  // ��ǰ��Ƭ����

int is_card_allowed(uint8_t card_id[4]) 
{
    for (int i = 0; i < num_allowed_cards; i++) 
	{
        if (memcmp(card_id, allowed_cards[i], 4) == 0) 
		{
            return 1; // ��Ƭ������
        }
    }
    return 0; // ��Ƭ��������
}

//����ģ�����
extern u8 res;
extern u8 res_flag;

void Show_Main_Menu(void);
void Show_Admin_Menu(void);
void Add_Card(void);

int main(void)
{
    Serial_Init();  // ��ʼ������
	RC522_Init();
	delay_init();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	OLED_Init();
	OLED_Clear();
	uart3_init();
	AS608_Init();
    while (1)
    {
		Show_Main_Menu();
    }

}

void Show_Main_Menu(void)
{
    while(1)
    {
        if(ReadCard_Flag)
		{
			ReadCard_Time++;
		}
		// ��ҳ����ʾ
        OLED_Clear();
		//��ʾ���֣������Ž�ϵͳ��
		OLED_Write_N_Chinese(16, 1, 0, 6);
		//��ʾ��
		OLED_Write_N_Chinese(56, 4, 1, 1);
		OLED_ShowString(0, 7, "Back", 6);

        // �ȴ���������
        while(1)
        {
            int key_value = Key_Scan();
			/*
			1    2    3    CLR
			4    5    6    UP
			BAK  0    SEE  OK
			7    8    9    DOWN
            */
            // ����ҳ�水��BACK����Ϩ��
            if (key_value == BACK)
            {
                OLED_Clear();
                Delay_ms(100);
            }
			//1.�������
            // ���°���0�����������ҳ��
            if (key_value == 0)
            {
                Password_unlock();
                if (admin_unlocked) // ����Ա�������
                {
                    admin_unlocked = 0;  // ���ý���״̬
					OLED_Write_N_Chinese(32, 1, 2, 3); // ��ʾ�ѽ���
					Delay_ms(500);
                    Show_Admin_Menu(); // ��ת������Աҳ��
                    return;
                }
				if(password_unlocked)
				{
					OLED_Write_N_Chinese(32, 1, 2, 3); // ��ʾ�ѽ���
					Delay_ms(5000);
					return;
				}
            }
			
			//2.��Ƭ����
			if (MI_OK == PcdRequest(0x52, Card_Type1))  // Ѱ������������ɹ�����MI_OK 
			{
				if (ReadCard_Flag == 0) 
				{
					ReadCard_Flag = 1;  // ��Ϊ1����ʾ�Ѿ���⵽��Ƭ����ʼ����
					ReadCard_Time = 0;  // ���ö���ʱ�����
				}
				
				OLED_Clear();
				OLED_Write_N_Chinese(32, 2, 5, 5);  //��ʾ����ȡ��Ƭ��
				Delay_ms(500);
				
				status = PcdAnticoll(Card_ID); // ����ײ ����ɹ�����MI_OK
				if (status == MI_OK) 
				{
					ReadCard_Flag=0;//����������ɣ����������־λ
					// ��֤��Ƭ�Ƿ�����
					if (is_card_allowed(Card_ID)) 
					{
						OLED_Clear();
						OLED_Write_N_Chinese(32, 1, 2, 3); // ��ʾ�ѽ���
						Delay_ms(5000);
						printf("Card allowed. Unlocking...\r\n");
						return;
					} 
					else 
					{
						printf("Card not allowed. Returning to main screen.\r\n");
						return;  // ��Ƭ������������������
					}
				} 
				else 
				{
					ReadCard_Flag=0;//���������˳������������־λ
					printf("Anticoll Error\r\n");
					return;  // ��Ƭ��ȡʧ�ܣ�����������
				}
				
			}
			else if (ReadCard_Time >= 10000000) 
			{
				ReadCard_Flag = 0;  // ��ʱ�������־λ
				ReadCard_Time = 0;  // ���ö���ʱ��
			}
			
			//3.��������
			//��1������������
			if(key_value == 1)
			{
				printf("blueteeth\r\n");
				OLED_Clear();
				OLED_Write_N_Chinese(24,3,25,5);
				while(res_flag==0);
				while(res_flag==1)
				{
					printf("main:res == %x\r\n",res);
					if(res==0x12)
					{
						res=0;
						res_flag=0;
						printf("right%x\r\n",res);
						OLED_Clear();
						OLED_Write_N_Chinese(32, 1, 2, 3); // ��ʾ�ѽ���
						Delay_ms(5000);
						return;
					}
					else
					{
						OLED_Clear();
						OLED_Write_N_Chinese(16,3,26,6);
						Delay_ms(1000);
						return;
					}
				}
			}
			
			//4.ָ�ƽ���
			if(PS_Sta)////��ָ��ģ��״̬����,�鿴�Ƿ���ָ�ƴ���
			{
				OLED_Clear();
				OLED_Write_N_Chinese(32,2,4,5);	
				Delay_ms(500);
				while(1)
				{
					u8 ensure;
					SearchResult result;
					//¼��ָ��ͼ��
					ensure=PS_GetImage();
					if(ensure==0x00)
					{
						//��������������ѡ��CharBuffer1
						ensure=PS_GenChar(CharBuffer1);
						if(ensure==0x00)//�ɹ�
						{
							//��������/�ȶ�
							ensure=PS_HighSpeedSearch(CharBuffer1,0,AS608Para.PS_max,&result);
							if(ensure==0x00)//�ɹ�
							{
								//��ʾ�ѽ���
								OLED_Clear();  // ���OLED��ʾ��
								OLED_Write_N_Chinese(32, 1, 2, 3); // ��ʾ�ѽ���
								Delay_ms(5000);
								return;
							}
							else
							{
								ShowErrorMessage(ensure);//��ӡ������Ϣ
								break;//�д�������ѭ��
							}
						}
						else
						{
							ShowErrorMessage(ensure);//��ӡ������Ϣ
							break;//�д�������ѭ��
						}
					}
					else
					{
						ShowErrorMessage(ensure);//��ӡ������Ϣ
						break;//�д�������ѭ��
					}
				}
				return;//����ʲôԭ���˳������ص�������
			}
			
            Delay_ms(100);
        }
    }
}

void Show_Admin_Menu(void)
{
    // ����ҳ���߼�
    int Offset = 0;
    OLED_Clear();
    OLED_ShowString(8, Offset, "->", 8);
    OLED_Write_N_Chinese(32, 0, 7, 4);//�޸�����
    OLED_Write_N_Chinese(32, 2, 8, 4);//���ָ��
    OLED_Write_N_Chinese(32, 4, 9, 4);//ɾ��ָ��
    OLED_Write_N_Chinese(32, 6, 10, 4);//��ӿ�Ƭ

    while(1)
    {
        int key_value = Key_Scan();
        if (key_value == BACK)
        {
            return;  // �������˵�
        }
        if (key_value == UP)
        {
            OLED_ShowString(8, Offset, "  ", 8);
            if (Offset == 0) Offset = 6;
            else Offset -= 2;
            OLED_ShowString(8, Offset, "->", 8);
        }
        if (key_value == DOWN)
        {
            OLED_ShowString(8, Offset, "  ", 8);
            if (Offset == 6) Offset = 0;
            else Offset += 2;
            OLED_ShowString(8, Offset, "->", 8);
        }
        if (key_value == ACK)
        {
            switch(Offset)
			{
				case 0://�޸�����
					key_lock=1;//��key_lockΪ1����ʾ�����޸�����
					Password_unlock();//�����µ�����
					if (key_lock == 0) // �޸�������ɺ�
					{
						OLED_Clear();
						Delay_ms(2000); // ��ʾ2��
						Show_Admin_Menu(); // ���ع���Աҳ��
					}
					break;
				case 2://���ָ��
					FingerNum++;
					add_finger_oled();
					Add_Finger(FingerNum);
					Show_Admin_Menu();
				case 4://ɾ������¼���һ��ָ��
					delate_finger_oled();
					Del_FR(FingerNum);
					FingerNum--;
					Show_Admin_Menu();
				case 6://��ӿ�Ƭ
					Add_Card();Show_Admin_Menu(); break; // ��ӿ�Ƭѡ��	
			}
        }
    }
}


void Password_unlock(void)
{
    key_index = 0;
    memset(buf, '\0', 4);

    OLED_Clear();
    OLED_Write_N_Chinese(0, 0, 6, 2); // ��ʾ������ʾ
    OLED_ShowChar(32, 0, ':', 6);
    OLED_ShowChar(51, 2, '_', 6);
    OLED_ShowChar(21, 3, '1', 6);
    OLED_ShowChar(48, 3, '2', 6);
    OLED_ShowChar(75, 3, '3', 6);
    OLED_ShowString(102, 3, "CLR", 6);
    OLED_ShowChar(21, 4, '4', 6);
    OLED_ShowChar(48, 4, '5', 6);
    OLED_ShowChar(75, 4, '6', 6);
    OLED_ShowString(102, 4, "UP", 6);
    OLED_ShowString(21, 5, "Bak", 6);
    OLED_ShowChar(48, 5, '0', 6);
    OLED_ShowString(75, 5, "See", 6);
    OLED_ShowString(102, 5, "ACK", 6);
    OLED_ShowChar(21, 6, '7', 6);
    OLED_ShowChar(48, 6, '8', 6);
    OLED_ShowChar(75, 6, '9', 6);
    OLED_ShowString(102, 6, "DOWN", 6);

    while (1)
    {
        key_value = Key_Scan();

        uint8_t x = 51 + key_index * 19;
        if (0 < key_value && key_value < 10)
        {
            if (key_index < 4)
            {
                buf[key_index] = key_value + '0';
                x = 51 + key_index * 19;
                OLED_ShowChar(x, 0, '*', 6);
                OLED_ShowChar(x, 2, ' ', 6);
                OLED_ShowChar(x + 19, 2, '_', 6);//��һ������λ
                key_index++;
            }
        }
        else if (key_value == CLEAR)
        {
            if (key_index > 0)
            {
				x = 51 + key_index * 19;
                key_index--;
                buf[key_index] = '0';
                OLED_ShowChar(x - 19, 0, ' ', 6);
                OLED_ShowChar(x - 19, 2, '_', 6);
                OLED_ShowChar(x - 19, 2, ' ', 6);
            }
        }
        else if (key_value == BACK)
        {
            return; // ������ҳ��
        }
        else if (key_value == ACK)
        {
            if (strncmp(buf, admin_password, 4) == 0)//������ȷ
            {
                OLED_Clear();
                OLED_Write_N_Chinese(32, 1, 2, 3); // ��ʾ�ѽ���
                Delay_ms(1000);
                admin_unlocked = 1;  // ���ý���״̬
                return; // ������ҳ��
            }
			else if(strncmp(buf, password, 4) == 0)//������ȷ
            {
                OLED_Clear();
                OLED_Write_N_Chinese(32, 1, 2, 3); // ��ʾ�ѽ���
                Delay_ms(1000);
                password_unlocked = 1;  // ���ý���״̬
                return; // ������ҳ��
            }
            else//�������
            {
				//��ʾ�������
                OLED_Clear();
				OLED_ShowString(8,3,"Password error",8);
                Delay_ms(1000);
                memset(buf, '\0', 4);
                key_index = 0;
				//���ؽ���ҳ��������������
                OLED_Clear();
                OLED_Write_N_Chinese(0, 0, 6, 2); // ��ʾ������ʾ
                OLED_ShowChar(32, 0, ':', 6);
                OLED_ShowChar(51, 2, '_', 6);
                OLED_ShowChar(21, 3, '1', 6);
                OLED_ShowChar(48, 3, '2', 6);
                OLED_ShowChar(75, 3, '3', 6);
                OLED_ShowString(102, 3, "Clr", 6);
                OLED_ShowChar(21, 4, '4', 6);
                OLED_ShowChar(48, 4, '5', 6);
                OLED_ShowChar(75, 4, '6', 6);
                OLED_ShowString(102, 4, "Lef", 6);
                OLED_ShowChar(21, 5, '7', 6);
                OLED_ShowChar(48, 5, '8', 6);
                OLED_ShowChar(75, 5, '9', 6);
                OLED_ShowString(102, 5, "Rig", 6);
                OLED_ShowString(21, 6, "Bak", 6);
                OLED_ShowChar(48, 6, '0', 6);
                OLED_ShowString(75, 6, "See", 6);
                OLED_ShowString(102, 6, "OK", 6);
            }
        }
        Delay_ms(100);
    }
}

//��ӿ�Ƭ
void Add_Card(void)
{
    uint8_t new_card_id[4];//��ӵĿ�Ƭ��ID��
	 uint32_t timeout = 10;  // ���ó�ʱ����
	uint32_t elapsed_time = 0;  // ���ڼ�¼�Ѿ�������ʱ��
	
	OLED_Clear();
	OLED_Write_N_Chinese(32,2,5,5);//��ʾ����ȡ��Ƭ��
	
	//��ȡ��ƬID
	while(1)
	{
		//Ѱ��
		if(PcdRequest(0x52,Card_Type1)==MI_OK)
		{
			//ͨ������ͻ���������ؿ�ID
			if(PcdAnticoll(new_card_id)==MI_OK)
			{
				printf("Serial Number:%d %d %d %d\r\n",Card_ID[0],Card_ID[1],Card_ID[2],Card_ID[3]);
				//�жϿ����Ƿ��Ѵ���
				if(is_card_allowed(new_card_id))
				{
					OLED_Clear();
					OLED_Write_N_Chinese(32,2,20,5);//��ʾ����Ƭ�Ѵ���
					Delay_ms(500);
				}
				else
				{
					//�жϿ�Ƭ�����Ƿ�ﵽ����
					if(num_allowed_cards<MAX_ALLOWED_CARDS)
					{
						//��ӿ�Ƭ���б�
						memcpy(allowed_cards[num_allowed_cards],new_card_id,4);
						num_allowed_cards++;//������Ŀ�Ƭ������1	
						OLED_Clear();
						OLED_Write_N_Chinese(24,2,21,6);//��ʾ����Ƭ��ӳɹ�
						Delay_ms(500);
					}
					else
					{
						OLED_Clear();
						OLED_Write_N_Chinese(24,2,22,6);//��ʾ����Ƭ��������
						Delay_ms(500);
					}
				}
				//�����ɹ��󷵻�
				return;
			}
			
			Delay_ms(100);//ÿ��ѭ����ʱ100ms
			elapsed_time++;// ��������ʱ��
			//�ж��Ƿ�ʱ����ʱ���˳���timeout=50�������ʱ50��
			if(elapsed_time>=timeout)
			{
				OLED_Clear();
				OLED_Write_N_Chinese(24,2,23,6);//��ʾ����ȡ��Ƭ��ʱ
				return;
			}
		}
	}
}

//2����ʾ������
void delate_finger_oled(void)//ɾָ����ʾ����
{
	OLED_Clear();
	OLED_Write_N_Chinese(32,2,18,5);	
	Delay_ms(500);
}

void add_finger_oled(void)//¼ָ����ʾ����
{
	OLED_Clear();
	OLED_Write_N_Chinese(32,2,17,5);	
	Delay_ms(500);
}

void AS608_Init(void)
{
	printf("��ʼ����\r\n");
	usart2_init(usart2_baund);	/*��ʼ������2,������ָ��ģ��ͨѶ*/
	AS608_IO_Init();					/*��ʼ��FR��״̬����*/
	printf("��ָ��ģ������\r\n");
	while(PS_HandShake(&AS608Addr))			/*��AS608ģ������*/
	{
		Delay_ms(400);
		printf("δ��⵽ģ��\r\n");
		Delay_ms(800);
		printf("������������ģ��\r\n"); 
	}
	printf("ͨѶ�ɹ�\r\n");
	printf("������:%d   ��ַ:%x\r\n",usart2_baund,AS608Addr);		/*��ӡ��Ϣ*/
	ensure=PS_ValidTempleteNum(&ValidN);										/*����ָ�Ƹ���*/
	if(ensure!=0x00)
		ShowErrorMessage(ensure);								/*��ʾȷ���������Ϣ*/
	ensure=PS_ReadSysPara(&AS608Para);  		/*������ */
	if(ensure==0x00)
	{
		printf("������:%d     �Աȵȼ�: %d",AS608Para.PS_max-ValidN,AS608Para.PS_level);
	}
	else
		ShowErrorMessage(ensure);	
}
