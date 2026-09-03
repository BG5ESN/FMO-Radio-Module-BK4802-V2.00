#include "components.h"
#include "main.h"
#include "osTimer.h"

TIM_HandleTypeDef Tim16Handle;

void osTimerInit()
{
    __HAL_RCC_TIM16_CLK_ENABLE();
    /* Set interrupt priority */
    HAL_NVIC_SetPriority(TIM16_IRQn, 0, 0);
    /* Enable TIM1 interrupt */
    HAL_NVIC_EnableIRQ(TIM16_IRQn);
    Tim16Handle.Instance = TIM16;
    Tim16Handle.Init.Period = 3200 - 1;                                  /* Auto-reload value */
    Tim16Handle.Init.Prescaler = 10 - 1;                               /* Prescaler of 1000-1 */
    Tim16Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             /* Clock not divided */
    Tim16Handle.Init.CounterMode = TIM_COUNTERMODE_UP;                   /* Up counting mode */
    Tim16Handle.Init.RepetitionCounter = 1 - 1;                          /* No repetition */
    Tim16Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; /* Auto-reload register not buffered */

    if (HAL_TIM_Base_Init(&Tim16Handle) != HAL_OK) /* Initialize TIM16 */
    {
        log_e("Error in initializing TIM16");
    }

    if (HAL_TIM_Base_Start_IT(&Tim16Handle) != HAL_OK)
    {
        log_e("Error in starting TIM16");
    }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM16)
    {
        SCH_Dispatch_IT();
    }
}
