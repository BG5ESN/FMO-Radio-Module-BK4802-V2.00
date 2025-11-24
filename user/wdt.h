#ifndef __WDT_H
#define __WDT_H

#include "py32f0xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t prescaler; /* 使用 IWDG_PRESCALER_x 枚举 */
    uint32_t reload;    /* 0~0x0FFF */
} WDT_Config;

HAL_StatusTypeDef WDT_Init(const WDT_Config *cfg);
HAL_StatusTypeDef WDT_Feed(void);

#define WDT_Kick() (WDT_Feed())

uint16_t WDT_CalcReload(uint32_t lsiHz, uint32_t timeoutMs, uint32_t prescalerDiv);

#ifdef __cplusplus
}
#endif

#endif /* __WDT_H */
