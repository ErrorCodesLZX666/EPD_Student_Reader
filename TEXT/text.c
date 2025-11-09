// #include "sys.h"
#include "fontupd.h"
#include "w25qxx.h"
#include "text.h"
#include "string.h"
// #include "EPD_Font.h"
#include "log.h"
#include "malloc.h"
// #include "usart.h"
//////////////////////////////////////////////////////////////////////////////////
// 本程序只供学习使用，未经作者许可，不得用于其它任何用途
// ALIENTEK STM32F407开发板
// 汉字显示 驱动代码
// 正点原子@ALIENTEK
// 技术论坛:www.openedv.com
// 创建日期:2014/5/15
// 版本：V1.0
// 版权所有，盗版必究。
// Copyright(C) 广州市星翼电子科技有限公司 2014-2024
// All rights reserved
//////////////////////////////////////////////////////////////////////////////////

// code 字符指针开始
// 从字库中查找出字模
// code 字符串的开始地址,GBK码
// mat  数据存放地址 (size/8+((size%8)?1:0))*(size) bytes大小
// size:字体大小
void Get_HzMat(unsigned char *code, unsigned char *mat, u8 size)
{
	unsigned char qh, ql;
	unsigned char i;
	unsigned long foffset;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size); // 得到字体一个字符对应点阵集所占的字节数
	// 判断当前code是否是ASCII编码
	if (*code >= 0x20 && *code <= 0x7e)
	{
		// LOGD("ASCII Code is touched!!! Code = 0x%x\r\n", *code);
		// const unsigned char *src = NULL;
		// switch (size)
		// {
		// case 12:
		// 	src = asc2_1206[*code - 32];
		// 	break;
		// case 16:
		// 	src = asc2_1608[*code - 32];
		// 	break;
		// case 24:
		// 	src = asc2_2412[*code - 32];
		// 	break;
		// default:
		// 	// 默认使用16
		// 	src = asc2_1608[*code - 32];
		// 	break;
		// }
		// mymemcpy(mat, src, csize);
		// // 这个字符是ASCII编码，那么就只需要偏倚一位即可。
		// // mat ++;
		// myfree(SRAMIN, src);
	}
	else
	{
		qh = *code;
		ql = *(code + 1);
		// 进入到GBK编码模式代码
		// ql=*code;
		// qh=*(code + 1);
		if (qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff) // 非 常用汉字
		{
			for (i = 0; i < csize; i++)
				*mat++ = 0x00; // 填充满格
			return;			   // 结束访问
		}
		if (ql < 0x7f)
			ql -= 0x40; // 注意!
		else
			ql -= 0x41;
		qh -= 0x81;
		foffset = ((unsigned long)190 * qh + ql) * csize; // 得到字库中的字节偏移量
		switch (size)
		{
		case 12:
			W25QXX_Read(mat, foffset + ftinfo.f12addr, csize);
			break;
		case 16:
			W25QXX_Read(mat, foffset + ftinfo.f16addr, csize);
			break;
		case 24:
			W25QXX_Read(mat, foffset + ftinfo.f24addr, csize);
			break;
		}
	}
}

void Show_Font(u16 x, u16 y, u8 *font, uint16_t fc, uint16_t bc, u8 size, u8 mode)
{
	u8 temp, t, t1;
	u16 y0 = y;
	u8 dzk[72];
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size); // 得到字体一个字符对应点阵集所占的字节数
	if (size != 12 && size != 16 && size != 24)
		return;					// 不支持的size
	Get_HzMat(font, dzk, size); // 得到相应大小的点阵数据
	for (t = 0; t < csize; t++)
	{
		temp = dzk[t]; // 得到点阵数据
		for (t1 = 0; t1 < 8; t1++)
		{
			if (temp & 0x80)
			{
			} // LCD_DrawPoint(x,y,fc);
			else if (mode == 0)
			{
			} // LCD_DrawPoint(x,y,bc);
			temp <<= 1;
			y++;
			if ((y - y0) == size)
			{
				y = y0;
				x++;
				break;
			}
		}
	}
}

void Show_Str(u16 x, u16 y, u8 *str, uint16_t fc, uint16_t bc, u8 size, u8 mode)
{
	u16 x0 = x;
	u16 y0 = y;
	u8 bHz = 0; // 字符或者中文

	if (x > 240 - size)
		return;
	if (y > 280 - size)
		return;

	while (*str != 0) // 数据未结束
	{
		if (!bHz)
		{
			if (*str > 0x80)
				bHz = 1; // 中文
			else		 // 字符
			{
				if (x > (x0 + 240 - size / 2))
					break; // 越界返回
				if (*str == 13)
					break; // 换行符号
				// LCD_ShowChar(x,y,*str,fc,bc,size,mode);
				str++;
				x += size / 2; // 字符,为全字的一半
			}
		}
		else // 中文
		{
			bHz = 0; // 有汉字库
			if (x > (x0 + 240 - size))
				break; // 越界返回
			if (ftinfo.fontok == 0XAA)
			{
				Show_Font(x, y, str, fc, bc, size, mode); // 显示这个汉字,空心显示
			}
			else
			{
				// LCD_ShowChar(x,y,'?',fc,bc,size,mode);
				// LCD_ShowChar(x+(size>>1),y,'?',fc,bc,size,mode);
			}
			str += 2;
			x += size; // 下一个汉字偏移
		}
	}
}

// 在指定位置开始显示一个字符串
// 支持自动换行
//(x,y):起始坐标
// width,height:区域
// str  :字符串
// size :字体大小
// mode:0,非叠加方式;1,叠加方式
// void Show_Str(u16 x,u16 y,u16 width,u16 height,u8*str,u8 size,u8 mode)
//{
//	u16 x0=x;
//	u16 y0=y;
//     u8 bHz=0;     //字符或者中文
//     while(*str!=0)//数据未结束
//     {
//         if(!bHz)
//         {
//	        if(*str>0x80)bHz=1;//中文
//	        else              //字符
//	        {
//                 if(x>(x0+width-size/2))//换行
//				{
//					y+=size;
//					x=x0;
//				}
//		        if(y>(y0+height-size))break;//越界返回
//		        if(*str==13)//换行符号
//		        {
//		            y+=size;
//					x=x0;
//		            str++;
//		        }
//                 else LCD_ShowChar(x,y,*str,LCEDA,WHITE,size,mode);
//		        //else LCD_ShowChar(x,y,*str,size,mode);//有效部分写入
//				str++;
//		        x+=size/2; //字符,为全字的一半
//	        }
//         }else//中文
//         {
//             bHz=0;//有汉字库
//             if(x>(x0+width-size))//换行
//			{
//				y+=size;
//				x=x0;
//			}
//	        if(y>(y0+height-size))break;//越界返回
//	        Show_Font(x,y,str,size,mode); //显示这个汉字,空心显示
//	        str+=2;
//	        x+=size;//下一个汉字偏移
//         }
//     }
// }

// 在指定宽度的中间显示字符串
// 如果字符长度超过了len,则用Show_Str显示
// len:指定要显示的宽度
void Show_Str_Mid(u16 x, u16 y, u8 *str, u8 size, u8 len)
{
	u16 strlenth = 0;
	strlenth = strlen((const char *)str);
	strlenth *= size / 2;
	//	if(strlenth>len)Show_Str(x,y,lcddev.width,lcddev.height,str,size,1);
	//	else
	//	{
	//		strlenth=(len-strlenth)/2;
	//	    Show_Str(strlenth+x,y,lcddev.width,lcddev.height,str,size,1);
	//	}
}
