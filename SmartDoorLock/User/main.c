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

//指纹模块变量
#define usart2_baund  57600/*串口2波特率，根据指纹模块波特率更改*/
SysPara AS608Para;/*指纹模块AS608的基本参数（结构体）*/
u16 ValidN;/*模块内有效指纹个数*/
u8 FingerNum = 1;
u8 ensure;
void AS608_Init(void);

//按键解锁模块变量
char buf[4] = {'\0'}; // 缓冲区：存放当前已经输入的密码
char admin_password[4] = {'1', '2', '4', '5'}; // 管理员密码
char password[4] = {'1', '2', '3', '4'}; //解锁密码
uint8_t key_value; // 当前按下的按键的键值
uint8_t key_index = 0; // 当前输入第几位密码了
uint8_t key_lock=0;//key_lock=1，可以修改密码
void Password_unlock(void);
u8 Offset;
volatile int admin_unlocked = 0; // 是否进入管理员模式
volatile int password_unlocked = 0;//是否解锁
void delate_finger_oled(void);
void add_finger_oled(void);

//RC522模块变量
uint8_t Card_Type1[2];//卡类型
uint8_t Card_ID[4];//在主页面解锁时扫描到的卡ID号
uint8_t Stored_Card_ID[4];//已存储的卡片ID号
uint8_t Card_KEY[6] = {0xff,0xff,0xff,0xff,0xff,0xff};//{0x11,0x11,0x11,0x11,0x11,0x11};   //密钥
uint8_t Card_Data[16];//读取到的块地址里面的数据
uint8_t status;//RFID射频卡操作的执行状态
static uint32_t ReadCard_Time=0;//当前读卡已用的时间，超时退出
static uint8_t ReadCard_Flag=0;
#define MAX_ALLOWED_CARDS 10  // 设定最大允许的卡片数量
uint8_t allowed_cards[MAX_ALLOWED_CARDS][4] = {
    {0x12, 0x34, 0x56, 0x78},  // 示例卡片1
    {0x87, 0x65, 0x43, 0x21},  // 示例卡片2
};
int num_allowed_cards = 2;  // 当前卡片数量

int is_card_allowed(uint8_t card_id[4]) 
{
    for (int i = 0; i < num_allowed_cards; i++) 
	{
        if (memcmp(card_id, allowed_cards[i], 4) == 0) 
		{
            return 1; // 卡片被允许
        }
    }
    return 0; // 卡片不被允许
}

//蓝牙模块变量
extern u8 res;
extern u8 res_flag;

void Show_Main_Menu(void);
void Show_Admin_Menu(void);
void Add_Card(void);

