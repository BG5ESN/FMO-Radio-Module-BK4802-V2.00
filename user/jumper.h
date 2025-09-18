#ifndef __JUMPER_H__
#define __JUMPER_H__
#include "stdint.h"
typedef enum
{
    E_JUMPER_MODE_NORMAL = 0,//正常模式，发射时滤波器工作，接收时衰减器档
    E_JUMPER_MODE_RX_ONLY = 1, //仅接收,天线开关一直在衰减器挡
    E_JUMPER_MODE_ATT_ONLY = 2,//正常模式,天线开关一直在衰减器档
} JumperMode;

void jumperInit(void);
uint8_t getJumperMode(void);


#endif