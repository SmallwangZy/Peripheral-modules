/*
 * touch.c
 *
 *  Created on: Sep 30, 2022
 *      Author: 31866
 */

#include "touch.h"

uint8_t CMD_RDX=0xD0;   //读取x坐标
uint8_t CMD_RDY=0x90;   //读取y坐标

uint8_t DuoGiFlag=0;  //设定初始值,为0说明要增长，为1说明要下降

_n_tp_dev tp_dev=
{
	TP_Init,
	TP_Scan,
	TP_Adjust,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};



//SPI写数据
void Tp_Write_Byte(u8 num)
{
	u8 count=0;
	for(count=0;count<8;count++)
	{
		if(num&0x80)   //如果num的第一位为1
			TDIN_SET;
		else
			TDIN_CLR;
		num<<=1;  //循环左移
		TCLK_CLR;
		Delay_us(1);
		TCLK_SET;   //上升沿有效
	}
}

//SPI读数据
u16 TP_Read_AD(u8 CMD)
{
	u8 count=0;
	u16 Num=0;
	TCLK_CLR;
	TDIN_CLR;
	TCS_CLR;   //拉低片选
	Tp_Write_Byte(CMD);
	Delay_us(6);
	TCLK_CLR;
	Delay_us(1);
	TCLK_SET;
	Delay_us(1);
	TCLK_CLR;
	for(count=0;count<16;count++)//读出16位数据,只有高12位有效
		{
			Num<<=1;
			TCLK_CLR;	//下降沿有效
			Delay_us(1);
			TCLK_SET;
			if(HAL_GPIO_ReadPin(T_MISO_GPIO_Port,T_MISO_Pin)==1)Num++;
		}
		Num>>=4;   	//只有高12位有效
		TCS_SET;		//释放片选
		return(Num);
}

//读取一个坐标值(x或者y)
//连续读取READ_TIMES次数据,对这些数据升序排列,
//然后去掉最低和最高LOST_VAL个数,取平均值(滤波)
//xy:指令（CMD_RDX/CMD_RDY）
//返回值:读到的数据
#define READ_TIMES 5 	//读取次数
#define LOST_VAL 1	  	//丢弃值
u16 TP_Read_XOY(u8 xy)
{
	u16 i, j;
	u16 buf[READ_TIMES];
	u16 sum=0;   //总和用于求平均值
	u16 temp;
	for(i=0;i<READ_TIMES;i++)
	{
		buf[i]=TP_Read_AD(xy);
	}
	for(i=0;i<READ_TIMES-1; i++)//排序
	{
		for(j=i+1;j<READ_TIMES;j++)
		{
			if(buf[i]>buf[j])//升序排列
			{
				temp=buf[i];
				buf[i]=buf[j];
				buf[j]=temp;
			}
		}
	}
	sum=0;
	for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];  //去掉了最高值和最低值
	temp=sum/(READ_TIMES-2*LOST_VAL);    //求出平均值
	return temp;
}
//读取x,y坐标
//最小值不能少于100.
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
u8 TP_Read_XY(u16 *x,u16 *y)
{
	u16 xtemp,ytemp;
	xtemp=TP_Read_XOY(CMD_RDX);    //读取X的坐标
	ytemp=TP_Read_XOY(CMD_RDY);    //读取Y的坐标
	//if(xtemp<100||ytemp<100)return 0;//读数失败
	*x=xtemp;    //传值给x，y
	*y=ytemp;
	return 1;//读数成功
}
//连续2次读取触摸屏IC,且这两次的偏差不能超过
//ERR_RANGE,满足条件,则认为读数正确,否则读数错误.
//该函数能大大提高准确度
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
#define ERR_RANGE 50 //误差范围
u8 TP_Read_XY2(u16 *x,u16 *y)
{
	u16 x1,y1;
 	u16 x2,y2;
 	u8 flag;
    flag=TP_Read_XY(&x1,&y1);
    if(flag==0)return(0);
    flag=TP_Read_XY(&x2,&y2);
    if(flag==0)return(0);
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//前后两次采样在+-50内
    &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
//        printf("Successes!");
        return 1;
    }
    else
    {
//    	printf("failed!");
    	return 0;
    }
}

