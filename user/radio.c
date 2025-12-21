/*
 *Radio driver
 *Author:xifengzui AKA BG5ESN
 *Date:2024-11-8
 */

#include "main.h"
#include "radio.h"
#include "led.h"
#include "BK4802.h"
#include "antennaPath.h"
#include "components.h"
#include "SHARECom.h"
#include "speaker.h"
#include "jumper.h"
#include "SHARECom.h"
#include "def.h"
#undef TAG
#define TAG "RADIO"

static GPIO_InitTypeDef GPIO_InitStruct;
static float txFreq = 145.100;   // MHz
static float rxFreq = 145.100;   // MHz
static int32_t freqOffsetHz = 0; // 全局频偏(Hz)

#define RSSI_OVERLOAD_THRE 125

void radioInit(void)
{
    BK4802Init();

    // 初始化通讯脚
    //  PTT 发射脚 PB6，读取到高电平时，进行发射，默认下拉，避免干扰
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 音频对外输出脚 PB7，输出，高电平为有正在产生的音频信号，低电平为无音频信号
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
}

xBool radioGetPTT(void)
{
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) ? xTrue : xFalse;
}

void radioSetAudioOutput(xBool enable)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// 0~10
void radioSetAudioOutputLevel(uint8_t level)
{
    BK4802SetVolLevel(level);
}

// 0~10
uint8_t radioGetAudioOutputLevel(void)
{
    return BK4802GetVolLevel();
}

static uint8_t micLevel = 0;
void radioSetMicInputLevel(uint8_t level)
{
    // not implemented,current not support.
    micLevel = level;
    return;
}
uint8_t radioGetMicInputLevel(void)
{
    // not implemented,current not support.
    return micLevel; // fake value
}

// 0~10
void radioSetSQLLevel(uint8_t level)
{
    BK4802SetRSSIThre(level); // 0~9
}

// 0~10
uint8_t radioGetSQLLevel(void)
{
    return BK4802GetRSSIThre(); // 0~9
}

void radioSetTxFreq(float freq)
{
    txFreq = freq;
    if (BK4802IsTx())
    {
        BK4802Flush(freq);
    }
}

void radioSetRxFreq(float freq)
{
    rxFreq = freq;
    if (!BK4802IsTx())
    {
        BK4802Flush(freq);
    }
}
uint8_t radioGetSMeter(void)
{
    // 降低SMeter的读取频率,改为每500ms读取一次
    static uint32_t smeterPeriod = 0;
    // 读取SMeter
    static uint8_t smeter = 0;
    if (millis() < smeterPeriod)
    {
        return smeter;
    }
    smeterPeriod = millis() + 500; // 500ms

    smeter = BK4802GetSMeter();
    return smeter;
}

void radioApplyFreqTune(void)
{
    // 直接调用 BK4802 偏移接口
    BK4802SetFreqOffsetHz((float)freqOffsetHz);
    // 重新刷新当前状态
    if (BK4802IsTx())
    {
        BK4802Flush(txFreq);
    }
    else
    {
        BK4802Flush(rxFreq);
    }
}

void radioSetFreqTune(int32_t tuneHz)
{
    // 限制一个合理范围，防止误操作 (例如 ±50 kHz)
    if (tuneHz > 50000)
        tuneHz = 50000;
    else if (tuneHz < -50000)
        tuneHz = -50000;
    freqOffsetHz = tuneHz;
    radioApplyFreqTune();
}

void timelyResetBK4802(void)
{
    static uint32_t resetBK4802Period = 0;
    if (millis() < resetBK4802Period)
    {
        return;
    }
    resetBK4802Period = millis() + (1000 * 3600 * 6); //  每6小时重新设置BK4802 避免奇怪的断开问题
    BK4802Reset(rxFreq);
}

