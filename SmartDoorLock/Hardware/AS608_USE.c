#include "AS608_use.h"
#include "oled.h"
#include "usart2.h"

extern SysPara AS608Para;							/*ָ��ģ��AS608����*/
extern u16 ValidN;									/*ģ������Чָ�Ƹ���*/
extern u8 FingerNum;

//��ʾȷ���������Ϣ
void ShowErrorMessage(u8 ensure)
{
	printf("\r\n������Ϣ��%s \r\n",(u8*)EnsureMessage(ensure));
}

void Press_Finger(void)
{
	SearchResult result;	// ���ڴ洢��������Ľṹ��
	u8 ensure;	// ���ڴ洢�������÷��ص�״̬��
	ensure=PS_GetImage();	// ��ȡָ��ͼ��
	if(ensure==0x00)	// ��ȡͼ��ɹ�
	{
		ensure=PS_GenChar(CharBuffer1);	// ��ͼ����������
		if(ensure==0x00) // ���������ɹ�
		{
			ensure=PS_HighSpeedSearch(CharBuffer1,0,AS608Para.PS_max,&result);// ����ָ�ƿ��е�ָ��
			if(ensure==0x00)//����������ָ�ƣ������ɹ�
			{
				OLED_Clear();  // ���OLED��ʾ��
                OLED_Write_N_Chinese(32, 1, 2, 3);  // ��ʾ"�ѽ���"�������ַ�
                OLED_Write_N_Chinese(56, 4, 1, 1);  // ��ʾ"��"�������ַ�
                Delay_ms(800);  // ��ʱ800����
                OLED_Write_N_Chinese(56, 4, 3, 1);  // ��ʾ"����"�������ַ�
                Delay_ms(800);  // ��ʱ800����
                OLED_Clear();  // ���OLED��ʾ��
			}
			else
			{
				ShowErrorMessage(ensure);
			}
		}
	}
}

//¼��ָ��
void Add_Finger(u16 FG_ID)
{
	u8 i, ensure, processnum = 0;  // `i` ���ڼ�����`ensure` �洢��������״̬�룬`processnum` �������̲���
    u16 ID;
	
	while(1)
	{
		switch(processnum)
		{
			case 0:	//¼��ͼ�������������浽CharBuffer1
				i++;
				printf("�밴ָ��\r\n");
				ensure=PS_GetImage(); // ��ʾ�û���ָ��
				if(ensure==0x00)
				{
					ensure=PS_GenChar(CharBuffer1); // ��������
					if(ensure==0x00)
					{
						printf("ָ������\r\n");
						i=0;  //�������
						processnum=1;  // �����ڶ���
					}
					else
					{
					ShowErrorMessage(ensure);  // ��ʾ������Ϣ   
					}
				}
				else
				{
				ShowErrorMessage(ensure);  // ��ʾ������Ϣ   
				}	
				break;
				
			case 1: //¼��ͼ�������������浽CharBuffer2
				i++;
				printf("���ٰ�һ��ָ��\r\n");
				ensure=PS_GetImage(); // ��ʾ�û���ָ��
				if(ensure==0x00)
				{
					ensure=PS_GenChar(CharBuffer1); // ��������
					if(ensure==0x00)
					{
						printf("ָ������\r\n");
						i=0;  //�������
						processnum=2;  // �����ڶ���
					}
					else
					{
					ShowErrorMessage(ensure);  // ��ʾ������Ϣ   
					}
				}
				else
				{
				ShowErrorMessage(ensure);  // ��ʾ������Ϣ   
				}	
				break;
				
			case 2: //�Ƚ�2��¼���ָ�������Ƿ�һ��
				printf("�Ա�����ָ��\r\n");  // �Ա����βɼ���ָ������
				ensure=PS_Match();
				if(ensure==0x00)
				{
					printf("�Ա�һ��\r\n");
					processnum=3;  // ����������
				}
				else
				{
					printf("�Ա�ʧ�ܣ�������¼��ָ��\r\n");
                    ShowErrorMessage(ensure);  // ��ʾ������Ϣ
                    i = 0;
                    processnum = 0;  // ���ص�һ��     
				}
				Delay_ms(1200);  // ��ʱ1200����
                break;
			
			case 3: //2��ָ�ƶ�һ������ϲ�����������ģ��
				printf("����ָ��ģ��\r\n");  // ����ָ��ģ��
				ensure=PS_RegModel();
				if(ensure==0x00)
				{
					printf("����ָ��ģ��ɹ�\r\n");
                    processnum = 4;  // �������岽
				}
				else
				{
					processnum = 0;
                    ShowErrorMessage(ensure);  // ��ʾ������Ϣ
				}
				Delay_ms(1200);  // ��ʱ1200����
                break;
				
			 case 4://����ָ�ƣ���CharBuffer2�е�ָ���������ݴ���flash���ݿ���    
                printf("����ָ��ID\r\n");  // ����ָ��ID
                printf("0=< ID <=299\r\n");  // ID ������0��299֮��
				if(FG_ID<AS608Para.PS_max)
				{
					ID=FG_ID;
				}
				ensure = PS_StoreChar(CharBuffer2, ID);  // ���ڶ��������洢Ϊָ��ģ��
                if (ensure == 0x00)
                {                    
                    printf("¼��ָ�Ƴɹ�\r\n");
                    OLED_Clear();  // ���OLED��ʾ��
                    OLED_Write_N_Chinese(16, 3, 15, 6);  // ��ʾ��¼��ɹ�������
                    Delay_ms(400);  // ��ʱ400����
                    PS_ValidTempleteNum(&ValidN);  // ��ȡ��ǰָ�ƿ��е�ָ������
                    return;  // ¼��ɹ����˳�����
                }
                else 
                {
                    processnum = 0;
                    ShowErrorMessage(ensure);  // ��ʾ������Ϣ
                }                    
                break;
		}
		Delay_ms(1000);  // ��ʱ1000����
		
        if (i == 5)  // ����5��û�а���ָ���˳�
		{
			printf("����5��û�а���ָ���˳�\r\n");
            OLED_Clear();  // ���OLED��ʾ��
            OLED_Write_N_Chinese(16, 3, 16, 6);  // ��ʾ��¼��ʧ�ܡ�����
            FingerNum--;  // ¼��ʧ�ܺ�ָ������1
            Delay_ms(100);  // ��ʱ100����
            break;    
		}
	}
}

//ɾ��ָ��
void Del_FR(u16 FG_ID)  /* ����ɾ��ָ��ID */
{
    u8 ensure;  // �洢��������״̬��
    printf("ɾ��ָ��\r\n");
    ensure = PS_DeletChar(FG_ID, 1);  // ɾ��ָ��ID�ĵ���ָ��
    if (ensure == 0)
    {
        printf("ɾ��ָ�Ƴɹ�\r\n");        
        OLED_Clear();  // ���OLED��ʾ��
        OLED_Write_N_Chinese(16, 3, 19, 6);  // ��ʾ��ɾ���ɹ�������
        Delay_ms(400);  // ��ʱ100����
    }
    else
        ShowErrorMessage(ensure);  // ��ʾ������Ϣ
    
    Delay_ms(1200);  // ��ʱ1200����
    PS_ValidTempleteNum(&ValidN);  // ��ȡ��ǰָ�ƿ��е�ָ������
}
