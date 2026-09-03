/**
  ******************************************************************************
  * @file    system_py32f0xx.c
  * @author  MCU Application Team
  * @brief   CMSIS Cortex-M0+ Device Peripheral Access Layer System Source File
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) Puya Semiconductor Co.
  * All rights reserved.</center></h2>
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

#include "py32f0xx.h"

#if !defined  (HSE_VALUE)
#define HSE_VALUE    24000000U    /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
#define HSI_VALUE  8000000U   /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

#if !defined  (LSI_VALUE)
#define LSI_VALUE   32768U      /*!< Value of LSI in Hz*/
#endif /* LSI_VALUE */

#if !defined  (LSE_VALUE)
#define LSE_VALUE  32768U      /*!< Value of LSE in Hz*/
#endif /* LSE_VALUE */


/************************* Miscellaneous Configuration ************************/
/* Note: Following vector table addresses must be defined in line with linker
         configuration. */
/*!< Uncomment the following line if you need to relocate the vector table
     anywhere in Flash or Sram, else the vector table is kept at the automatic
     remap of boot address selected */
#define USER_VECT_TAB_ADDRESS

#if defined(USER_VECT_TAB_ADDRESS)
/*!< Uncomment the following line if you need to relocate your vector Table
     in Sram else user remap will be done in Flash. */
/* #define VECT_TAB_SRAM */
#if defined(VECT_TAB_SRAM)
#define VECT_TAB_BASE_ADDRESS   SRAM_BASE       /*!< Vector Table base address field.
                                                     This value must be a multiple of 0x200. */
#define VECT_TAB_OFFSET         0x00000000U     /*!< Vector Table base offset field.
                                                     This value must be a multiple of 0x200. */
#else
#define VECT_TAB_BASE_ADDRESS   FLASH_BASE      /*!< Vector Table base address field.
                                                     This value must be a multiple of 0x200. */
#define VECT_TAB_OFFSET         0x00000000U     /*!< Vector Table base offset field.
                                                     This value must be a multiple of 0x200. */
#endif /* VECT_TAB_SRAM */
#endif /* USER_VECT_TAB_ADDRESS */
/******************************************************************************/
/*----------------------------------------------------------------------------
  Clock Variable definitions
 *----------------------------------------------------------------------------*/
/* This variable is updated in three ways:
    1) by calling CMSIS function SystemCoreClockUpdate()
    2) by calling HAL API function HAL_RCC_GetHCLKFreq()
    3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
       Note: If you use this function to configure the system clock; then there
             is no need to call the 2 first functions listed above, since SystemCoreClock
             variable is updated automatically.
*/
uint32_t SystemCoreClock = HSI_VALUE;

const uint32_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint32_t APBPrescTable[8] =  {0, 0, 0, 0, 1, 2, 3, 4};
const uint32_t HSIFreqTable[8] = {4000000U, 8000000U, 16000000U, 22120000U, 24000000U, 4000000U, 4000000U, 4000000U};

/**
  * @brief  Setup the microcontroller system.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{ 
  /* Configure the Vector Table location -------------------------------------*/
#if defined(USER_VECT_TAB_ADDRESS)
  SCB->VTOR = VECT_TAB_BASE_ADDRESS | VECT_TAB_OFFSET; /* Vector Table Relocation */
#endif /* USER_VECT_TAB_ADDRESS */

#if defined(PY32F030PRE) || defined(PY32F003PRE) || defined(PY32F002APRE)
  /* Set the HSI clock to 8MHz by default */
  RCC->ICSCR = (RCC->ICSCR & 0xFFFF0000) | *(uint32_t *)(0x1FFF0F04);
#endif

#if defined(PY32F071PRE) || defined(PY32F072PRE)
  /* Set the HSI clock to 8MHz by default, Set the LSI clock to 32.768KHz by default*/
  RCC->ICSCR = (RCC->ICSCR & 0xFE000000) | *(uint32_t *)(0x1FFF3208) | ((*(uint32_t *)(0x1FFF3348))<<16);
