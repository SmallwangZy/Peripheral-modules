/*
 * ultra.c
 *
 *  Created on: Oct 5, 2022
 *      Author: 31866
 */

#include "ultra.h"

uint8_t IS_FIRST_CAPTURED = 0;   //是否为第一次捕获（区分上升沿和下降沿）
uint32_t IC_Val1=0;     //上升沿捕获的值
uint32_t IC_Val2=0;     //下降沿捕获的值
uint32_t Difference=0;  //两次捕获的差值
uint16_t Distance=0;    //距离
uint16_t Buff[MAX_FILTER]={0};

uint16_t ThreeFlag=0; //等于0说明上一次数据是两位；等于1说明上一次是三位

uint8_t i;


/*****有两种中断，一种是捕获中断，一种是溢出中断*****/

//读取HCSR04的值
uint16_t HCSR04_Read(void)
{
	HAL_GPIO_WritePin(TRIG_GPIO_Port ,TRIG_Pin ,0);  //先拉低
	Delay_us (10);    //缓冲
	HAL_GPIO_WritePin(TRIG_GPIO_Port ,TRIG_Pin ,1);  //给一个高电平
	Delay_us (12);   //持续10us以上的高电平
	HAL_GPIO_WritePin(TRIG_GPIO_Port ,TRIG_Pin ,0);	//恢复为低电平
	HAL_TIM_IC_Start_IT(&htim3,TIM_CHANNEL_1);  //再次使能TIM3的输入捕获中断
	HAL_Delay (100);          //延迟100ms作于缓冲。
	return Distance ;
}

uint16_t HCSR04_Filiter()  //采用均值滤波
{
	uint16_t sum=0;
	for(i=0;i<MAX_FILTER;i++)
	{
		HCSR04_Read();
		Buff[i]=Distance;
		sum+=Distance;       //累加求总和
	}
	sum=sum-MAX(Buff,MAX_FILTER)-MIN(Buff,MAX_FILTER);   //去掉最大值和最小值
	return sum/(MAX_FILTER-2);     //求去掉峰值以后的平均值
}

uint16_t MAX(uint16_t pdata[],int n)  //寻找一个数组中的最大值
{
	uint16_t max=0;
	for(i=0;i<n;i++)
	{
		if(pdata[i]>max)
			max=pdata[i];
	}
	return max;
}

uint16_t MIN(uint16_t pdata[],int n)  //寻找一个数组中的最小值。
{
	uint16_t min=0xffff;
	for(i=0;i<n;i++)
	{
		if(pdata[i]<min)
			min=pdata[i];
	}
	return min;
}

void Read_Distance()
{
     HCSR04_Read();


}