//
//触摸按键扫描
//tp:0,屏幕坐标;1,物理坐标(校准等特殊场合用)
//返回值:当前触屏状态.
//0,触屏无触摸;1,触屏有触摸
u8 TP_Scan(u8 tp)
{
	if(HAL_GPIO_ReadPin(T_PEN_GPIO_Port,T_PEN_Pin)==0)    //pen是否按下
	{
		if(tp)TP_Read_XY2(&tp_dev.x,&tp_dev.y);//读取物理坐标
		else if(TP_Read_XY2(&tp_dev.x,&tp_dev.y))//读取屏幕坐标
		{
			printf("X:%d\n",tp_dev.x);
			printf("Y:%d\n",tp_dev.y);
//	 		tp_dev.x=tp_dev.xfac*tp_dev.x+tp_dev.xoff;//将结果转换为屏幕坐标
//			tp_dev.y=tp_dev.yfac*tp_dev.y+tp_dev.yoff;
			//这个是校准得到的参数。
//	 		tp_dev.x=-0.0865800902*tp_dev.x+0x0154;//将结果转换为屏幕坐标
//			tp_dev.y=-0.123978585*tp_dev.y+0x01ED;
	 	}
		if((tp_dev.sta&TP_PRES_DOWN)==0)//之前没有被按下
		{
			tp_dev.sta=TP_PRES_DOWN|TP_CATH_PRES;//按键按下
			tp_dev.x0=tp_dev.x;//记录第一次按下时的坐标
			tp_dev.y0=tp_dev.y;
		}
	}else   //没有被按下
	{
		if(tp_dev.sta&TP_PRES_DOWN)//之前是被按下的
		{
			tp_dev.sta&=~(1<<7);//标记按键松开,防止按过一次就失效了。
		}else//之前就没有被按下
		{
			tp_dev.x0=0;
			tp_dev.y0=0;
			tp_dev.x=0xffff;
			tp_dev.y=0xffff;
		}
	}
	return tp_dev.sta&TP_PRES_DOWN;//返回当前的触屏状态
}

#define SAVE_ADDR_BASE 40    //EEPROM里面的地址区间基地址
//保存校准的参数
void TP_Save_Adjdata(void)
{
	HAL_AT24CXX_WriteLenByte(SAVE_ADDR_BASE,(u8*)&tp_dev.xfac,14);	//强制保存&tp_dev.xfac地址开始的14个字节数据，即保存到tp_dev.touchtype
	HAL_AT24CXX_WriteOneByte(SAVE_ADDR_BASE+14,0X0A);		//写0X0A标记校准过了已经
}
//µÃµ½±£´æÔÚEEPROMÀïÃæµÄÐ£×¼Öµ
//·µ»ØÖµ£º1£¬³É¹¦»ñÈ¡Êý¾Ý
//        0£¬»ñÈ¡Ê§°Ü£¬ÒªÖØÐÂÐ£×¼
u8 TP_Get_Adjdata(void)
{
	u8 temp;
	temp=HAL_AT24CXX_ReadOneByte(SAVE_ADDR_BASE+14);//¶ÁÈ¡±ê¼Ç×Ö,¿´ÊÇ·ñÐ£×¼¹ý£¡
	if(temp==0X0A)    //触摸屏已经校准过了
 	{
		HAL_AT24CXX_ReadLenByte(SAVE_ADDR_BASE,(u8*)&tp_dev.xfac,14);//读取之前保存的校准数？
		if(tp_dev.touchtype)//X,Y方向与屏幕相反
		{
			CMD_RDX=0X90;
			CMD_RDY=0XD0;
		}else				   //X,Y·½ÏòÓëÆÁÄ»ÏàÍ¬
		{
			CMD_RDX=0XD0;
			CMD_RDY=0X90;
		}
		return 1;
	}
	return 0;
}
//与LCD部分有关的函数
//画一个触摸点
//用来校准用的
//x,y:坐标
//color:颜色
void TP_Drow_Touch_Point(u16 x,u16 y,u16 color)
{
	POINT_COLOR=color;
	LCD_DrawLine(x-12,y,x+13,y);//横线
	LCD_DrawLine(x,y-12,x,y+13);//竖线
	LCD_DrawPoint(x+1,y+1);
	LCD_DrawPoint(x-1,y+1);
	LCD_DrawPoint(x+1,y-1);
	LCD_DrawPoint(x-1,y-1);
	LCD_Draw_Circle(x,y,6);//画中心圈
}

