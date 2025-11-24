#include "wdt.h"

static IWDG_HandleTypeDef hiwdg;

HAL_StatusTypeDef WDT_Init(const WDT_Config *cfg)
{
    if (cfg == NULL) return HAL_ERROR;
    if (cfg->reload > 0x0FFF) return HAL_ERROR;

    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = cfg->prescaler;
    hiwdg.Init.Reload = cfg->reload;

    return HAL_IWDG_Init(&hiwdg);
}

HAL_StatusTypeDef WDT_Feed(void)
{
    return HAL_IWDG_Refresh(&hiwdg);
}
uint16_t WDT_CalcReload(uint32_t lsiHz, uint32_t timeoutMs, uint32_t prescalerDiv)
{
    /* 计数周期 = prescalerDiv / lsiHz 秒; reload = timeout / 计数周期 - 1 */
    uint32_t ticks = ((uint64_t)timeoutMs * lsiHz) / (prescalerDiv * 1000ULL);
    if (ticks == 0) return 0; /* 最小 */
    if (ticks > 0x0FFF) ticks = 0x0FFF; /* 最大裁剪 */
    return (uint16_t)ticks;
}