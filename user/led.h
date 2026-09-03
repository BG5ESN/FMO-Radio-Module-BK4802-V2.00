#ifndef __LED_H__
#define __LED_H__
#include "components.h"
typedef enum
{
    E_LED_ON,
    E_LED_OFF,
    E_LED_BLINK,//
    E_LED_BLINK_WITH_COUNT,//带计数的闪烁模式，例如闪烁10次,等待一段时间后重复,输入onTime\offTime\Count\WaitTime 这种可以用来指示发射状态等
} LedPattern_t;
typedef struct
{
    uint32_t onTime;
    uint32_t offTime;
} LedPattern_Blink_Parms;

typedef struct
{
    uint32_t onTime;
    uint32_t offTime;
    uint32_t count;
    uint32_t waitTime;
} LedPattern_BlinkCount_Parms;

typedef union
{
    LedPattern_Blink_Parms blink;
    LedPattern_BlinkCount_Parms blinkCount;
} LedPattern_Parms;

#define LED_ON() ledSet(E_LED_ON, NULL)
#define LED_OFF() ledSet(E_LED_OFF, NULL)

#define LED_BLINK(onMillis, offMillis)     \
    do                                     \
    {                                      \
        LedPattern_Parms tmpParm;          \
        tmpParm.blink.onTime = onMillis;   \
        tmpParm.blink.offTime = offMillis; \
        ledSet(E_LED_BLINK, &tmpParm);     \
    } while (0)

#define LED_BLINK_COUNT(vOnMillis, vOffMillis, vCount, vWaitMillis) \
    do                                                          \
    {                                                           \
        LedPattern_Parms tmpParm;                               \
        tmpParm.blinkCount.onTime = vOnMillis;                   \
        tmpParm.blinkCount.offTime = vOffMillis;                 \
        tmpParm.blinkCount.count = vCount;                       \
        tmpParm.blinkCount.waitTime = vWaitMillis;               \
        ledSet(E_LED_BLINK_WITH_COUNT, &tmpParm);               \
    } while (0)

void ledInit(void);
void ledSet(LedPattern_t pattern, LedPattern_Parms *parms);
void ledTask(void); // put in a 10ms loop task
#endif