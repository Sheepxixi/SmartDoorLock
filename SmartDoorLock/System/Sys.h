#ifndef __SYS_H
#define __SYS_H	
#include "stm32f10x.h"


//0,不支持ucos
//1,支持ucos
#define SYSTEM_SUPPORT_OS		0		//定义系统文件夹是否支持UCOS
																	    
	 
//位带操作,实现51类似的GPIO控制功能
//具体实现思想,参考<<CM3权威指南>>第五章(87页~92页).
//IO口操作宏定义

/*
为什么要使用位带别名区？
stm32会一次性访问32位（4个字节，一个字）的寄存器(内存)地址，之前要修改寄存器的一个位的话，
要先将寄存器一个字的内容读出来，再修改对应的位，最后再把修改后一个字的内容写回寄存器；
而使用位带别名区，位段的一个位地址映射成了别名区的一个字地址（32位，4个字节），
操作这个字地址的内容就是操作了这个位，从而可以对位进行操作。

支持位带操作的区域是SRAM区的最低1MB范围（APB1/2，AHB1外设）和片内外设区的最低1MB范围。
也就是0x20000000 - 0x200F FFFF、0x40000000 - 0x400FFFFF；为什么是00000-FFFFF？
因为上面这些是字节地址，比如0x2000 0000就代表了一个字节的内存的地址，1MB=2^20B,20位二进制数相当于5位16进制数，
0x20000000 到 0x200FFFFF 和 0x40000000 到 0x400FFFFF 每个地址范围正好包含 1MB 的内存区域。

对于SRAM/GPIO位带区的某个比特(bit)，记它所在的字节地址为A，
位序号为n(0<=n<=7)(如果是GPIO则n<=16)，则该比特在位带别名区中的地址为：

AliasAddr=0x22000000+( (A-0x20000000) * 8 + n )*4  = 0x0200 0000+32A+4n
解释：
0x22000000					：是位带别名区的起始地址；
0x20000000					：是位带区的起始地址（SRAM 和 GPIO 的起始地址）；
A-0x20000000				：是你要操作的字节地址与位带区起始地址的地址偏移，
(A-0x20000000)*8			：因为是字节地址，所以要*8；得到偏移了几位
(A-0x20000000)*8+n			：位偏移
( (A-0x20000000)*8+n )*4	：每个位在位带别名区中都占用 4 字节，所以要乘以 4。



*/
//获得该位在位带别名区的地址：addr为字节地址，bitnum为第几位
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
//将给定的地址转换为一个指向 volatile unsigned long 类型的指针，并读取该地址的值
//(volatile unsigned long  *):指针类型转换，最外面一个*是解引用
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
//前面2条结合：
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 
//IO口地址映射

//GPIO口输出数据寄存器
#define GPIOA_ODR_Addr    (GPIOA_BASE+12) //0x4001080C 
#define GPIOB_ODR_Addr    (GPIOB_BASE+12) //0x40010C0C 
#define GPIOC_ODR_Addr    (GPIOC_BASE+12) //0x4001100C 
#define GPIOD_ODR_Addr    (GPIOD_BASE+12) //0x4001140C 
#define GPIOE_ODR_Addr    (GPIOE_BASE+12) //0x4001180C 
#define GPIOF_ODR_Addr    (GPIOF_BASE+12) //0x40011A0C    
#define GPIOG_ODR_Addr    (GPIOG_BASE+12) //0x40011E0C    
//输出数据寄存器的值
#define GPIOA_IDR_Addr    (GPIOA_BASE+8) //0x40010808 
#define GPIOB_IDR_Addr    (GPIOB_BASE+8) //0x40010C08 
#define GPIOC_IDR_Addr    (GPIOC_BASE+8) //0x40011008 
#define GPIOD_IDR_Addr    (GPIOD_BASE+8) //0x40011408 
#define GPIOE_IDR_Addr    (GPIOE_BASE+8) //0x40011808 
#define GPIOF_IDR_Addr    (GPIOF_BASE+8) //0x40011A08 
#define GPIOG_IDR_Addr    (GPIOG_BASE+8) //0x40011E08 
 
//IO口操作,只对单一的IO口!
//确保n的值小于16!
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //读取输出数据寄存器某个位的值，输出模式下使用
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  //读取输入数据寄存器某个位的值，输入模式下使用

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)  //输出 
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)  //输入 

#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  //输出 
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)  //输入 

#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)  //输出 
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)  //输入 

#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)  //输出 
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)  //输入

#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)  //输出 
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)  //输入

#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  //输出 
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  //输入

//以下为汇编函数
void WFI_SET(void);		//执行WFI指令
void INTX_DISABLE(void);//关闭所有中断
void INTX_ENABLE(void);	//开启所有中断
void MSR_MSP(u32 addr);	//设置堆栈地址

#endif
