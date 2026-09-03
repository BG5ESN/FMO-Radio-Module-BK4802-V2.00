#include "components.h"
#include "radio.h"
#include "main.h"
#define BK4802_SNR_BAD_THRE 2

#define IF 0.1370000
#define TWO24 16777216
#define CRYSTAL 21.2500000

static uint8_t thresholdIdx = 0;
static uint8_t softRSSIThre = 80; // default
// 范围60~127 共10档,均匀分布
// 0~10档                           0   1   2  3   4   5    6    7   8    9   10
static const uint8_t threTable[] = {0, 64, 70, 76, 82, 89, 97, 104, 112, 118, 125};
#define THRE_SIZE (sizeof(threTable) / sizeof(threTable[0]))
static xBool isTx = false;
static float g_freqOffsetMHz = 0.0f;
static float g_lastUserFreqMHz = 0.0f;
typedef struct
{
    uint8_t addr;
    uint16_t value;
} BK4802Reg;

#if 1 // 软件IIC接口部分
// SDA PA10
// SCL PA9
void sclIn(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
void sclOut(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void sdaIn(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
void sdaOut(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
void sdaHigh(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
}
void sdaLow(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
}
void sclHigh(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
}
void sclLow(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
}
unsigned char sdaRead(void)
{
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) ? 1 : 0;
}

void radioDelay(void)
{
    uint32_t i = 100;
    while (i--)
        ;
}

static SoftI2cHandler SoftIICPort =
    {
        .delay = radioDelay,
        .isErr = 0,
        .sclHigh = sclHigh,
        .sclIn = sclIn,
        .sclLow = sclLow,
        .sclOut = sclOut,
        .sdaHigh = sdaHigh,
        .sdaIn = sdaIn,
        .sdaLow = sdaLow,
        .sdaOut = sdaOut,
        .sdaRead = sdaRead,
};

#endif

/*通用寄存器,配置后不再变化*/
const static BK4802Reg commonConfig[] =
    {
        {6, 0xf140},  // 锁相环相关设置，建议用默认值
        {9, 0xe0e0},  // 锁相环相关设置
        {10, 0x8543}, // B15~B10：微调晶体振荡器频率; B07~B06：ADC稳压器输出电压
                      // B05:1-中频ADC时钟有效; B04:1-采样ADC时钟有效
                      // B03~B02：晶体振荡器幅度; B01~B00：数字稳压器输出电压
        {11, 0x0700}, // B15~B13：发射音频ADC参考电压; B12:增加发射音频VGA增益
                      // B06:麦克风输入电阻(0-16k,1-8k); B05:麦克风VGA反馈电阻(0-52k,1-26k)
                      // B04~B03：音频功放非交叠时间(0-2nS;1-3nS;2-4nS;3-5nS)
        {12, 0xa066}, // B15~B08：接收中频ADC参考电压; B07~B05：接收中频增益(3dB/级)
                      // B04:接收中频滤波器带宽(0-200kHz;1-1MHz)
                      // B03:接收中频VGA电流偏置(0-2uA;1-10uA); B02~B00：中频稳压器输出电压
        {13, 0xffff}, // B15:收发控制方式(0-寄存器;1-DSP); B14:收发模式(0-接收;1-发射)
                      // B13:LNA增益(0-低;1-高); B12~B10:LNA偏置电流
                      // B09~B07:混频器直流偏置; B05:ASK模式使能
                      // B04~B03:接收前端稳压器输出电压; B02~B00:PA输出功率
        {15, 0x061F}, // B12~B00:发射限幅设置
        {16, 0x9e3c}, // B15:发射音频AGC使能(0-无效;1-使能)
        {17, 0x1f00}, // TX Audio AGC设置
        {18, 0xd1d1}, // B15~B04:发射频偏控制
        {19, 0x300C}, // B15~B14:CIC滤波器增益(0-0dB;1-1dB;2-3.5dB;3-6dB)
                      // B13~B12:FM解调输出幅度设置; B03~B00:音量调整

        {20, 0x01ff}, // B15:关闭自动频率校正AFC(0-打开;1-关闭)

        {21, 0xE000}, // AFC阈值设置
        {22, 0x0c00}, // B15:IQ边带选择(0-上边带;1-下边带)
                      // B14:AFC方向选择(0-上边带;1-下边带)
                      // B07~B00:静噪RSSI阈值

};
#define BK4802CommonRegNum (sizeof(commonConfig) / sizeof(BK4802Reg))
/*接收寄存器*/
const static BK4802Reg rxConfig[] = {
    // Register configuration array {REG, VALUE}
    {4, 0x0300}, // B15:1-关闭频率综合器; B14:1-关闭接收前端; B13:1-关闭接收中频; B12:1-关闭接收音频
                 // B11:1-关闭发射前端; B10:1-关闭发射拼音; B09:1-关闭采样ADC
    {5, 0x0C04}, // 锁相环相关设置，建议用默认值

};
#define BK4802RxRegNum (sizeof(rxConfig) / sizeof(BK4802Reg))
/*发送寄存器*/
const static BK4802Reg txConfig[] =
    {
        {4, 0x7C00},
        {5, 0x0004},
};
#define BK4802TxRegNum (sizeof(txConfig) / sizeof(BK4802Reg))
/*动态寄存器（如音量）*/
static BK4802Reg dynamicConfig[] =
    {
        {7, 0xED00}, // 15~13 IF Gain
        {8, 0x17E0},
        {14, 0xFFE0}};
#define BK4802DynamicRegNum (sizeof(dynamicConfig) / sizeof(BK4802Reg))

uint16_t BK4802GetDynamicCfg(uint8_t cfgReg)
{
    // 遍历:dynamicConfig 找到指定的寄存器参数
    for (int ii = 0; ii < sizeof(dynamicConfig) / sizeof(dynamicConfig[0]); ii++)
    {
        if (dynamicConfig[ii].addr == cfgReg)
        {
            return dynamicConfig[ii].value;
        }
    }
    return 0;
}

void BK4802SetDynamicCfg(uint8_t cfgReg, uint16_t value)
{
    for (int ii = 0; ii < BK4802DynamicRegNum; ii++)
    {
        if (dynamicConfig[ii].addr == cfgReg)
        {
            dynamicConfig[ii].value = value;
            return;
        }
    }
}

void BK4802SetRSSIThre(uint8_t thre)
{
    // 0~9
    if (thre >= THRE_SIZE)
    {
        thre = THRE_SIZE - 1;
    }
    thresholdIdx = thre;
    softRSSIThre = threTable[thre];
}

uint8_t BK4802GetRSSIThre(void)
{
    return thresholdIdx;
}

void BK4802WriteReg(uint8_t addr, uint16_t data)
{
    softI2cWriteWordToAddr(&SoftIICPort, 0x48, addr, data);
    if (SoftIICPort.isErr)
    {
        log_e("write error!");
    }
}

uint16_t BK4802ReadReg(uint8_t addr)
{
    uint16_t ret = softI2cReadWordFromAddr(&SoftIICPort, 0x48, addr);
    if (SoftIICPort.isErr)
    {
        log_e("read error!");
        ret = 0;
    }
    return ret;
}

uint8_t BK4802SNRRead(void)
{
    // 地址为24 读取信噪比,BIT13~BIT08为信噪比值
    uint16_t value;
    value = BK4802ReadReg(24);
    value = value & 0x3F00;
    value >>= 8;
    return (uint8_t)value;
}

uint8_t BK4802RSSIRead(void)
{
    // 地址为24 读取信号强度,BIT07~BIT00为信号强度值
    uint16_t value;
    value = BK4802ReadReg(24);
    value = value & 0x00FF;
    return (uint8_t)value;
}

uint8_t BK4802AFCResidualRead(void)
{
    // 地址为25 读取AFC残差,BIT07~BIT00为AFC残差值
    uint16_t value;
    value = BK4802ReadReg(25);
    value = value & 0x00FF;
    return (uint8_t)value;
}

uint8_t BK4802ExNoiseIndicator(void)
{
    // 地址为25 读取外部噪声指示,BIT12~BIT00为外部噪声指示值
    uint16_t value;
    value = BK4802ReadReg(26);
    value = value & 0x1FFF;
    return (uint8_t)value;
}

uint8_t BK4802RXVolumeRead(void)
{
    uint16_t value;
    value = BK4802ReadReg(30);
    return (uint8_t)(value >> 8 & 0x00FF);
}

uint8_t BK4802readASKOUT(void)
{
    // read PB2
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
}

uint8_t BK4802ExNoiseThreshodForSpeakOffConditonAcquireFromSARADC(void)
{
    uint16_t value;
    value = BK4802ReadReg(30);
    return (uint8_t)(value & 0x00FF);
}
uint8_t BK4802RSSIThreshodForSpeakOffConditonAcquireFromSARADC(void)
{
    uint16_t value;
    value = BK4802ReadReg(31);
    return (uint8_t)(value & 0x00FF);
}

uint16_t BK4802ChipID(void)
{
    return BK4802ReadReg(27);
}

void BK4802SetVolLevel(uint8_t level) // 0~31
{
    uint8_t vol;

    uint16_t reg14;
    reg14 = BK4802GetDynamicCfg(14);

    if (level > 31)
    {
        level = 31;
    }

    vol = level;
    reg14 &= ~(0x1F << 9); // 清除原有音量值
    reg14 |= (vol << 9);   // 设置新的音量值

    BK4802WriteReg(14, reg14);
    BK4802SetDynamicCfg(14, reg14);
}

uint8_t BK4802GetVolLevel(void)
{
    return 0; // not implemented
}

xBool nDivCacl(float freq, float *outnDiv, uint16_t *outRegs)
{

    // step1:确定分批系数
    if (freq <= 512 && freq > 384)
    {
        *outRegs = 0x0002;
        *outnDiv = 4.0000; // 4x1
        return true;
    }
    else if (freq <= 170 && freq >= 128)
    {
        *outRegs = 0x2004;
        *outnDiv = 12.0000; // 4x3
        return true;
    }
    else if (freq <= 57 && freq >= 43)
    {
        *outRegs = 0x8008;
        *outnDiv = 36.0000; // 4x9
        return true;
    }
    else if (freq <= 46 && freq >= 35)
    {
        *outRegs = 0xA00A;
        *outnDiv = 44.0000; // 4x11
        return true;
    }
    else if (freq <= 32 && freq >= 24)
    {
        *outRegs = 0xC00F;
        *outnDiv = 64.0000; // 4x16
        return true;
    }
    else
    {
        return false;
    }
}

void BK4802Default(void)
{
    for (int i = 0; i < BK4802CommonRegNum; i++)
    {
        BK4802WriteReg(commonConfig[i].addr, commonConfig[i].value);
    }
}

void BK4802Tx(float freq)
{
    BK4802Reg freqRegs[3];
    freqRegs[0].addr = 0;
    freqRegs[1].addr = 1;
    freqRegs[2].addr = 2;
    float nDiv;
    uint32_t txValue;
    float adjFreq = freq + g_freqOffsetMHz; // 应用偏移后的目标射频频率

    g_lastUserFreqMHz = freq; // 记录原始用户频率

    if (nDivCacl(adjFreq, &nDiv, &freqRegs[2].value) == false)
    {
        log_w("freq(含偏移)超出范围: req=%.6f MHz, offset=%.6f MHz, adj=%.6f MHz", freq, g_freqOffsetMHz, adjFreq);
        return;
    }
    txValue = (uint32_t)(adjFreq * nDiv * TWO24 / CRYSTAL);
    freqRegs[0].value = (uint16_t)((txValue >> 16) & 0xFFFF);
    freqRegs[1].value = (uint16_t)(txValue & 0xFFFF);

    isTx = true;

    // 等待模块切换到发射状态
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET); // TX
    HAL_Delay(30);

    // step1:设置寄存器
    for (int i = 0; i < BK4802TxRegNum; i++)
    {
        BK4802WriteReg(txConfig[i].addr, txConfig[i].value);
    }
    BK4802Default();
    // step2: 设置动态寄存器
    for (int i = 0; i < sizeof(dynamicConfig) / sizeof(dynamicConfig[0]); i++)
    {
        BK4802WriteReg(dynamicConfig[i].addr, dynamicConfig[i].value);
    }

    // step2:设置频率,从高位到低位
    BK4802WriteReg(freqRegs[2].addr, freqRegs[2].value);
    BK4802WriteReg(freqRegs[0].addr, freqRegs[0].value);
    BK4802WriteReg(freqRegs[1].addr, freqRegs[1].value);

    double actualMHz = ((double)txValue * (double)CRYSTAL) / ((double)nDiv * (double)TWO24);
    log_i("TX req:%.4f MHz off:%.6f MHz adj:%.4f MHz actual:%.6f MHz nDiv:%.1f r2:%04x r0:%04x r1:%04x",
          freq, g_freqOffsetMHz, adjFreq, actualMHz, nDiv, freqRegs[2].value, freqRegs[0].value, freqRegs[1].value);
}

void BK4802Rx(float freq)
{
    BK4802Reg freqRegs[3];
    freqRegs[0].addr = 0;
    freqRegs[1].addr = 1;
    freqRegs[2].addr = 2;
    float nDiv;
    uint32_t rx;
    float adjFreq = freq + g_freqOffsetMHz; // 应用偏移后的目标射频频率

    g_lastUserFreqMHz = freq; // 记录原始用户频率
    if (nDivCacl(adjFreq, &nDiv, &freqRegs[2].value) == false)
    {
        log_w("freq(含偏移)超出范围: req=%.6f MHz, offset=%.6f MHz, adj=%.6f MHz", freq, g_freqOffsetMHz, adjFreq);
        return;
    }
    // 接收路径：本振 = 期望RF - IF，因此加偏移后本振应 = (adjFreq - IF)
    rx = (uint32_t)(((adjFreq - IF * 1.000f) * nDiv * TWO24) / CRYSTAL);
    freqRegs[0].value = (uint16_t)((rx >> 16) & 0xFFFF);
    freqRegs[1].value = (uint16_t)(rx & 0xFFFF);
    isTx = false;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET); // 迅速速切换到RX
    HAL_Delay(30);

    // step1:设置寄存器
    for (int i = 0; i < BK4802RxRegNum; i++)
    {
        BK4802WriteReg(rxConfig[i].addr, rxConfig[i].value);
    }
    BK4802Default();
    // step2: 设置动态寄存器
    for (int i = 0; i < sizeof(dynamicConfig) / sizeof(dynamicConfig[0]); i++)
    {
        BK4802WriteReg(dynamicConfig[i].addr, dynamicConfig[i].value);
    }

    // step3:设置频率,从高位到低位
    BK4802WriteReg(freqRegs[2].addr, freqRegs[2].value);
    BK4802WriteReg(freqRegs[0].addr, freqRegs[0].value);
    BK4802WriteReg(freqRegs[1].addr, freqRegs[1].value);
    // 计算由寄存器量化后的实际本振频率（接收路径为本振=RF-IF）
    double actualRxMHz = ((double)rx * (double)CRYSTAL) / ((double)nDiv * (double)TWO24);
    log_i("RX req:%.4f MHz off:%.6f MHz adj:%.4f MHz actualLO:%.6f MHz nDiv:%.1f r2:%04x r0:%04x r1:%04x",
          freq, g_freqOffsetMHz, adjFreq, actualRxMHz, nDiv, freqRegs[2].value, freqRegs[0].value, freqRegs[1].value);
}

void BK4802Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    // step1: 初始化所有关于电台的GPIO和外设功能
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // 时钟线SCK = PA10
    // 数据线SDA = PA9
    // 默认设置为输入浮空状态
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 发送接收控制口为TRX=PA8 文档描述：高电平为发射，低电平为接收，上电默认为接收
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

    // 初始化BK4802
    softI2cInit(&SoftIICPort);
    HAL_Delay(100); // 启动延时，不能立即设置模块。
    BK4802Rx(438.5000);
}

