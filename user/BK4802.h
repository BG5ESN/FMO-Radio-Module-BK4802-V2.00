#ifndef __BK4802_H__
#define __BK4802_H__
#include "components.h"

void BK4802Init(void);

void BK4802SetVolLevel(uint8_t level);
uint8_t BK4802GetVolLevel(void);

void BK4802SetRSSIThre(uint8_t thre); // 0~9
uint8_t BK4802GetRSSIThre(void);

// void BK4802Enable(void);
// void BK4802Disable(void);
void BK4802SetPower(uint8_t level);
uint16_t BK4802GetDynamicCfg(uint8_t cfgReg); // 从配置数组中获取指定的寄存器参数
void BK4802SetDynamicCfg(uint8_t cfgReg, uint16_t value);
uint8_t BK4802SNRRead(void);
uint8_t BK4802RSSIRead(void);
uint8_t BK4802RXVolumeRead(void);
uint8_t BK4802readASKOUT(void);
void BK4802Tx(float freq);
void BK4802Rx(float freq);
// 可以通过此函数刷新状态
void BK4802Flush(float freq);
xBool BK4802IsTx(void); // 是否在发送状态
xBool BK4802IsRx(void); // 是否在接收状态
xBool BK4802IsError(void);
void BK4802Reset(float freq);
void BK4802DebugTask(void);
uint8_t BK4802GetSMeter(void); // 量化RSSI,返回1~9

// 频率偏移校准接口
void BK4802SetFreqOffsetHz(float offsetHz);      // 直接设置绝对偏移(Hz)
void BK4802SetFreqOffsetPPM(float ppm);          // 以 PPM 设置(相对最近一次调用的原始频率)
float BK4802GetFreqOffsetHz(void);               // 获取当前偏移(Hz)
float BK4802GetFreqOffsetMHz(void);              // 获取当前偏移(MHz)
float BK4802QuantizeFreq(float freqMHz, float stepHz); // 辅助量化
void BK4802FlushWithStep(float reqFreqMHz, float stepHz); // 刷新时应用量化
#endif