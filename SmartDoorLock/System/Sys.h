#ifndef __SYS_H
#define __SYS_H	
#include "stm32f10x.h"


//0,��֧��ucos
//1,֧��ucos
#define SYSTEM_SUPPORT_OS		0		//����ϵͳ�ļ����Ƿ�֧��UCOS
																	    
	 
//λ������,ʵ��51���Ƶ�GPIO���ƹ���
//����ʵ��˼��,�ο�<<CM3Ȩ��ָ��>>������(87ҳ~92ҳ).
//IO�ڲ����궨��

/*
ΪʲôҪʹ��λ����������
stm32��һ���Է���32λ��4���ֽڣ�һ���֣��ļĴ���(�ڴ�)��ַ��֮ǰҪ�޸ļĴ�����һ��λ�Ļ���
Ҫ�Ƚ��Ĵ���һ���ֵ����ݶ����������޸Ķ�Ӧ��λ������ٰ��޸ĺ�һ���ֵ�����д�ؼĴ�����
��ʹ��λ����������λ�ε�һ��λ��ַӳ����˱�������һ���ֵ�ַ��32λ��4���ֽڣ���
��������ֵ�ַ�����ݾ��ǲ��������λ���Ӷ����Զ�λ���в�����

֧��λ��������������SRAM�������1MB��Χ��APB1/2��AHB1���裩��Ƭ�������������1MB��Χ��
Ҳ����0x20000000 - 0x200F FFFF��0x40000000 - 0x400FFFFF��Ϊʲô��00000-FFFFF��
��Ϊ������Щ���ֽڵ�ַ������0x2000 0000�ʹ�����һ���ֽڵ��ڴ�ĵ�ַ��1MB=2^20B,20λ���������൱��5λ16��������
0x20000000 �� 0x200FFFFF �� 0x40000000 �� 0x400FFFFF ÿ����ַ��Χ���ð��� 1MB ���ڴ�����

����SRAM/GPIOλ������ĳ������(bit)���������ڵ��ֽڵ�ַΪA��
λ���Ϊn(0<=n<=7)(�����GPIO��n<=16)����ñ�����λ���������еĵ�ַΪ��

AliasAddr=0x22000000+( (A-0x20000000) * 8 + n )*4  = 0x0200 0000+32A+4n
���ͣ�
0x22000000					����λ������������ʼ��ַ��
0x20000000					����λ��������ʼ��ַ��SRAM �� GPIO ����ʼ��ַ����
A-0x20000000				������Ҫ�������ֽڵ�ַ��λ������ʼ��ַ�ĵ�ַƫ�ƣ�
(A-0x20000000)*8			����Ϊ���ֽڵ�ַ������Ҫ*8���õ�ƫ���˼�λ
(A-0x20000000)*8+n			��λƫ��
( (A-0x20000000)*8+n )*4	��ÿ��λ��λ���������ж�ռ�� 4 �ֽڣ�����Ҫ���� 4��



*/
//��ø�λ��λ���������ĵ�ַ��addrΪ�ֽڵ�ַ��bitnumΪ�ڼ�λ
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
//�������ĵ�ַת��Ϊһ��ָ�� volatile unsigned long ���͵�ָ�룬����ȡ�õ�ַ��ֵ
//(volatile unsigned long  *):ָ������ת����������һ��*�ǽ�����
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
//ǰ��2����ϣ�
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 
//IO�ڵ�ַӳ��

//GPIO��������ݼĴ���
#define GPIOA_ODR_Addr    (GPIOA_BASE+12) //0x4001080C 
#define GPIOB_ODR_Addr    (GPIOB_BASE+12) //0x40010C0C 
#define GPIOC_ODR_Addr    (GPIOC_BASE+12) //0x4001100C 
#define GPIOD_ODR_Addr    (GPIOD_BASE+12) //0x4001140C 
#define GPIOE_ODR_Addr    (GPIOE_BASE+12) //0x4001180C 
#define GPIOF_ODR_Addr    (GPIOF_BASE+12) //0x40011A0C    
#define GPIOG_ODR_Addr    (GPIOG_BASE+12) //0x40011E0C    
//������ݼĴ�����ֵ
#define GPIOA_IDR_Addr    (GPIOA_BASE+8) //0x40010808 
#define GPIOB_IDR_Addr    (GPIOB_BASE+8) //0x40010C08 
#define GPIOC_IDR_Addr    (GPIOC_BASE+8) //0x40011008 
#define GPIOD_IDR_Addr    (GPIOD_BASE+8) //0x40011408 
#define GPIOE_IDR_Addr    (GPIOE_BASE+8) //0x40011808 
#define GPIOF_IDR_Addr    (GPIOF_BASE+8) //0x40011A08 
#define GPIOG_IDR_Addr    (GPIOG_BASE+8) //0x40011E08 
 
//IO�ڲ���,ֻ�Ե�һ��IO��!
//ȷ��n��ֵС��16!
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //��ȡ������ݼĴ���ĳ��λ��ֵ�����ģʽ��ʹ��
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  //��ȡ�������ݼĴ���ĳ��λ��ֵ������ģʽ��ʹ��

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)  //��� 
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)  //���� 

#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  //��� 
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)  //���� 

#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)  //��� 
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)  //���� 

#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)  //��� 
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)  //����

#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)  //��� 
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)  //����

#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  //��� 
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  //����

//����Ϊ��ຯ��
void WFI_SET(void);		//ִ��WFIָ��
void INTX_DISABLE(void);//�ر������ж�
void INTX_ENABLE(void);	//���������ж�
void MSR_MSP(u32 addr);	//���ö�ջ��ַ

#endif