int main(void)
{
    Serial_Init();  // 初始化串口
	RC522_Init();
	delay_init();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
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
		// 主页面显示
        OLED_Clear();
		//显示汉字：智能门禁系统；
		OLED_Write_N_Chinese(16, 1, 0, 6);
		//显示锁
		OLED_Write_N_Chinese(56, 4, 1, 1);
		OLED_ShowString(0, 7, "Back", 6);

        // 等待按键按下
        while(1)
        {
            int key_value = Key_Scan();
			/*
			1    2    3    CLR
			4    5    6    UP
			BAK  0    SEE  OK
			7    8    9    DOWN
            */
            // 在主页面按下BACK，则熄屏
            if (key_value == BACK)
            {
                OLED_Clear();
                Delay_ms(100);
            }
			//1.密码解锁
            // 按下按键0进入密码解锁页面
            if (key_value == 0)
            {
                Password_unlock();
                if (admin_unlocked) // 管理员密码解锁
                {
                    admin_unlocked = 0;  // 重置解锁状态
					OLED_Write_N_Chinese(32, 1, 2, 3); // 显示已解锁
					Delay_ms(500);
                    Show_Admin_Menu(); // 跳转到管理员页面
                    return;
                }
				if(password_unlocked)
				{
					OLED_Write_N_Chinese(32, 1, 2, 3); // 显示已解锁
					Delay_ms(5000);
					return;
				}
            }
			
			//2.卡片解锁
			if (MI_OK == PcdRequest(0x52, Card_Type1))  // 寻卡函数，如果成功返回MI_OK 
			{
				if (ReadCard_Flag == 0) 
				{
					ReadCard_Flag = 1;  // 置为1，表示已经检测到卡片，开始处理
					ReadCard_Time = 0;  // 重置读卡时间计数
				}
				
				OLED_Clear();
				OLED_Write_N_Chinese(32, 2, 5, 5);  //显示：读取卡片中
				Delay_ms(500);
				
				status = PcdAnticoll(Card_ID); // 防冲撞 如果成功返回MI_OK
				if (status == MI_OK) 
				{
					ReadCard_Flag=0;//读卡操作完成，清除读卡标志位
					// 验证卡片是否被允许
					if (is_card_allowed(Card_ID)) 
					{
						OLED_Clear();
						OLED_Write_N_Chinese(32, 1, 2, 3); // 显示已解锁
						Delay_ms(5000);
						printf("Card allowed. Unlocking...\r\n");
						return;
					} 
					else 
					{
						printf("Card not allowed. Returning to main screen.\r\n");
						return;  // 卡片不被允许，返回主界面
					}
				} 
				else 
				{
					ReadCard_Flag=0;//读卡即将退出，清除读卡标志位
					printf("Anticoll Error\r\n");
					return;  // 卡片读取失败，返回主界面
				}
				
			}
			else if (ReadCard_Time >= 10000000) 
			{
				ReadCard_Flag = 0;  // 超时，清除标志位
				ReadCard_Time = 0;  // 重置读卡时间
			}
			
			//3.蓝牙解锁
			//按1进入蓝牙解锁
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
						OLED_Write_N_Chinese(32, 1, 2, 3); // 显示已解锁
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
			
			//4.指纹解锁
			if(PS_Sta)////读指纹模块状态引脚,查看是否有指纹触摸
			{
				OLED_Clear();
				OLED_Write_N_Chinese(32,2,4,5);	
				Delay_ms(500);
				while(1)
				{
					u8 ensure;
					SearchResult result;
					//录入指纹图像
					ensure=PS_GetImage();
					if(ensure==0x00)
					{
						//生成特征：这里选择CharBuffer1
						ensure=PS_GenChar(CharBuffer1);
						if(ensure==0x00)//成功
						{
							//高速搜索/比对
							ensure=PS_HighSpeedSearch(CharBuffer1,0,AS608Para.PS_max,&result);
							if(ensure==0x00)//成功
							{
								//显示已解锁
								OLED_Clear();  // 清空OLED显示屏
								OLED_Write_N_Chinese(32, 1, 2, 3); // 显示已解锁
								Delay_ms(5000);
								return;
							}
							else
							{
								ShowErrorMessage(ensure);//打印错误信息
								break;//有错误，跳出循环
							}
						}
						else
						{
							ShowErrorMessage(ensure);//打印错误信息
							break;//有错误，跳出循环
						}
					}
					else
					{
						ShowErrorMessage(ensure);//打印错误信息
						break;//有错误，跳出循环
					}
				}
				return;//不管什么原因退出，都回到主界面
			}
			
            Delay_ms(100);
        }
    }
}

void Show_Admin_Menu(void)
{
    // 管理页面逻辑
    int Offset = 0;
    OLED_Clear();
    OLED_ShowString(8, Offset, "->", 8);
    OLED_Write_N_Chinese(32, 0, 7, 4);//修改密码
    OLED_Write_N_Chinese(32, 2, 8, 4);//添加指纹
    OLED_Write_N_Chinese(32, 4, 9, 4);//删除指纹
    OLED_Write_N_Chinese(32, 6, 10, 4);//添加卡片

    while(1)
    {
        int key_value = Key_Scan();
        if (key_value == BACK)
        {
            return;  // 返回主菜单
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
				case 0://修改密码
					key_lock=1;//置key_lock为1，表示可以修改密码
					Password_unlock();//输入新的密码
					if (key_lock == 0) // 修改密码完成后
					{
						OLED_Clear();
						Delay_ms(2000); // 显示2秒
						Show_Admin_Menu(); // 返回管理员页面
					}
					break;
				case 2://添加指纹
					FingerNum++;
					add_finger_oled();
					Add_Finger(FingerNum);
					Show_Admin_Menu();
				case 4://删除最新录入的一个指纹
					delate_finger_oled();
					Del_FR(FingerNum);
					FingerNum--;
					Show_Admin_Menu();
				case 6://添加卡片
					Add_Card();Show_Admin_Menu(); break; // 添加卡片选项	
			}
        }
    }
}


