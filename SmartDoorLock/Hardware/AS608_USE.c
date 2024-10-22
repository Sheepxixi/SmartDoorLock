#include "AS608_use.h"
#include "oled.h"
#include "usart2.h"

extern SysPara AS608Para;							/*指纹模块AS608参数*/
extern u16 ValidN;									/*模块内有效指纹个数*/
extern u8 FingerNum;

//显示确认码错误信息
void ShowErrorMessage(u8 ensure)
{
	printf("\r\n错误信息：%s \r\n",(u8*)EnsureMessage(ensure));
}

void Press_Finger(void)
{
	SearchResult result;	// 用于存储搜索结果的结构体
	u8 ensure;	// 用于存储函数调用返回的状态码
	ensure=PS_GetImage();	// 获取指纹图像
	if(ensure==0x00)	// 获取图像成功
	{
		ensure=PS_GenChar(CharBuffer1);	// 将图像生成特征
		if(ensure==0x00) // 生成特征成功
		{
			ensure=PS_HighSpeedSearch(CharBuffer1,0,AS608Para.PS_max,&result);// 搜索指纹库中的指纹
			if(ensure==0x00)//库里搜索到指纹，解锁成功
			{
				OLED_Clear();  // 清空OLED显示屏
                OLED_Write_N_Chinese(32, 1, 2, 3);  // 显示"已解锁"的中文字符
                OLED_Write_N_Chinese(56, 4, 1, 1);  // 显示"锁"的中文字符
                Delay_ms(800);  // 延时800毫秒
                OLED_Write_N_Chinese(56, 4, 3, 1);  // 显示"解锁"的中文字符
                Delay_ms(800);  // 延时800毫秒
                OLED_Clear();  // 清空OLED显示屏
			}
			else
			{
				ShowErrorMessage(ensure);
			}
		}
	}
}

//录入指纹
void Add_Finger(u16 FG_ID)
{
	u8 i, ensure, processnum = 0;  // `i` 用于计数，`ensure` 存储函数返回状态码，`processnum` 控制流程步骤
    u16 ID;
	
	while(1)
	{
		switch(processnum)
		{
			case 0:	//录入图像，生成特征，存到CharBuffer1
				i++;
				printf("请按指纹\r\n");
				ensure=PS_GetImage(); // 提示用户按指纹
				if(ensure==0x00)
				{
					ensure=PS_GenChar(CharBuffer1); // 生成特征
					if(ensure==0x00)
					{
						printf("指纹正常\r\n");
						i=0;  //计数清空
						processnum=1;  // 跳到第二步
					}
					else
					{
					ShowErrorMessage(ensure);  // 显示错误信息   
					}
				}
				else
				{
				ShowErrorMessage(ensure);  // 显示错误信息   
				}	
				break;
				
			case 1: //录入图像，生成特征，存到CharBuffer2
				i++;
				printf("请再按一次指纹\r\n");
				ensure=PS_GetImage(); // 提示用户按指纹
				if(ensure==0x00)
				{
					ensure=PS_GenChar(CharBuffer1); // 生成特征
					if(ensure==0x00)
					{
						printf("指纹正常\r\n");
						i=0;  //计数清空
						processnum=2;  // 跳到第二步
					}
					else
					{
					ShowErrorMessage(ensure);  // 显示错误信息   
					}
				}
				else
				{
				ShowErrorMessage(ensure);  // 显示错误信息   
				}	
				break;
				
			case 2: //比较2次录入的指纹特征是否一样
				printf("对比两次指纹\r\n");  // 对比两次采集的指纹特征
				ensure=PS_Match();
				if(ensure==0x00)
				{
					printf("对比一致\r\n");
					processnum=3;  // 跳到第三步
				}
				else
				{
					printf("对比失败，请重新录入指纹\r\n");
                    ShowErrorMessage(ensure);  // 显示错误信息
                    i = 0;
                    processnum = 0;  // 跳回第一步     
				}
				Delay_ms(1200);  // 延时1200毫秒
                break;
			
			case 3: //2次指纹都一样，则合并特征、生成模板
				printf("生成指纹模板\r\n");  // 生成指纹模板
				ensure=PS_RegModel();
				if(ensure==0x00)
				{
					printf("生成指纹模板成功\r\n");
                    processnum = 4;  // 跳到第五步
				}
				else
				{
					processnum = 0;
                    ShowErrorMessage(ensure);  // 显示错误信息
				}
				Delay_ms(1200);  // 延时1200毫秒
                break;
				
			 case 4://存入指纹：将CharBuffer2中的指纹特征数据存入flash数据库中    
                printf("储存指纹ID\r\n");  // 储存指纹ID
                printf("0=< ID <=299\r\n");  // ID 必须在0到299之间
				if(FG_ID<AS608Para.PS_max)
				{
					ID=FG_ID;
				}
				ensure = PS_StoreChar(CharBuffer2, ID);  // 将第二个特征存储为指纹模板
                if (ensure == 0x00)
                {                    
                    printf("录入指纹成功\r\n");
                    OLED_Clear();  // 清空OLED显示屏
                    OLED_Write_N_Chinese(16, 3, 15, 6);  // 显示“录入成功”字样
                    Delay_ms(400);  // 延时400毫秒
                    PS_ValidTempleteNum(&ValidN);  // 读取当前指纹库中的指纹数量
                    return;  // 录入成功后退出函数
                }
                else 
                {
                    processnum = 0;
                    ShowErrorMessage(ensure);  // 显示错误信息
                }                    
                break;
		}
		Delay_ms(1000);  // 延时1000毫秒
		
        if (i == 5)  // 超过5次没有按手指则退出
		{
			printf("超过5次没有按手指则退出\r\n");
            OLED_Clear();  // 清空OLED显示屏
            OLED_Write_N_Chinese(16, 3, 16, 6);  // 显示“录入失败”字样
            FingerNum--;  // 录入失败后指纹数减1
            Delay_ms(100);  // 延时100毫秒
            break;    
		}
	}
}

//删除指纹
void Del_FR(u16 FG_ID)  /* 输入删除指纹ID */
{
    u8 ensure;  // 存储函数返回状态码
    printf("删除指纹\r\n");
    ensure = PS_DeletChar(FG_ID, 1);  // 删除指定ID的单个指纹
    if (ensure == 0)
    {
        printf("删除指纹成功\r\n");        
        OLED_Clear();  // 清空OLED显示屏
        OLED_Write_N_Chinese(16, 3, 19, 6);  // 显示“删除成功”字样
        Delay_ms(400);  // 延时100毫秒
    }
    else
        ShowErrorMessage(ensure);  // 显示错误信息
    
    Delay_ms(1200);  // 延时1200毫秒
    PS_ValidTempleteNum(&ValidN);  // 读取当前指纹库中的指纹数量
}