#endif
}

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)             /* Get Core Clock Frequency      */
{
  uint32_t tmp;
  uint32_t hsidiv;
  uint32_t hsifs;
#if defined(RCC_PLL_SUPPORT)
  uint32_t pllsource;
  uint32_t pllmul;
  const uint8_t PLLMULTable[4] = {2, 3, 2, 2};
#endif

  /* Get SYSCLK source -------------------------------------------------------*/
  switch (RCC->CFGR & RCC_CFGR_SWS)
  {
  case RCC_CFGR_SWS_0:  /* HSE used as system clock */
    SystemCoreClock = HSE_VALUE;
    break;

  case (RCC_CFGR_SWS_1 | RCC_CFGR_SWS_0):  /* LSI used as system clock */
    SystemCoreClock = LSI_VALUE;
    break;
	
#if defined(RCC_LSE_SUPPORT)
  case RCC_CFGR_SWS_2:  /* LSE used as system clock */
    SystemCoreClock = LSE_VALUE;
    break;
#endif

#if defined(RCC_PLL_SUPPORT)
  case RCC_CFGR_SWS_1:  /* PLL used as system clock */
	pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC);
	pllmul = PLLMULTable[((RCC->PLLCFGR & RCC_PLLCFGR_PLLMUL)>>RCC_PLLCFGR_PLLMUL_Pos)];
	
    if (pllsource == RCC_PLLCFGR_PLLSRC_HSI) /* HSI used as PLL clock source */
    {
      hsifs = ((READ_BIT(RCC->ICSCR, RCC_ICSCR_HSI_FS)) >> RCC_ICSCR_HSI_FS_Pos);
      SystemCoreClock = pllmul * HSIFreqTable[hsifs];
    }
    else   /* HSE used as PLL clock source */
    {
      SystemCoreClock = pllmul * HSE_VALUE;
    }
    break;
#endif

  case 0x00000000U:  /* HSISYS used as system clock */
  default:           /* HSISYS used as system clock */
    hsifs = ((READ_BIT(RCC->ICSCR, RCC_ICSCR_HSI_FS)) >> RCC_ICSCR_HSI_FS_Pos);
    hsidiv = (1UL << ((READ_BIT(RCC->CR, RCC_CR_HSIDIV)) >> RCC_CR_HSIDIV_Pos));
    SystemCoreClock = HSIFreqTable[hsifs] / hsidiv;
    break;
  }
  /* Compute HCLK clock frequency --------------------------------------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos)];
  /* HCLK clock frequency */
  SystemCoreClock >>= tmp;
}

#if defined(PY32F030PRE) || defined(PY32F003PRE) || defined(PY32F002APRE)
#ifndef VECT_TAB_SRAM
#if (defined (__CC_ARM)) || (defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
extern int32_t $Super$$main(void);
uint32_t VECT_SRAM_TAB[48]__attribute__((section(".ARM.__at_0x20000000")));
/* re-define main function */
int $Sub$$main(void)
{
  uint8_t i;
  uint32_t *pFmcVect = (uint32_t *)(FLASH_BASE | VECT_TAB_OFFSET);
  for (i = 0; i < 48; i++)
  {
    VECT_SRAM_TAB[i] = pFmcVect[i];
  }

  SCB->VTOR = SRAM_BASE;

  $Super$$main();
  return 0;
}
#elif defined(__ICCARM__)
extern int32_t main(void);
/* __low_level_init will auto called by IAR cstartup */
extern void __iar_data_init3(void);
uint32_t VECT_SRAM_TAB[48] @SRAM_BASE;
int __low_level_init(void)
{
  uint8_t i;
  uint32_t *pFmcVect = (uint32_t *)(FLASH_BASE | VECT_TAB_OFFSET);
  /* call IAR table copy function. */
  __iar_data_init3();

  for (i = 0; i < 48; i++)
  {
    VECT_SRAM_TAB[i] = pFmcVect[i];
  }

  SCB->VTOR = SRAM_BASE;

  main();
  return 0;
}
#endif
#endif
#endif