void BK4802Reset(float freq)
{
    BK4802Rx(freq);
}

void BK4802DebugTask(void)
{
    static uint32_t readBK4802Period = 0;
    if (millis() < readBK4802Period)
    {
        return;
    }
    readBK4802Period = millis() + 1000;
    uint8_t rssi = BK4802RSSIRead();
    uint8_t snr = BK4802SNRRead();
    uint8_t afc = BK4802AFCResidualRead();
    uint8_t exNoise = BK4802ExNoiseIndicator();
    uint8_t rxVol = BK4802RXVolumeRead();
    uint8_t exNoiseThreshod = BK4802ExNoiseThreshodForSpeakOffConditonAcquireFromSARADC();
    uint8_t rssiThreshod = BK4802RSSIThreshodForSpeakOffConditonAcquireFromSARADC();
    log_i("RSSI:%d, SNR:%d, AFC:%d, Ex Noise:%d, RX Vol:%d, Noise (AUTO):%d, Thr OFF(AUTO):%d\r", rssi, snr, afc, exNoise, rxVol, exNoiseThreshod, rssiThreshod);
}

void BK4802Enable(void)
{
    // TODO
}

void BK4802Disable(void)
{
    // TODO
}

void BK4802Flush(float freq)
{
    if (isTx)
    {
        BK4802Tx(freq);
    }
    else
    {
        BK4802Rx(freq);
    }
}

