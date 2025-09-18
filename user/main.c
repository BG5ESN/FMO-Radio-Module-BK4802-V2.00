#include "main.h"
#include "SHARECom.h"
#include "systemClock.h"
#include "osTimer.h"
#include "components.h"
#include "at.h"
#include "antennaPath.h"
#include "radio.h"
#include "speaker.h"
#include "led.h"
#include "misc.h"
#include "jumper.h"
#undef LOG_TAG
#define LOG_TAG "MAIN"

// at com init
SHARECom COM =
    {
        .bandCap = E_AT_BAND_CAP_144_146 | E_AT_BAND_CAP_430_440,
        .freqTune = 0,
        .rCTCSS = 0,
        .tCTCSS = 0,
        .rxVol = 5,
        .txVol = 5,
        .sql = 3,
        .txFreq = 145.100,
        .rxFreq = 145.100,
        .txPwr = TX_PWR_LOW,
        .ver = VERSION};

// 同步任务，将AT的COM中产生的各种指令，同步至其他模块
void syncInit(void)
{
  radioSetFreqTune(COM.freqTune);      // 微调频率，用于校准中心频点，如果有需要的话，可以在AT指令中调整
  radioSetAudioOutputLevel(COM.txVol); // 设置音频输出电平
  radioSetMicInputLevel(COM.rxVol);    // 设置麦克风输入电平
  radioSetTxFreq(COM.txFreq);          // 设置发射频率
  radioSetRxFreq(COM.rxFreq);          // 设置接收频率
  radioSetSQLLevel(COM.sql);           // 设置静噪电平
  radioSetPower(COM.txPwr);            // 设置发射功率
}
void syncTask(void)
{
  atCtrl(isEnableCom());         // 控制AT指令的开关，是否启用AT指令
  COM.smeter = radioGetSMeter(); // 获取信号强度
  // 指令设置
  ATCmd atCmd = FetchATCmd();
  // 更新COM的实时数据
  if (atCmd == E_AT_CMD_NONE)
  {
    return;
  }
  else if (atCmd == E_AT_CMD_SQL)
  {
    log_d("setting SQL:%d", COM.sql);
    radioSetSQLLevel(COM.sql); // 设置静噪电平
  }
  else if (atCmd == E_AT_CMD_TXFREQ)
  {
    log_d("setting TX freq:%f", COM.txFreq);
    radioSetTxFreq(COM.txFreq); // 设置发射频率
  }
  else if (atCmd == E_AT_CMD_RXFREQ)
  {
    log_d("setting RX freq:%f", COM.rxFreq);
    radioSetRxFreq(COM.rxFreq); // 设置接收频率
  }
  else if (atCmd == E_AT_CMD_RXVOL)
  {
    log_d("setting RX volume");
    radioSetMicInputLevel(COM.rxVol); // 设置麦克风输入电平
  }
  else if (atCmd == E_AT_CMD_TXVOL)
  {
    log_d("setting TX volume");
    radioSetAudioOutputLevel(COM.txVol); // 设置音频输出电平
  }
  else if (atCmd == E_AT_CMD_TCTCSS)
  {
    log_d("setting TX CTCSS");
    log_d("not support");
  }
  else if (atCmd == E_AT_CMD_RCTCSS)
  {
    log_d("setting RX CTCSS");
    log_d("not support");
  }
  else if (atCmd == E_AT_CMD_TXPWR)
  {
    log_d("setting TX power %d", COM.txPwr);
    radioSetPower(COM.txPwr); // 设置发射功率
  }
  else if (atCmd == E_AT_CMD_FREQTUNE)
  {
    log_d("setting freq tune");
    radioSetFreqTune(COM.freqTune); // 微调频率，用于校准中心频点，如果有需要的话，可以在AT指令中调整
  }
}

extern void BK4802DebugTask(void);

int main(void)
{
  HAL_Init();
  miscInit();
  ledInit();
  HAL_Delay(1000);
  componentInit();
  systemClockInit();
  speakerInit();
  log_d("NFM Module V1.00 AT Command");
  jumperInit();
  radioInit();
  antennaPathInit(); // 初始化天线路径
  atInit(&COM);
  syncInit();
  osTimerInit();

  SCH_Add_Task(atTask, 0, 10);
  SCH_Add_Task(radioTask, 0, 10);
  SCH_Add_Task(ledTask, 0, 10);
  SCH_Add_Task(syncTask, 0, 100);
  SCH_Add_Task(BK4802DebugTask, 1000, 1000);
  while (1)
  {
    SCH_Dispatch_Tasks();
  }
}