//提示校准结果(各个参数)
void TP_Adj_Info_Show(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac)
{
	POINT_COLOR=RED;
	LCD_ShowString(40,160,lcddev.width,lcddev.height,16,"x1:");
 	LCD_ShowString(40+80,160,lcddev.width,lcddev.height,16,"y1:");
 	LCD_ShowString(40,180,lcddev.width,lcddev.height,16,"x2:");
 	LCD_ShowString(40+80,180,lcddev.width,lcddev.height,16,"y2:");
	LCD_ShowString(40,200,lcddev.width,lcddev.height,16,"x3:");
 	LCD_ShowString(40+80,200,lcddev.width,lcddev.height,16,"y3:");
	LCD_ShowString(40,220,lcddev.width,lcddev.height,16,"x4:");
 	LCD_ShowString(40+80,220,lcddev.width,lcddev.height,16,"y4:");
 	LCD_ShowString(40,240,lcddev.width,lcddev.height,16,"fac is:");
	LCD_ShowNum(40+24,160,x0,4,16);		//显示数值
	LCD_ShowNum(40+24+80,160,y0,4,16);	//显示数值
	LCD_ShowNum(40+24,180,x1,4,16);		//显示数值
	LCD_ShowNum(40+24+80,180,y1,4,16);	//显示数值
	LCD_ShowNum(40+24,200,x2,4,16);		//显示数值
	LCD_ShowNum(40+24+80,200,y2,4,16);	//显示数值
	LCD_ShowNum(40+24,220,x3,4,16);		//显示数值
	LCD_ShowNum(40+24+80,220,y3,4,16);	//显示数值
 	LCD_ShowNum(40+56,240,fac,3,16); 	//显示数值,该数值必须在95~105范围之内.

}
////触摸屏校准代码
////得到四个校准参数
void TP_Adjust(void)
{
	u16 pos_temp[4][2];//坐标缓存值
	u8  cnt=0;
	u16 d1,d2;
	u32 tem1,tem2;
	float fac;
	u16 outtime=0;
 	cnt=0;
	POINT_COLOR=BLUE;
	BACK_COLOR =WHITE;
	LCD_Clear(WHITE);//清屏
	POINT_COLOR=RED;//红色
	LCD_Clear(WHITE);//清屏
	POINT_COLOR=BLACK;
	LCD_ShowString(10,40,320,16,16,"Please use the stylus click the");//显示提示信息
	LCD_ShowString(10,56,320,16,16,"cross on the screen.The cross will");//显示提示信息
	LCD_ShowString(10,72,320,16,16,"always move until the screen ");//显示提示信息
	LCD_ShowString(10,88,320,16,16,"adjustment is completed.");//显示提示信息

	TP_Drow_Touch_Point(20,20,RED);//画点1
	tp_dev.sta=0;//消除触发信号
	tp_dev.xfac=0;//xfac用来标记是否校准过,所以校准之前必须清掉!以免错误
	while(1)//如果连续10秒钟没有按下,则自动退出
	{
		tp_dev.scan(1);//扫描物理坐标
		if((tp_dev.sta&0xc0)==TP_CATH_PRES)//按键按下了一次(此时按键松开了.)
		{
			outtime=0;
			tp_dev.sta&=~(1<<6);//标记按键已经被处理过了.

			pos_temp[cnt][0]=tp_dev.x;
			pos_temp[cnt][1]=tp_dev.y;
			cnt++;
			switch(cnt)
			{
				case 1:
					TP_Drow_Touch_Point(20,20,WHITE);				//清除点1
					TP_Drow_Touch_Point(lcddev.width-20,20,RED);	//画点2
					break;
				case 2:
 					TP_Drow_Touch_Point(lcddev.width-20,20,WHITE);	//清除点2
					TP_Drow_Touch_Point(20,lcddev.height-20,RED);	//画点3
					break;
				case 3:
 					TP_Drow_Touch_Point(20,lcddev.height-20,WHITE);			//清除点3
 					TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,RED);	//画点4
					break;
				case 4:	 //全部四个点已经得到
	    		    //对边相等
					tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
					tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,2的距离

					tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
					tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到3,4的距离
					fac=(float)d1/d2;
					if(fac<0.8||fac>1.05||d1==0||d2==0)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据
 						continue;
					}
					tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,3的距离

					tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,4的距离
					fac=(float)d1/d2;
					if(fac<0.8||fac>1.05)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据
						continue;
					}//正确了

					//对角线相等
					tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,4的距离

					tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,3的距离
					fac=(float)d1/d2;
					if(fac<0.8||fac>1.05)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
// 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据
						continue;
					}//正确了
					//计算结果
					tp_dev.xfac=(float)(lcddev.width-40)/(pos_temp[1][0]-pos_temp[0][0]);//得到xfac
					tp_dev.xoff=(lcddev.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//得到xoff

					tp_dev.yfac=(float)(lcddev.height-40)/(pos_temp[2][1]-pos_temp[0][1]);//得到yfac
					tp_dev.yoff=(lcddev.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//得到yoff
					if(abs(tp_dev.xfac)>2||abs(tp_dev.yfac)>2)//触屏和预设的相反了.
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
//						LCD_ShowString(40,26, 16,"TP Need readjust!",1);
						tp_dev.touchtype=!tp_dev.touchtype;//修改触屏类型.
						if(tp_dev.touchtype)//X,Y方向与屏幕相反
						{
							CMD_RDX=0X90;
							CMD_RDY=0XD0;
						}else				   //X,Y方向与屏幕相同
						{
							CMD_RDX=0XD0;
							CMD_RDY=0X90;
						}
						continue;
					}
//					POINT_COLOR=BLUE;
//					LCD_Clear(WHITE);//清屏
//					LCD_ShowString(35,110, 16,"Touch Screen Adjust OK!",1);//校正完成
					HAL_Delay(1000);
					TP_Save_Adjdata();      //保存数据
// 					LCD_Clear(WHITE);//清屏
					return;//校正完成
			}
		}
		HAL_Delay(10);
		outtime++;
		if(outtime>1000)
		{
			TP_Get_Adjdata();
			break;
	 	}
 	}
}