xBool BK4802IsTx(void)
{
    return isTx;
}

xBool BK4802IsError(void)
{
    return SoftIICPort.isErr;
}

// RSSI滤波相关变量
static float filteredRssi = 0.0f;             // 滤波后的RSSI值
static const float rxNotDetectedAlpha = 0.2; // 滤波系数,没有收到任何信号时，值越小，避免环境突变噪声
static const float rxDetectedAlpha = 0.6f;    // 检测到有效信号后,值大，停止响应越快
static xBool lastRxState = xFalse;            // 上一次的接收状态
static const uint8_t hysteresis = 3;          // 滞后值，防止在阈值附近抖动

xBool BK4802IsRx(void)
{
    uint8_t rssi;
    uint8_t snr;
    rssi = BK4802RSSIRead();
    snr = BK4802SNRRead();
    // 使用滞后比较来决定状态切换
    if (!lastRxState)
    {
        filteredRssi = rxNotDetectedAlpha * rssi + (1.0f - rxNotDetectedAlpha) * filteredRssi;
        // 当前不是接收状态，检查是否进入接收
        if ((filteredRssi > softRSSIThre + hysteresis) && (snr >= BK4802_SNR_BAD_THRE) && (isTx == xFalse))
        {
            lastRxState = xTrue;
        }
    }
    else
    {
        filteredRssi = rxDetectedAlpha * rssi + (1.0f - rxDetectedAlpha) * filteredRssi;
        if (filteredRssi < softRSSIThre - hysteresis || (snr < BK4802_SNR_BAD_THRE))
        {
            lastRxState = xFalse;
        }
    }

    return lastRxState;
}

