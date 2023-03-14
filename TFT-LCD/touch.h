/*
 * touch.h
 *
 *  Created on: Sep 30, 2022
 *      Author: 31866
 */

#ifndef INC_TOUCH_H_
#define INC_TOUCH_H_
/********本次使用的是软件模拟SPI通信********/
#include "tim.h"
#include "gpio.h"
#include "LCD.h"
#include "24cxx.h"
#include "math.h"
#include "stdio.h"
#include "main.h"
#include "openmv.h"
#include "ultra.h"



#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t


#define TP_PRES_DOWN 0x80  		//是否有触摸屏按键被按下
#define TP_CATH_PRES 0x40  		//有按键按下了

typedef struct
{

	u8 (*init)(void);			//初始化触摸屏控制器
	u8 (*scan)(u8);				//扫描触摸屏.0,屏幕扫描;1,物理坐标;
	void (*adjust)(void);		//触摸屏校准
	u16 x0;						//原始坐标(第一次按下时的坐标)
	u16 y0;
	u16 x; 						//当前坐标(此次扫描时,触屏的坐标)
	u16 y;
	u8  sta;					//笔的状态
									//b7:按下1/松开0;
		                            //b6:0,没有按键按下;1,有按键按下.
	//电压与坐标的关系，一次函数
	float xfac;
	float yfac;
	short xoff;
	short yoff;

	//新增的参数，当触摸屏的左右上下完全颠倒时需要用到
	//touchtype=0,适合左右为X坐标，上下为Y坐标
	//touchtype=1,适合左右为Y坐标，上下为X坐标
	u8 touchtype;
}_n_tp_dev;

extern _n_tp_dev tp_dev;
extern uint8_t DuoGiFlag;

#define PEN_SET HAL_GPIO_WritePin(T_PEN_GPIO_Port,T_PEN_Pin,1)
#define PEN_CLR HAL_GPIO_WritePin(T_PEN_GPIO_Port,T_PEN_Pin,0)



#define TDIN_SET HAL_GPIO_WritePin(T_MOSI_GPIO_Port,T_MOSI_Pin,1)
#define TDIN_CLR HAL_GPIO_WritePin(T_MOSI_GPIO_Port,T_MOSI_Pin,0)

#define TCLK_SET HAL_GPIO_WritePin(T_CLK_GPIO_Port,T_CLK_Pin,1)
#define TCLK_CLR HAL_GPIO_WritePin(T_CLK_GPIO_Port,T_CLK_Pin,0)

#define TCS_SET HAL_GPIO_WritePin(T_CS_GPIO_Port,T_CS_Pin,1)
#define TCS_CLR HAL_GPIO_WritePin(T_CS_GPIO_Port,T_CS_Pin,0)


void Tp_Write_Byte(u8 num);
u16 TP_Read_AD(u8 CMD);
u16 TP_Read_XOY(u8 xy);
u8 TP_Read_XY(u16 *x,u16 *y);
u8 TP_Read_XY2(u16 *x,u16 *y);
u8 TP_Scan(u8 tp);
u8 TP_Init(void);
void TP_Save_Adjdata(void);
u8 TP_Get_Adjdata(void);
void TP_Adjust(void);
void TP_Drow_Touch_Point(u16 x,u16 y,u16 color);
void rtp_test(void);
void TP_Adj_Info_Show(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac);
void TouchChoose();
void Stop(uint16_t angle);



#endif /* INC_TOUCH_H_ */
