/*
 *Radio driver
 *Author:xifengzui AKA BG5ESN
 *Date:2024-11-8
 */

#ifndef __RADIO_H__
#define __RADIO_H__
#include "components.h"
#include "wdt.h"
#include "SHARECom.h"
#define RSSI_TRIG_THRESHOLD_0 0
#define RSSI_TRIG_THRESHOLD_1 10
#define RSSI_TRIG_THRESHOLD_2 20
#define RSSI_TRIG_THRESHOLD_3 30
#define RSSI_TRIG_THRESHOLD_4 40
#define RSSI_TRIG_THRESHOLD_5 50
#define RSSI_TRIG_THRESHOLD_6 60
#define RSSI_TRIG_THRESHOLD_7 70
#define RSSI_TRIG_THRESHOLD_8 80
#define RSSI_TRIG_THRESHOLD_9 90
#define RSSI_TRIG_THRESHOLD_10 100

#define SNR_TRIG_THRESHOLD_0 0
#define SNR_TRIG_THRESHOLD_1 5
#define SNR_TRIG_THRESHOLD_2 10
#define SNR_TRIG_THRESHOLD_3 15
#define SNR_TRIG_THRESHOLD_4 20
#define SNR_TRIG_THRESHOLD_5 25
#define SNR_TRIG_THRESHOLD_6 30
#define SNR_TRIG_THRESHOLD_7 35
#define SNR_TRIG_THRESHOLD_8 40
#define SNR_TRIG_THRESHOLD_9 45
#define SNR_TRIG_THRESHOLD_10 50

void radioInit(void);
void radioTask(void);

void radioSetAudioOutputLevel(uint8_t level);
void radioSetMicInputLevel(uint8_t level);
void radioSetSQLLevel(uint8_t level);
void radioSetPower(uint8_t level); // 0,1,2分为三档 0最低, 2最高
void radioSetTxFreq(float freq);
void radioSetRxFreq(float freq);
void radioSetFreqTune(int32_t tuneHz); // 设置频率偏移(Hz)
void radioApplyFreqTune(void);         // 重新应用频偏到当前收/发频率
uint8_t radioGetSMeter(void);
#endif