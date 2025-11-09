#ifndef __FONTUPD_H__
#define __FONTUPD_H__	 
//#include "sys.h"
#include "gd32f4xx.h"
#include "systick.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//字库更新 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2017/5/2
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	 
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

//字体信息保存地址,占33个字节,第1个字节用于标记字库是否存在.后续每8个字节一组,分别保存起始地址和文件大小														   
extern u32 FONTINFOADDR;	





//字库信息结构体定义
//用来保存字库基本信息，地址，大小等
typedef struct __attribute__((packed))  
{
    uint8_t fontok;         // 字库存在标志，0xAA表示字库正常
    uint32_t ugbkaddr;      // unigbk的地址
    uint32_t ugbksize;      // unigbk的大小
    uint32_t f12addr;       // gbk12地址
    uint32_t gbk12size;     // gbk12的大小
    uint32_t f16addr;       // gbk16地址
    uint32_t gbk16size;     // gbk16的大小
    uint32_t f24addr;       // gbk24地址
    uint32_t gkb24size;     // gbk24的大小
} _font_info;


extern _font_info ftinfo;	//字库信息结构体

u32 fupd_prog(u16 x,u16 y,u8 size,u32 fsize,u32 pos);	//显示更新进度
u8 updata_fontx(u16 x,u16 y,u8 size,u8 *fxpath,u8 fx);	//更新指定字库
u8 update_font(u16 x,u16 y,u8 size,u8* src);			//更新全部字库
u8 font_init(void);										//初始化字库
#endif





















