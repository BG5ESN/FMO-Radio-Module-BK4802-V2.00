#ifndef __SHARECOM_H__
#define __SHARECOM_H__
#include "components.h"
// AT Command COM Port
typedef struct
{
    uint8_t ver[8];  // version info
    uint16_t bandCap; // band capability
    uint8_t sql;     // SQL level 0~10 0 is off
    float txFreq;    // TX freq 438.500MHz 145.100MHz
    float rxFreq;    // RX freq
    uint8_t rxVol;   // RX volume 0~10 0 is off
    uint8_t txVol;   // TX volume 0~10 0 is off
    float tCTCSS;    // TX CTCSS
    float rCTCSS;    // RX CTCSS
    int32_t freqTune; // frequency offset in Hz (可正可负)，用于晶振/本振校准
    uint8_t txPwr;   // 0 low 1 mid 2 high
    uint8_t smeter;  // S meter level 1~9
    uint8_t rfEnable; // 1: allow TX 0: forbid TX (AT+RF=ENABLE/DISABLE)
} SHARECom;

#endif