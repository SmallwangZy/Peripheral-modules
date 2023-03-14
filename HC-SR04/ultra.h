/*
 * ultra.h
 *
 *  Created on: Oct 5, 2022
 *      Author: 31866
 */

#ifndef INC_ULTRA_H_
#define INC_ULTRA_H_

#include "tim.h"
#include "LCD.h"
#define MAX_FILTER 5

extern uint16_t Distance;
extern uint8_t IS_FIRST_CAPTURED;   //是否为第一次捕获（区分上升沿和下降沿）
extern uint32_t IC_Val1;     //上升沿捕获的值
extern uint32_t IC_Val2;     //下降沿捕获的值
extern uint32_t Difference;  //两次捕获的差值
extern uint16_t Distance;    //距离

uint16_t HCSR04_Read(void);
uint16_t HCSR04_Filiter();
uint16_t MAX(uint16_t pdata[],int n);
uint16_t MIN(uint16_t pdata[],int n);
void Read_Distance();

#endif /* INC_ULTRA_H_ */
