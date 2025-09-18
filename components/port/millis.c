#include "py32f0xx.h"
uint32_t millis(void)
{
    return HAL_GetTick();
}
