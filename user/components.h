#ifndef __COMPONENT_H__
#define __COMPONENT_H__
#include "xDef.h"
#include "xRingBuf.h"
#include "xString.h"
#include "xMath.h"
#include "elog.h"
#include "SEGGER_RTT.h"
#include "millis.h"
#include "Sch51.h"
#include "pid.h"
#include "softI2C.h"//软件iic通用代码
void componentInit(void);
#endif
