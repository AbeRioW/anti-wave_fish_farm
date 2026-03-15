#ifndef __BH1750_H
#define __BH1750_H

#include "main.h"
#include "i2c.h"

#define BH1750_ADDR 0x46 << 1
#define BH1750_ADDR_ALT 0x44 << 1

#define BH1750_POWER_ON 0x01
#define BH1750_POWER_OFF 0x00
#define BH1750_RESET 0x07
#define BH1750_CON_H_MODE 0x10
#define BH1750_CON_H_MODE2 0x11
#define BH1750_CON_L_MODE 0x13
#define BH1750_ONE_TIME_H 0x20
#define BH1750_ONE_TIME_H2 0x21
#define BH1750_ONE_TIME_L 0x23

void BH1750_Init(void);
float BH1750_ReadLight(void);
void BH1750_SetAddress(uint8_t addr);

#endif