////触摸屏初始化
////返回值:0,没有进行校准
////       1,进行过校准
////触摸屏初始化
////返回值:0,没有进行校准
////       1,进行过校准
u8 TP_Init(void)
{
    TP_Read_XY(&tp_dev.x,&tp_dev.y);//第一次读取初始化
// 	AT24CXX_Init();//初始化24CXX

//	if(1)return 0;//已经校准
	if(TP_Get_Adjdata())return 0;//已经校准
	else			   //未校准?
	{
		LCD_Clear(RED);//清屏
	  TP_Adjust();  //屏幕校准
	  TP_Save_Adjdata();
	}
	TP_Get_Adjdata();
	return 1;
}

void rtp_test(void)
{
//	u8 key;
//	u8 i=0;
	while(1)
	{
		tp_dev.scan(0);
		if(tp_dev.sta&TP_PRES_DOWN)			//触摸屏被按下
		{
			HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,0);
		 	break;
		}else HAL_Delay(10);	//没有按键按下的时候
		if(HAL_GPIO_ReadPin(KEY0_GPIO_Port,KEY0_Pin)==0)	//KEY0按下,则执行校准程序
		{
			LCD_Clear(WHITE);//清屏
		    TP_Adjust();  	//屏幕校准
		    LCD_Clear(WHITE);	//清屏
		}
//		i++;
//		if(i%20==0)LED0=!LED0;
	}
}