uint8_t BK4802GetSMeter(void)
{
    // RSSI量化，返回1~9
    // RSSI范围从60~127 量化为1~9
    //  60~64 1
    //  65~68 2
    //  69~84 3
    //  85~86 4
    //  87~102 5
    //  103~108 6
    //  109~114 7
    //  115~124 8
    //  125~127 9
    uint8_t rssi = BK4802RSSIRead();
    uint8_t smeter = 0;
    if (rssi <= threTable[1]) // 64
    {
        smeter = 0;
    }
    else if (rssi <= threTable[2])
    {
        smeter = 1;
    }
    else if (rssi <= threTable[3])
    {
        smeter = 2;
    }
    else if (rssi <= threTable[4])
    {
        smeter = 3;
    }
    else if (rssi <= threTable[5])
    {
        smeter = 4;
    }
    else if (rssi <= threTable[6])
    {
        smeter = 5;
    }
    else if (rssi <= threTable[7])
    {
        smeter = 6;
    }
    else if (rssi <= threTable[8])
    {
        smeter = 7;
    }
    else if (rssi <= threTable[9])
    {
        smeter = 8;
    }
    else if (rssi <= threTable[10]) // 125
    {
        smeter = 9;
    }
    else
    {
        smeter = 10;
    }
    return smeter;
}

