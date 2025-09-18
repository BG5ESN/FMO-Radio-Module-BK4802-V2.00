#include "softI2C.h"
/*
typedef struct
{
    // I2C总线的引脚控制
    void (*sclIn)(void);
    void (*sclOut)(void);
    void (*sclHigh)(void);
    void (*sclLow)(void);
    void (*sdaIn)(void);
    void (*sdaOut)(void);
    void (*sdaHigh)(void);
    void (*sdaLow)(void);
    unsigned char (*sdaRead)(void);
    void (*delay)(void);

    unsigned char slaveReadAddress;//从机读地址
    unsigned char slaveWriteAddress;//从机写地址

    //状态指示
    unsigned char isErr; //指示是否出错，出错置为1，
}SoftI2cHandler;
*/
void softI2cInit(SoftI2cHandler *handler)
{
    handler->sclOut();
    handler->sdaOut();
    handler->sclHigh();
    handler->sdaHigh();
}
void softI2cStart(SoftI2cHandler *handler)
{
    handler->sdaHigh();
    handler->sclHigh();
    handler->delay();
    handler->sdaLow();
    handler->delay();
    handler->sclLow();
}
void softI2cStop(SoftI2cHandler *handler)
{
    handler->sdaLow();
    handler->sclHigh();
    handler->delay();
    handler->sdaHigh();
    handler->delay();
}

void softI2cAck(SoftI2cHandler *handler)
{
    handler->sdaLow();
    handler->sclHigh();
    handler->delay();
    handler->sclLow();
    handler->delay();
}
void softI2cNack(SoftI2cHandler *handler)
{
    handler->sdaHigh();
    handler->sclHigh();
    handler->delay();
    handler->sclLow();
    handler->delay();
}
unsigned char softI2cWaitAck(SoftI2cHandler *handler)
{
    unsigned char ack;
    handler->sdaIn();
    handler->sdaHigh();
    handler->delay();
    handler->sclHigh();
    ack = handler->sdaRead();
    handler->sclLow();
    handler->delay();
    handler->sdaOut();
    return ack;
}
void softI2cSendByte(SoftI2cHandler *handler, unsigned char data)
{
    unsigned char i;
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
        {
            handler->sdaHigh();
        }
        else
        {
            handler->sdaLow();
        }
        data <<= 1;
        handler->delay();
        handler->sclHigh();
        handler->delay();
        handler->sclLow();
        handler->delay();
    }
}
unsigned char softI2cReadByte(SoftI2cHandler *handler)
{
    unsigned char i, data;
    handler->sdaIn();
    for (i = 0; i < 8; i++)
    {
        handler->sclHigh();
        data <<= 1;
        handler->delay();
        if (handler->sdaRead())
        {
            data |= 0x01;
        }
        handler->delay();
        handler->sclLow();
        handler->delay();
    }
    handler->sdaOut();
    return data;
}
// 读取指定地址的数据
unsigned char softI2cReadByteFromAddr(SoftI2cHandler *handler, unsigned char devAddr7Bit, unsigned char reg)
{
    unsigned char data;
    unsigned char readAddr = devAddr7Bit << 1 | 0x01;
    unsigned char writeAddr = devAddr7Bit << 1 | 0x00;
    handler->isErr = 0;
    softI2cStart(handler);
    softI2cSendByte(handler, writeAddr);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return 0;
    }
    softI2cSendByte(handler, reg);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return 0;
    }
    softI2cStart(handler);
    softI2cSendByte(handler, readAddr);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return 0;
    }
    data = softI2cReadByte(handler);
    softI2cNack(handler);
    softI2cStop(handler);
    return data;
}

// 写入指定地址的数据
void softI2cWriteByteToAddr(SoftI2cHandler *handler, unsigned char devAddr7Bit, unsigned char reg, unsigned char data)
{
    handler->isErr = 0;
    // unsigned char readAddr = devAddr7Bit << 1 | 0x01;
    unsigned char writeAddr = devAddr7Bit << 1 | 0x00;
    softI2cStart(handler);
    softI2cSendByte(handler, writeAddr);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }
    softI2cSendByte(handler, reg);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }
    softI2cSendByte(handler, data);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }
    softI2cStop(handler);
}

