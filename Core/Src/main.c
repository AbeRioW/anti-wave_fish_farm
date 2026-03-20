/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "BH1750.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// 微秒延时函数
void delay_us(uint32_t us)
{
    uint32_t delay = (HAL_RCC_GetHCLKFreq()/4000000*us);
    while(delay--);
}

// DS18B20相关函数
#define DS18B20_PIN DS1820_Pin
#define DS18B20_PORT DS1820_GPIO_Port

// 设置DS18B20引脚为输出
void DS18B20_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

// 设置DS18B20引脚为输入
void DS18B20_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

// 复位DS18B20
uint8_t DS18B20_Reset(void)
{
    uint8_t response = 0;
    
    DS18B20_SetOutput();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(750);
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    delay_us(15);
    
    DS18B20_SetInput();
    response = HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN);
    
    return response;
}

// 检查DS18B20是否存在
uint8_t DS18B20_Check(void)
{
    uint8_t pulse_time = 0;
    
    DS18B20_SetInput();
    
    // 等待DS18B20的低脉冲（60-240us）
    while (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) && pulse_time < 100) {
        pulse_time++;
        delay_us(1);
    }
    
    if (pulse_time >= 100) {
        return 1; // 无响应
    }
    
    pulse_time = 0;
    
    // 等待低脉冲结束
    while (!HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) && pulse_time < 240) {
        pulse_time++;
        delay_us(1);
    }
    
    if (pulse_time >= 240) {
        return 1; // 无响应
    }
    
    return 0; // 正常
}

// 向DS18B20写1位
void DS18B20_WriteBit(uint8_t bit)
{
    DS18B20_SetOutput();
    
    if (bit) {
        // 写1
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        delay_us(9);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        delay_us(55);
    } else {
        // 写0
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        delay_us(70);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        delay_us(2);
    }
}

// 向DS18B20写1字节
void DS18B20_WriteByte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        DS18B20_WriteBit(byte & 0x01);
        byte >>= 1;
    }
}

// 从DS18B20读1位
uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit = 0;
    
    DS18B20_SetOutput();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(10);
    
    DS18B20_SetInput();
    if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_SET) {
        bit = 1;
    } else {
        bit = 0;
    }
    
    delay_us(45);
    return bit;
}

// 从DS18B20读1字节
uint8_t DS18B20_ReadByte(void)
{
    uint8_t i, byte = 0;
    for (i = 0; i < 8; i++) {
        byte >>= 1;
        if (DS18B20_ReadBit()) {
            byte |= 0x80;
        }
    }
    return byte;
}

// 读取DS18B20温度
float DS18B20_ReadTemperature(void)
{
    uint8_t temp_l, temp_h;
    int16_t temp_raw;
    float temp = -999.9f;
    
    // 复位
    DS18B20_Reset();
    if (DS18B20_Check()) {
        return -999.9f; // 错误
    }
    
    // 跳过ROM
    DS18B20_WriteByte(0xCC);
    
    // 转换温度
    DS18B20_WriteByte(0x44);
    
    // 等待转换完成（最长800ms，12位分辨率）
    HAL_Delay(800); // 使用固定延时，更可靠
    
    // 复位
    DS18B20_Reset();
    if (DS18B20_Check()) {
        return -999.9f; // 错误
    }
    
    // 跳过ROM
    DS18B20_WriteByte(0xCC);
    
    // 读温度
    DS18B20_WriteByte(0xBE);
    
    // 读取温度低字节
    temp_l = DS18B20_ReadByte();
    
    // 读取温度高字节
    temp_h = DS18B20_ReadByte();
    
    // 组合温度数据
    temp_raw = (int16_t)((temp_h << 8) | temp_l);
    
    // 计算温度
    if (temp_raw < 0) {
        // 负温度
        temp = (float)(~temp_raw + 1) * 0.0625f;
    } else {
        // 正温度
        temp = (float)temp_raw * 0.0625f;
    }
    
    // 验证温度范围（DS18B20的有效范围：-55℃ to +125℃）
    if (temp < -55.0f || temp > 125.0f) {
        return -999.9f; // 错误
    }
    
    return temp;
}

// 初始化DS18B20
void DS18B20_Init(void)
{
    // 设置为输出模式
    DS18B20_SetOutput();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    
    // 复位测试
    DS18B20_Reset();
}