void BK4802SetPower(uint8_t level)
{
    uint16_t readVal = 0;
    if (level > 2)
    {
        level = 2;
    }

    // 设置为最低功率
    readVal = BK4802GetDynamicCfg(8); // 寄存器8的B07~B05位设置
    if (readVal == 0)
    {
        log_e("Failed to read register 8");
        return;
    }

    // 量化功率,分为3档
    switch (level)
    {
    case 0:
        readVal &= ~(0x07 << 5); // 清除B07~B05位
        readVal |= (0x01 << 5);
        break;
    case 1:
        // 设置为中等功率
        readVal &= ~(0x07 << 5); // 清除B07~B05位
        readVal |= (0x02 << 5);  // 设置B05位
        break;
    case 2:
        // 设置为最高功率
        readVal |= (0x07 << 5); // 设置B06~B05位
        break;
    }
    log_d("setting reg 8 to 0x%04X", readVal);
    BK4802WriteReg(8, readVal);
    BK4802SetDynamicCfg(8, readVal);
}

void BK4802SetFreqOffsetHz(float offsetHz)
{
    g_freqOffsetMHz = offsetHz / 1e6f;
    log_i("Set Freq Offset: %.2f Hz (内部=%.6f MHz)", offsetHz, g_freqOffsetMHz);
}

