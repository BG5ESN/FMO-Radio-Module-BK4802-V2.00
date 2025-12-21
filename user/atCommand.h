/*
 *AT Command Module Proc Headfile
 *Author:xifengzui AKA BG5ESN
 *Date:2024-10-21
 */
#ifndef __AT_CMD_H__
#define __AT_CMD_H__

#include "components.h"
#include "SHARECom.h"
#include "radioConvert.h"
#define VERSION "V1"        // protocol version : V1 DO NOT CHANGE, IT IS USED TO IDENTIFY THE PROTOCOL
#define NAME "FMO-BP-V1.0.10" // max 16 char
#define E_AT_BAND_CAP_144_148 0x01
#define E_AT_BAND_CAP_430_440 0x02
#define E_AT_BAND_CAP_50_54 0x04
#define E_AT_BAND_CAP_28_29P7 0x08
#define E_AT_BAND_CAP_24P89_24P99 0x10
#define E_AT_BAND_CAP_21_21P45 0x20
#define E_AT_BAND_CAP_18P068_18P168 0x40
#define E_AT_BAND_CAP_14_14P35 0x80
#define E_AT_BAND_CAP_10P1_10P15 0x100
#define E_AT_BAND_CAP_7_7P2 0x200
#define E_AT_BAND_CAP_5P3515_5P3665 0x400
#define E_AT_BAND_CAP_3P5_3P9 0x800
#define E_AT_BAND_CAP_1P8_2 0x1000

#define TX_PWR_LOW 0
#define TX_PWR_MID 1
#define TX_PWR_HIGH 2

typedef enum
{
    E_AT_CMD_NONE,
    E_AT_CMD_TEST,
    E_AT_CMD_NAME,
    E_AT_CMD_VER,
    E_AT_CMD_BANDCAP,
    E_AT_CMD_SQL,
    E_AT_CMD_TXFREQ,
    E_AT_CMD_RXFREQ,
    E_AT_CMD_RXVOL,
    E_AT_CMD_TXVOL,
    E_AT_CMD_TCTCSS,
    E_AT_CMD_RCTCSS,
    E_AT_CMD_TXPWR,
    E_AT_CMD_FREQTUNE,
    E_AT_CMD_SMETER,
    E_AT_CMD_RF, // RF ENABLE / DISABLE
    E_AT_CMD_SYS, // System operations e.g. RESET
    E_AT_CMD_MAX,
} ATCmd;

// UART port define
typedef uint16_t (*ATCmdRecvBytesCb)(uint8_t *bytes, uint16_t len); // 接收数据回调函数
typedef void (*ATCmdSendBytesCb)(uint8_t *bytes, uint16_t len);     // 发送数据回调函数
typedef struct
{
    ATCmdRecvBytesCb recvBytes; // receive data from user interface
    ATCmdSendBytesCb sendBytes; // send data to user interface
} ATCmdPort;

// AT Command Port init
void ATCmdInit(const ATCmdPort *port);

// put in mainloop will handler the AT command every 10ms
// if get cmd and user not calling ATTryGetCmd, the cmd will hold until the ATCmdSendResult is called
void ATCmdHandler(SHARECom *com);

// when receive the AT command, call this function to get the command,get args from the SHARECom
// multi operation will be pushed into a queue,you can call it many times to get all the command changes
// 当接收到 AT 命令时，调用此函数以获取命令，并从 SHARECom 获取参数
// 多次操作将被推入队列，可以多次调用以获取所有命令更改
ATCmd FetchATCmd(void);

#endif