// 读取AZDM01混度传感器值
float AZDM01_ReadHumidity(void)
{
    float humidity = 0.0f;
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        uint16_t adc_value = HAL_ADC_GetValue(&hadc1);
        // 根据AZDM01传感器的特性转换为湿度值
        // 假设传感器输出范围为0-3.3V，对应0-100%RH
        humidity = (float)adc_value / 4095.0f * 100.0f;
    }
    HAL_ADC_Stop(&hadc1);
    return humidity;
}

// HC-SR04超声波测距函数（使用GPIO轮询 + 简单计数）
float HCSR04_MeasureDistance(void)
{
    uint32_t start_time = 0;
    uint32_t end_time = 0;
    uint32_t pulse_width = 0;
    float distance = 0.0f;
    
    // 发送10us的触发脉冲
    HAL_GPIO_WritePin(HC_SR04_Trig_GPIO_Port, HC_SR04_Trig_Pin, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(HC_SR04_Trig_GPIO_Port, HC_SR04_Trig_Pin, GPIO_PIN_RESET);
    
    // 等待Echo信号上升沿
    uint32_t timeout = 0;
    while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET) { // Echo连接到PB3
        timeout++;
        if (timeout > 1000000) { // 超时保护
            return -1.0f;
        }
    }
    
    // 记录起始时间（使用TIM2计数器）
    __HAL_TIM_SetCounter(&htim2, 0);
    HAL_TIM_Base_Start(&htim2);
    
    // 等待Echo信号下降沿
    timeout = 0;
    while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_SET) { // Echo连接到PB3
        timeout++;
        if (timeout > 10000000) { // 超时保护
            HAL_TIM_Base_Stop(&htim2);
            return -1.0f;
        }
    }
    
    // 记录结束时间（使用TIM2计数器）
    pulse_width = __HAL_TIM_GetCounter(&htim2);
    HAL_TIM_Base_Stop(&htim2);
    
    // 计算距离：距离(cm) = 脉冲宽度(us) * 0.0343 / 2
    // TIM2的时钟频率是72MHz / 72 = 1MHz，所以1个计数单位 = 1us
    distance = (float)pulse_width * 0.0343f / 2.0f;
    
    // 限制距离范围（2cm-400cm）
    if (distance < 2.0f || distance > 400.0f) {
        return -1.0f;
    }
    
    return distance;
}

// 初始化测量状态
void HCSR04_Init(void)
{
    // 无需特殊初始化
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  BH1750_Init();
  HCSR04_Init();
  DS18B20_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 读取光照数据
    float light_value = BH1750_ReadLight();
    
    // 读取超声波距离
    float distance = HCSR04_MeasureDistance();
    
    // 读取温度数据
    float temperature = DS18B20_ReadTemperature();
    
    // 读取混度数据
    float humidity = AZDM01_ReadHumidity();
    
    // 显示数据
    OLED_Clear();
    OLED_ShowString(0,0,(uint8_t*)"Lux:",8,1);
    oled_showFnum(30,0,light_value,8,1);
    
    OLED_ShowString(0,16,(uint8_t*)"Distance:",8,1);
    if (distance >= 0) {
        // 四舍五入到小数点后1位
        float rounded_distance = (float)((int)(distance * 10 + 0.5)) / 10;
        oled_showFnum(60,16,rounded_distance,8,1);
        OLED_ShowString(100,16,(uint8_t*)"cm",8,1);
    } else {
        OLED_ShowString(60,16,(uint8_t*)"Error",8,1);
    }
    
    OLED_ShowString(0,32,(uint8_t*)"Temp:",8,1);
    if (temperature != -999.9f) {
        // 四舍五入到小数点后1位
        float rounded_temp = (float)((int)(temperature * 10 + 0.5)) / 10;
        oled_showFnum(30,32,rounded_temp,8,1);
        OLED_ShowString(70,32,(uint8_t*)"C",8,1);
    } else {
        OLED_ShowString(30,32,(uint8_t*)"Error",8,1);
    }
    
    OLED_ShowString(0,48,(uint8_t*)"Humidity:",8,1);
    // 四舍五入到小数点后1位
    float rounded_humidity = (float)((int)(humidity * 10 + 0.5)) / 10;
    oled_showFnum(60,48,rounded_humidity,8,1);
    OLED_ShowString(100,48,(uint8_t*)"%",8,1);
    
    OLED_Refresh();
    HAL_Delay(500);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
