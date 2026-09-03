#include "misc.h"
#include "components.h"
#include "main.h"

static GPIO_InitTypeDef GPIO_InitStruct;
void miscInit(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    // 使能输入脚，PB7，高电平为使能，低电平为关闭，默认打开，上拉
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

xBool isEnableCom(void)
{
    // log_d("GPIOB7:%d", HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5));
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_SET ? xTrue : xFalse;
}