// 读取指定地址的数据 word
unsigned int softI2cReadWordFromAddr(SoftI2cHandler *handler, unsigned char devAddr7Bit, unsigned char reg)
{
    unsigned int data;
    unsigned char readAddr = devAddr7Bit << 1 | 0x01;
    unsigned char writeAddr = devAddr7Bit << 1 | 0x00;
    handler->isErr = 0;
    softI2cStart(handler);
    softI2cSendByte(handler, writeAddr);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return 0;
    }
    softI2cSendByte(handler, reg);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return 0;
    }
    softI2cStart(handler);
    softI2cSendByte(handler, readAddr);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return 0;
    }
    data = softI2cReadByte(handler);
    softI2cAck(handler);
    data <<= 8;
    data |= softI2cReadByte(handler);
    softI2cNack(handler);
    softI2cStop(handler);
    return data;
}
// 写入指定地处的数据 word
void softI2cWriteWordToAddr(SoftI2cHandler *handler, unsigned char devAddr7Bit, unsigned char reg, unsigned int data)
{
    handler->isErr = 0;
    // unsigned char readAddr = devAddr7Bit << 1 | 0x01;
    unsigned char writeAddr = devAddr7Bit << 1 | 0x00;
    softI2cStart(handler);
    softI2cSendByte(handler, writeAddr);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }
    softI2cSendByte(handler, reg);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }
    softI2cSendByte(handler, (unsigned char)(data >> 8));
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }
    softI2cSendByte(handler, (unsigned char)data);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }
    softI2cStop(handler);
}

void softI2cReadMultFromAddr(SoftI2cHandler *handler, unsigned char devAddr7Bit, unsigned char reg, unsigned char *buf, unsigned char count)
{
    unsigned char ii = 0;
    unsigned char readAddr = devAddr7Bit << 1 | 0x01;
    unsigned char writeAddr = devAddr7Bit << 1 | 0x00;
    handler->isErr = 0;
    softI2cStart(handler);
    softI2cSendByte(handler, writeAddr);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }
    softI2cSendByte(handler, reg);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }
    softI2cStart(handler);

    softI2cSendByte(handler, readAddr);

    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }

    for (ii = 0; ii < count - 1; ii++)
    {
        *buf = softI2cReadByte(handler);
        buf++;
        softI2cAck(handler);
    }

    *buf = softI2cReadByte(handler);
    softI2cNack(handler);
    softI2cStop(handler);
}

void softI2cWriteMultToAddr(SoftI2cHandler *handler, unsigned char devAddr7Bit, unsigned char reg, unsigned char *buf, unsigned char count)
{
    unsigned char ii = 0;
    handler->isErr = 0;
    // unsigned char readAddr = devAddr7Bit << 1 | 0x01;
    unsigned char writeAddr = devAddr7Bit << 1 | 0x00;

    softI2cStart(handler);
    softI2cSendByte(handler, writeAddr);

    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }

    softI2cSendByte(handler, reg);
    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }

    for (ii = 0; ii < count; ii++)
    {
        softI2cSendByte(handler, *buf);
        buf++;
        if (softI2cWaitAck(handler))
        {
            handler->isErr = 1;
            return;
        }
    }

    softI2cStop(handler);
}
void softI2cWriteMultData(SoftI2cHandler *handler, unsigned char devAddr7Bit, unsigned char *buf, unsigned char count)
{
    unsigned char ii = 0;
    handler->isErr = 0;
    // unsigned char readAddr = devAddr7Bit << 1 | 0x01;
    unsigned char writeAddr = devAddr7Bit << 1 | 0x00;

    softI2cStart(handler);
    softI2cSendByte(handler, writeAddr);

    if (softI2cWaitAck(handler))
    {
        handler->isErr = 1;
        return;
    }
    for (ii = 0; ii < count; ii++)
    {
        softI2cSendByte(handler, *buf);
        buf++;
        if (softI2cWaitAck(handler))
        {
            handler->isErr = 1;
            return;
        }
    }
}
