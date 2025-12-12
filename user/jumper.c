#include "jumper.h"
#include "components.h"
#include "main.h"
static GPIO_InitTypeDef GPIO_InitStruct;

uint8_t getJumpHex(void)
{
    uint8_t jumpHex = 0;
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET) // PIN 1
    {
        jumpHex |= 0x01;
    }
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) // PIN 2
    {
        jumpHex |= 0x02;
    }
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12) == GPIO_PIN_RESET) // PIN 3
    {
        jumpHex |= 0x04;
    }
    return jumpHex;
}

#if (DBUG_FUNCTION == ANTENNA_TEST)
static uint8_t antennaMode; // 默认模式为正常模式

AntennaMode getJumperModeEnum(uint8_t mode)
{
    switch (mode)
    {
    case 0:
        return E_ANTENNA_MODE_NORMAL; // 正常模式
    case 1:
        return E_ANTENNA_MODE_RX_ONLY; // 仅接收模式
    case 2:
        return E_ANTENNA_MODE_ATT_ONLY; // 仅衰减器模式
    default:
        return E_ANTENNA_MODE_NORMAL; // 默认模式
    }
}
uint8_t getAntennaTestMode(void)
{
    return antennaMode; // JumperMode仅在开机时初始化一次，后续不需要改变,避免因环境产生跳变
}
#elif (DBUG_FUNCTION == VOL_ADJ_TEST)
uint8_t bootVolume = 0; // 开机音量调节值
uint8_t getVolumeAdjTestValue(void)
{
    return bootVolume; // 开机音量调节值，仅在开机时读取一次
}
#endif


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
    HAL_Delay(200); // 稳定延时 avoid bounce
    uint8_t jumpHex = getJumpHex();
#if DBUG_FUNCTION == ANTENNA_TEST
    antennaMode = getJumperModeEnum(jumpHex); // 默认模式
    log_d("Antenna mode: %d", antennaMode);
#elif DBUG_FUNCTION == VOL_ADJ_TEST
    bootVolume = jumpHex & 0x07; // 仅低3位有效
    log_d("Volume adjust test value: %d", bootVolume);
#endif
}