#include "hc_sr04.h"

float distant;     									//测量距离
uint32_t measure_buf[3] = {0};			//存放定时器计数值的数组
uint8_t measure_cnt = 0;  				  //状态标志位
uint32_t higt_time;                 //超声波模块返回高电平时间


uint8_t distance_legth = 5;  //默认的距离值
bool distance_boundary = false;

//us延时函数
void delay_us(uint32_t us)
{
		uint32_t delay = (HAL_RCC_GetHCLKFreq()/4000000*us);
	  while(delay--);
}

//读取距离
void sr04_getdata(void)
{
	  char long_dis[100]={0};
		switch(measure_cnt)
		{
			case 0:
				TRIG_H;
			  delay_us(30);
			  TRIG_L;
			
			  measure_cnt++;
			  __HAL_TIM_SET_CAPTUREPOLARITY(&htim2,TIM_CHANNEL_1,TIM_INPUTCHANNELPOLARITY_RISING);
				HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1);      //启动输入捕获
				break;
			case 3:
				 higt_time = measure_buf[1]-measure_buf[0];      //高电平时间
			  
			   distant = (higt_time*0.034)/2;                	//单位是cm
			
			   printf("当前距离%.2f",distant);
			  measure_cnt = 0;
			  TIM2->CNT = 0;                //情况计时器技术
				HAL_TIM_IC_Stop_IT(&htim2,TIM_CHANNEL_1);
			  HAL_Delay(1000);
			
			  break;
		}
}

//回调函数
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
		if(TIM2==htim->Instance)    //判断触发的中断的定时器为TIM2
		{
				switch(measure_cnt)
				{
					case 1:
							measure_buf[0] = HAL_TIM_ReadCapturedValue(&htim2,TIM_CHANNEL_1);     //获取当前的捕获值
					    __HAL_TIM_SET_CAPTUREPOLARITY(&htim2,TIM_CHANNEL_1,TIM_INPUTCHANNELPOLARITY_FALLING);         //设置为下降沿捕获
							measure_cnt++;
					break;
					
					case 2:
							measure_buf[1] = HAL_TIM_ReadCapturedValue(&htim2,TIM_CHANNEL_1);     //获取当前的捕获值
					    HAL_TIM_IC_Stop_IT(&htim2,TIM_CHANNEL_1);                             //停止捕获
					    measure_cnt++;
					break;
				}
		}
}

