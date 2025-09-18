#include "main.h"
#include "systemClock.h"
#include "components.h"
void systemClockInit(void)
{
    // Initialize the system clock
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    // step1 config cpu clock source
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE; /* Configure HSE, HSI, LSI, LSE */
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_22p12MHz; // 22.12MHz
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        log_e("Error in HAL_RCC_OscConfig");
    }
    // step2 config cpu clock
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1; /* Configure SYSCLK, HCLK, PCLK */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;                                      /* Configure system clock as PLL */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;                                             /* AHB clock not divided */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;                                              /* APB clock not divided */
    /* Configure clock source */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        log_e("Error in HAL_RCC_ClockConfig");
    }
}

#if (defined(__CC_ARM)) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
/**
 * @brief  writes a character to the usart
 * @param  ch
 *         *f
 * @retval the character
 */
int fputc(int ch, FILE *f)
{
    /* Send a byte to USART */
    SEGGER_RTT_Write(0, (char *)&ch, 1);
    return (ch);
}

/**
 * @brief  get a character from the usart
 * @param  *f
 * @retval a character
 */
int fgetc(FILE *f)
{
    int ch;
    /* Receive a byte from USART */
    SEGGER_RTT_Read(0, (char *)&ch, 1);
    return (ch);
}
#endif