void Password_unlock(void)
{
    key_index = 0;
    memset(buf, '\0', 4);

    OLED_Clear();
    OLED_Write_N_Chinese(0, 0, 6, 2); // 显示解锁提示
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
                OLED_ShowChar(x + 19, 2, '_', 6);//下一个密码位
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
            return; // 返回主页面
        }
        else if (key_value == ACK)
        {
            if (strncmp(buf, admin_password, 4) == 0)//密码正确
            {
                OLED_Clear();
                OLED_Write_N_Chinese(32, 1, 2, 3); // 显示已解锁
                Delay_ms(1000);
                admin_unlocked = 1;  // 设置解锁状态
                return; // 返回主页面
            }
			else if(strncmp(buf, password, 4) == 0)//密码正确
            {
                OLED_Clear();
                OLED_Write_N_Chinese(32, 1, 2, 3); // 显示已解锁
                Delay_ms(1000);
                password_unlocked = 1;  // 设置解锁状态
                return; // 返回主页面
            }
            else//密码错误
            {
				//提示密码错误
                OLED_Clear();
				OLED_ShowString(8,3,"Password error",8);
                Delay_ms(1000);
                memset(buf, '\0', 4);
                key_index = 0;
				//返回解锁页面重新输入密码
                OLED_Clear();
                OLED_Write_N_Chinese(0, 0, 6, 2); // 显示解锁提示
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

//添加卡片
void Add_Card(void)
{
    uint8_t new_card_id[4];//添加的卡片的ID号
	 uint32_t timeout = 10;  // 设置超时限制
	uint32_t elapsed_time = 0;  // 用于记录已经经过的时间
	
	OLED_Clear();
	OLED_Write_N_Chinese(32,2,5,5);//显示：读取卡片中
	
	//读取卡片ID
	while(1)
	{
		//寻卡
		if(PcdRequest(0x52,Card_Type1)==MI_OK)
		{
			//通过防冲突操作，返回卡ID
			if(PcdAnticoll(new_card_id)==MI_OK)
			{
				printf("Serial Number:%d %d %d %d\r\n",Card_ID[0],Card_ID[1],Card_ID[2],Card_ID[3]);
				//判断卡牌是否已存在
				if(is_card_allowed(new_card_id))
				{
					OLED_Clear();
					OLED_Write_N_Chinese(32,2,20,5);//显示：卡片已存在
					Delay_ms(500);
				}
				else
				{
					//判断卡片容量是否达到上限
					if(num_allowed_cards<MAX_ALLOWED_CARDS)
					{
						//添加卡片到列表
						memcpy(allowed_cards[num_allowed_cards],new_card_id,4);
						num_allowed_cards++;//已允许的卡片数量加1	
						OLED_Clear();
						OLED_Write_N_Chinese(24,2,21,6);//显示：卡片添加成功
						Delay_ms(500);
					}
					else
					{
						OLED_Clear();
						OLED_Write_N_Chinese(24,2,22,6);//显示：卡片容量已满
						Delay_ms(500);
					}
				}
				//读卡成功后返回
				return;
			}
			
			Delay_ms(100);//每次循环延时100ms
			elapsed_time++;// 增加已用时间
			//判断是否超时，超时则退出：timeout=50，最多延时50次
			if(elapsed_time>=timeout)
			{
				OLED_Clear();
				OLED_Write_N_Chinese(24,2,23,6);//显示：读取卡片超时
				return;
			}
		}
	}
}

//2个显示函数：
void delate_finger_oled(void)//删指纹显示函数
{
	OLED_Clear();
	OLED_Write_N_Chinese(32,2,18,5);	
	Delay_ms(500);
}

void add_finger_oled(void)//录指纹显示函数
{
	OLED_Clear();
	OLED_Write_N_Chinese(32,2,17,5);	
	Delay_ms(500);
}

void AS608_Init(void)
{
	printf("初始化中\r\n");
	usart2_init(usart2_baund);	/*初始化串口2,用于与指纹模块通讯*/
	AS608_IO_Init();					/*初始化FR读状态引脚*/
	printf("与指纹模块握手\r\n");
	while(PS_HandShake(&AS608Addr))			/*与AS608模块握手*/
	{
		Delay_ms(400);
		printf("未检测到模块\r\n");
		Delay_ms(800);
		printf("尝试重新连接模块\r\n"); 
	}
	printf("通讯成功\r\n");
	printf("波特率:%d   地址:%x\r\n",usart2_baund,AS608Addr);		/*打印信息*/
	ensure=PS_ValidTempleteNum(&ValidN);										/*读库指纹个数*/
	if(ensure!=0x00)
		ShowErrorMessage(ensure);								/*显示确认码错误信息*/
	ensure=PS_ReadSysPara(&AS608Para);  		/*读参数 */
	if(ensure==0x00)
	{
		printf("库容量:%d     对比等级: %d",AS608Para.PS_max-ValidN,AS608Para.PS_level);
	}
	else
		ShowErrorMessage(ensure);	
}