void TouchChoose()
{
	while(1)
	{
		tp_dev.scan(0);
		if(tp_dev.y<2700&&tp_dev.y>1350)  //基础部分
		{
			HAL_Delay(10); //触摸屏消抖
			chooseflag=2;
			break;
		}
		else if(tp_dev.y>0&&tp_dev.y<1350)    //发挥部分
		{
			HAL_Delay(10); //触摸屏消抖
			chooseflag=3;
			LCD_Clear(WHITE);
			TIM2->CCR2=2300;   //顶部舵机转到45度
			HAL_UART_Receive_IT(&huart2,&bluetooth_rx_buffer,1);
			while(1)
			{
				if(inputstate==1)
				{
					Stop(realangle);
				}
			}
		}
		else if(tp_dev.y>2700&&tp_dev.y<4050)   //进入调试部分
		{
			HAL_Delay(10);  //触摸屏消抖
			chooseflag=1;
			TIM2->CCR1=1750;  //底部转至90
			LCD_Clear(GREEN);
			HAL_UART_Receive_IT(&huart2,&bluetooth_rx_buffer,1);
			while(1)
			{
				if(inputstate==1)
				{
					pwmval2=(uint16_t)(6.667*inangle2+2000);
					TIM2->CCR2=pwmval2;
					inputstate=3;
				}
				IsRelayerDown();   //充电开关
				IsRelayer2Down();  //放电开关
			}
		}
	}
}


void Stop(uint16_t angle)
{
	uint8_t count=0;   //用于计数转几圈
	HAL_GPIO_WritePin(Relayer_GPIO_Port,Relayer_Pin,1);   //电容提前开始充电
	LCD_ShowAutoPage();
	while(1)
	{
		LCD_Fill(122,360,160,384,WHITE);  //底部舵机数据部分清零
		HCSR04_Read();
		LCD_ShowNum(122,360,Distance,GetPlace(Distance),24);
		HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,0);
		if(DuoGiFlag==0&&count!=2)
		{
//			HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,0);
			TIM2->CCR1=1984;   //转到120
			DuoGiFlag=1;
			HAL_Delay(1500);
			count++;
		}
		else if(DuoGiFlag==1&&count!=2)
		{
			TIM2->CCR1=1517;   //转到60
			DuoGiFlag=0;
			HAL_Delay(1500);
			count++;
		}
//				IsAimXMiddle();   //不断探测是否搜寻的靶子
		else if(count==2)
		{
			TIM2->CCR1=(uint16_t)(7.778*angle+1050);   //直接令底部舵机指向angle
			HAL_Delay(800);
			HAL_GPIO_WritePin(Relayer_GPIO_Port,Relayer_Pin,0);
			HAL_GPIO_WritePin(Beep_GPIO_Port,Beep_Pin,1);     //蜂鸣器响
			HAL_GPIO_WritePin(Relayer2_GPIO_Port,Relayer2_Pin,1); //放电继电器打开
			count++;
		}
		else if(count==3)
		{
			HAL_Delay(100);
			if(TIM2->CCR1<1750)
				TIM2->CCR1=1984;
			else
				TIM2->CCR1=1517;
		}
	}
}
