#include "led.h"
#include "wdt.h"
#include "main.h"
#undef LOG_TAG
#define LOG_TAG "LED"
// PB1->PA4
static GPIO_InitTypeDef GPIO_InitStruct;
static LedPattern_t localPattern = E_LED_OFF;
static LedPattern_Parms localParms;
void ledInit(void)
{
    // PB8 为发射LED灯指示
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
}

void ledSet(LedPattern_t pattern, LedPattern_Parms *parms)
{
    switch (pattern)
    {
    case E_LED_ON:
        localPattern = E_LED_ON;
        // log_i("led on");
        break;
    case E_LED_OFF:
        localPattern = E_LED_OFF;
        // log_i("led off");
        break;
    case E_LED_BLINK:
        localPattern = E_LED_BLINK;
        // log_i("blink on:%d off:%d", parms->blink.onTime, parms->blink.offTime);
        localParms.blink.onTime = parms->blink.onTime;
        localParms.blink.offTime = parms->blink.offTime;
        break;
    case E_LED_BLINK_WITH_COUNT:
        localPattern = E_LED_BLINK_WITH_COUNT;
        // log_i("blink count on:%d off:%d count:%d wait:%d",
        //       parms->blinkCount.onTime, parms->blinkCount.offTime,
        //       parms->blinkCount.count, parms->blinkCount.waitTime);
        localParms.blinkCount.onTime = parms->blinkCount.onTime;
        localParms.blinkCount.offTime = parms->blinkCount.offTime;
        localParms.blinkCount.count = parms->blinkCount.count;
        localParms.blinkCount.waitTime = parms->blinkCount.waitTime;
        break;
    default:
        break;
    }
}

void ledTask(void)
{
    WDT_Kick(); // 喂狗
    if (localPattern == E_LED_ON)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    }
    else if (localPattern == E_LED_OFF)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    }
    else if (localPattern == E_LED_BLINK)
    {
        static uint8_t state = 0;
        static uint32_t lastTime = 0;
        uint32_t currentTime = millis();
        // log_i("Current time: %lu, Last time: %lu, State: %d,onTime:%d,offTime:%d",
        //       currentTime, lastTime, state, localParms.blink.onTime, localParms.blink.offTime);
        if (state == 1)
        {

            if (currentTime >= (localParms.blink.onTime + lastTime))
            {
                // log_i("blink on");
                lastTime = currentTime;
                state = 0;
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
            }
        }
        else
        {
            if (currentTime >= (localParms.blink.offTime + lastTime))
            {
                // log_i("blink off");
                lastTime = currentTime;
                state = 1;
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
            }
        }
    }
    else if (localPattern == E_LED_BLINK_WITH_COUNT)
    {
        static uint8_t state = 0;
        static uint32_t lastTime = 0;
        static uint32_t currentCount = 0;
        static uint8_t inWaitPeriod = 0;
        uint32_t currentTime = millis();

        if (inWaitPeriod)
        {
            // 在等待期间LED保持关闭
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

            // 检查等待时间是否结束
            if (currentTime >= (localParms.blinkCount.waitTime + lastTime))
            {
                // 等待时间结束，重新开始闪烁
                lastTime = currentTime;
                currentCount = 0;
                inWaitPeriod = 0;
                state = 0; // 从关闭状态开始
            }
        }
        else if (state == 1)
        {
            // LED亮着
            if (currentTime >= (localParms.blinkCount.onTime + lastTime))
            {
                lastTime = currentTime;
                state = 0;
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

                // 完成一次完整的闪烁周期（亮->灭）
                currentCount++;

                // 检查是否达到指定闪烁次数
                if (currentCount >= localParms.blinkCount.count)
                {
                    inWaitPeriod = 1;
                    // 不需要重置currentCount，因为进入等待期后会重置
                }
            }
        }
        else
        {
            // LED灭着
            if (currentTime >= (localParms.blinkCount.offTime + lastTime))
            {
                lastTime = currentTime;
                state = 1;
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
            }
        }
    }
}