void radioTask(void)
{
    extern SHARECom COM; // 使用全局共享结构体中的 rfEnable 状态

    static uint8_t lastPTT = 0xFF;
    static uint8_t lastVout = 0xFF;
    static uint8_t lastEn = 0xFF;
    static uint8_t lastGainLevel = 7; // IF增益等级,默认最大值
    uint8_t vout = 0;
    uint8_t ptt = 0;
    uint8_t en = 0;
    WDT_Kick(); // 喂狗
    // 读取PTT状态
    ptt = radioGetPTT();
    if (ptt != lastPTT)
    {
        lastPTT = ptt;

        if (ptt) // 二次判断，可能被上面禁用
        {
#if (DBUG_FUNCTION == ANTENNA_TEST) // 启用天线路径测试功能后,根据跳线状态决定天线路径
            if (getAntennaTestMode() == E_ANTENNA_MODE_RX_ONLY)
            {
                // DO Nothing
                log_d("PTT ON RX_ONLY");
                LED_BLINK(100, 3000);
            }
            else if (COM.rfEnable == 0)
            {
                log_w("PTT ignored: RF DISABLED via AT+RF");
                LED_BLINK(100, 1500); // 慢闪表示被禁止
            }
            else if (getAntennaTestMode() == E_ANTENNA_MODE_ATT_ONLY)
            {
                antennaPathCtrl(ANTENNA_PATH_ATTENUATOR); // 正常一直在衰减器挡位
                log_d("PTT ON ATT_ONLY");

                HAL_Delay(100); // 等待衰减器稳定
                BK4802Tx(txFreq);
                LED_BLINK(100, 500);
            }
            else if (getAntennaTestMode() == E_ANTENNA_MODE_NORMAL)
            {
                log_d("PTT ON");
                antennaPathCtrl(ANTENNA_PATH_FILTER); // UHF
                HAL_Delay(100);                       // 等待衰减器稳定
                BK4802Tx(txFreq);
                LED_ON();
            }
#else // 未启用天线路径测试,收发工作模式
            log_d("PTT ON");
            antennaPathCtrl(ANTENNA_PATH_FILTER); // UHF
            HAL_Delay(100);                       // 等待衰减器稳定
            BK4802Tx(txFreq);
            LED_ON();
#endif
            speakerPlay(xTrue);
        }
        else
        {
            // log_d("PTT OFF:%f", rxFreq);
            antennaPathCtrl(ANTENNA_PATH_ATTENUATOR); // 打开衰减器
            BK4802Rx(rxFreq);
            speakerPlay(xFalse);
            LED_BLINK_COUNT(200, 200, getJumpHex() + 1, 3000);
        }
    }

    if (!ptt) // 发射时，不进行接收信号判定
    {
        // 判定RSSI和SNR是否满足条件,并触发音频发送
        // TODO: 判定合适的值
        uint8_t rxExist = BK4802IsRx();
        if (BK4802IsError())
        {
            BK4802Reset(rxFreq);
        }
        else
        {
            timelyResetBK4802();
        }

        if (rxExist)
        {
            vout = 1;
        }
        else
        {
            vout = 0;
        }

        if (vout != lastVout)
        {
            lastVout = vout;
            if (vout)
            {
                // log_i("Audio ON");
                // log_i("RSSI:%d,SNR:%d", rssi, snr);
                radioSetAudioOutput(xTrue);
                LED_BLINK(50, 50);
            }
            else
            {
                // log_i("Audio OFF");
                radioSetAudioOutput(xFalse);
                LED_BLINK_COUNT(200, 200, getJumpHex() + 1, 3000);
            }
        }

        // 自动增益控制逻辑
        // 解释：如果在接收状态且RSSI达到最大值，则降低IF增益等级以降低过载提示，如果到0db的时候，还过载，这意味着是真正的超过了系统承受能力。
        //  解释：如果在非接收状态且经过一定时间没有接收到信号，则逐步提升IF增益等级以提高灵敏度。
        if (rxExist)
        {
            uint8_t rssi = 0;
            rssi = BK4802RSSIRead();

            if (rssi <= BK4802GetCurThre()) // 如果小于了当前的RSSI阈值,则提升增益
            {
                if (lastGainLevel < 7)
                {
                    lastGainLevel++;
                    BK4802IFGainLevel(lastGainLevel);
                    log_d("AGC: Increase Gain Level to %d", lastGainLevel);
                }
            }
            else if (rssi > RSSI_OVERLOAD_THRE) // 最大增益值
            {
                if (lastGainLevel > 0)
                {
                    lastGainLevel--;
                    BK4802IFGainLevel(lastGainLevel);
                    log_d("AGC: Decrease Gain Level to %d", lastGainLevel);
                }
            }
        }
        else
        {
            if (lastGainLevel != 7)
            {
                lastGainLevel = 7; // 直接提升到最大值
                BK4802IFGainLevel(lastGainLevel);
                log_d("AGC: Reset Gain Level to %d", lastGainLevel);
            }
        }
    }
}
void radioSetPower(uint8_t level)
{
    if (level > 2)
    {
        level = 2; // 限制最大值为2
    }
    BK4802SetPower(level);
}