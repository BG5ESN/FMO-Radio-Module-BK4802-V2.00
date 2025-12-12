#ifndef __JUMPER_H__
#define __JUMPER_H__
#include "stdint.h"
#include "def.h"

void jumperInit(void);
uint8_t getJumpHex(void);
#if DBUG_FUNCTION == ANTENNA_TEST
typedef enum
{
    E_ANTENNA_MODE_NORMAL = 0,   // 正常模式，发射时滤波器工作，接收时衰减器档
    E_ANTENNA_MODE_RX_ONLY = 1,  // 仅接收,天线开关一直在衰减器挡
    E_ANTENNA_MODE_ATT_ONLY = 2, // 正常模式,天线开关一直在衰减器档
} AntennaMode;
uint8_t getAntennaTestMode(void); // 获取天线路径测试值
#elif DBUG_FUNCTION == VOL_ADJ_TEST
uint8_t getVolumeAdjTestValue(void); // 获取音量调节测试值,范围0~7 (0:静音,7:最大)
#endif
#endif