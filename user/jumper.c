#include "jumper.h"
#include "components.h"
#include "main.h"
static GPIO_InitTypeDef GPIO_InitStruct;
static uint8_t jumperMode; // 默认模式为正常模式

void jumperInit(void)
{
    // 初始化 JUMP 1 PB0
    // 初始化 JUMP 2 PB1
    // 初始化 JUMP 3 PB2
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_15 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_Delay(200);

    jumperMode = E_JUMPER_MODE_NORMAL; // 默认模式
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET)
    {
        jumperMode |= 0x01;
    }
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET)
    {
        jumperMode |= 0x02;
    }
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12) == GPIO_PIN_RESET)
    {
        jumperMode |= 0x04;
    }
    log_d("Jumper mode: %d", jumperMode);
}

uint8_t getJumperMode(void)
{
    return jumperMode; // JumperMode仅在开机时初始化一次，后续不需要改变
}