#include "antennaPath.h"
#include "components.h"
#include "main.h"
static GPIO_InitTypeDef GPIO_InitStruct;
void antennaPathInit(void)
{
    // PB0: H PB1 L FILTER 
    // PB0: L PB1 H ATTENUATOR
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    antennaPathCtrl(ANTENNA_PATH_ATTENUATOR); // 默认使能衰减器，避免大功率烧毁LNA
}
void antennaPathCtrl(ANTENNA_PATH path)
{
    switch (path)
    {
        case ANTENNA_PATH_ATTENUATOR:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); // PB0 L
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);   // PB1 H
            break;
        case ANTENNA_PATH_FILTER:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);   // PB0 H
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET); // PB1 L
            break;
        default:
            // Handle error or unexpected path
            break;
    }
}