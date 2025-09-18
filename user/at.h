#ifndef __AT_H__
#define __AT_H__
#include "main.h"
#include "components.h"
#include "atCommand.h"
#include "SHARECom.h"
#define UART_RECV_BUF_SIZE 128
void atInit(SHARECom *SHARECom);
void atTask(void);
void atCtrl(xBool isEnable);
#endif