void BK4802SetFreqOffsetPPM(float ppm)
{
    // ppm = 1e-6，相对频率误差
    float offsetMHz = g_lastUserFreqMHz * (ppm * 1e-6f);
    g_freqOffsetMHz = offsetMHz;
    log_i("Set Freq Offset by PPM: ppm=%.2f -> off=%.6f MHz (lastUser=%.6f MHz)", ppm, g_freqOffsetMHz, g_lastUserFreqMHz);
}

float BK4802GetFreqOffsetHz(void)
{
    return g_freqOffsetMHz * 1e6f;
}

float BK4802GetFreqOffsetMHz(void)
{
    return g_freqOffsetMHz;
}

float BK4802QuantizeFreq(float freqMHz, float stepHz)
{
    if (stepHz <= 0.0f)
    {
        return freqMHz; // 不量化
    }
    float stepMHz = stepHz / 1e6f;
    float q = (float)((int)((freqMHz / stepMHz) + 0.5f)) * stepMHz;
    return q;
}

void BK4802FlushWithStep(float reqFreqMHz, float stepHz)
{
    float qFreq = BK4802QuantizeFreq(reqFreqMHz, stepHz);
    BK4802Flush(qFreq);
}

void BK4802IFGainLevel(uint8_t level)
{
    // IF Gain Level 0~7
    uint16_t writeLevel = 0;
    if (level > 7)
    {
        level = 7;
    }
    // 寄存器7的 15~13位设置,只读,默认值0xED00 (最大增益)
    dynamicConfig[0].value &= ~(0x07 << 13); // 清除B15~B13位
    dynamicConfig[0].value |= (level << 13);
    writeLevel = dynamicConfig[0].value;
    BK4802WriteReg(7, writeLevel);
    log_d("setting IF Gain Level to %d, reg7=0x%04X", level, writeLevel);
}
uint8_t BK4802GetCurThre(void)
{
    return softRSSIThre;
}