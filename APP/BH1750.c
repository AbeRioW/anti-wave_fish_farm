#include "BH1750.h"

#define BH1750_WRITE_ADDR 0x46
#define BH1750_READ_ADDR  0x47

void BH1750_Init(void)
{
    uint8_t power_on_cmd = 0x01;
    uint8_t measure_cmd = 0x10;

    HAL_I2C_Master_Transmit(&hi2c1, BH1750_WRITE_ADDR, &power_on_cmd, 1, 100);
    HAL_Delay(10);
    HAL_I2C_Master_Transmit(&hi2c1, BH1750_WRITE_ADDR, &measure_cmd, 1, 100);
    HAL_Delay(180);
}

float BH1750_ReadLight(void)
{
    uint8_t buf[2];
    uint16_t dis_data;
    float temp;
    uint8_t measure_cmd = 0x10;

    HAL_I2C_Master_Transmit(&hi2c1, BH1750_WRITE_ADDR, &measure_cmd, 1, 100);
    HAL_Delay(180);
    HAL_I2C_Master_Receive(&hi2c1, BH1750_READ_ADDR, buf, 2, 100);

    dis_data = buf[0];
    dis_data = (dis_data << 8) + buf[1];
    temp = (float)dis_data / 1.2f;

    return temp;
}
