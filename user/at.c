#include "at.h"
UART_HandleTypeDef UartHandle;
static SHARECom *atCOM = NULL;
static xRingBuf_t uartRecvBufHandler;
static uint8_t uartRecvBufData[UART_RECV_BUF_SIZE + 2];
static uint8_t uartIsrBuffer[64]; // 中断接收缓冲区
static bool isATEnable = xTrue;
uint16_t atRecvCb(uint8_t *bytes, uint16_t len)
{
    uint16_t tmp;
    tmp = xRingBufGet(&uartRecvBufHandler, bytes, len);
    return tmp;
}

void atSendCb(uint8_t *bytes, uint16_t len)
{
    HAL_UART_Transmit(&UartHandle, bytes, len, 1000);
}

uint32_t atMillisCb(void)
{
    return millis();
}

const static ATCmdPort atPort = {
    .recvBytes = atRecvCb,
    .sendBytes = atSendCb,
};

void atInit(SHARECom *SHARECom)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    xRingBufInit(&uartRecvBufHandler, uartRecvBufData, sizeof(uartRecvBufData));

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART2_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    UartHandle.Instance = USART2;
    UartHandle.Init.BaudRate = 19200;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits = UART_STOPBITS_1;
    UartHandle.Init.Parity = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode = UART_MODE_TX_RX;
    UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
    UartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&UartHandle);

    __HAL_UART_ENABLE_IT(&UartHandle, UART_IT_IDLE); // 使能空闲中断
    HAL_UART_Receive_IT(&UartHandle, uartIsrBuffer, sizeof(uartIsrBuffer));
    atCOM = SHARECom;
    ATCmdInit(&atPort);
}

uint32_t tCnt = 0;
void atTask(void)
{
    // test
    if (atCOM == NULL)
    {
        log_e("At COM is NULL!");
        return;
    }

    ATCmdHandler(atCOM);
}

void HAL_UART_IdleFrameDetectCpltCallback(UART_HandleTypeDef *UartHandle)
{
    // 将接受的数据放入环形缓冲区
    uint16_t len = UartHandle->RxXferSize - UartHandle->RxXferCount;
    log_d("uart recv len:%d,rxXferSize:%d,rxXferCount:%d", len, UartHandle->RxXferSize, UartHandle->RxXferCount);

    if (len > 0 && isATEnable)
    {
        xRingBufPut(&uartRecvBufHandler, uartIsrBuffer, len);
        memset(uartIsrBuffer, 0, sizeof(uartIsrBuffer));
    }
    else if (isATEnable == xFalse)
    {
        log_d("AT is disabled, ignore uart data.");
    }

    // 重新开启接收中断
    HAL_UART_Abort_IT(UartHandle);
    HAL_UART_Receive_IT(UartHandle, uartIsrBuffer, sizeof(uartIsrBuffer));
}

void atCtrl(xBool isEnable)
{
    isATEnable = isEnable;
}