#include "speaker.h"
#include "main.h"
static GPIO_InitTypeDef GPIO_InitStruct;
void speakerInit(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);//speaker off
}
void speakerPlay(xBool isPlay)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, isPlay ? GPIO_PIN_RESET : GPIO_PIN_SET);
}