#ifndef __SOFT_I2C_H__
#define __SOFT_I2C_H__

/*i2c接口句柄*/
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
    unsigned char isErr; //指示是否出错，出错置为1，
}SoftI2cHandler;
unsigned char isSoftI2cErr(void);
void softI2cInit(SoftI2cHandler *handler);
void softI2cStart(SoftI2cHandler *handler);
void softI2cStop(SoftI2cHandler *handler);
void softI2cAck(SoftI2cHandler *handler);
void softI2cNack(SoftI2cHandler *handler);
unsigned char softI2cWaitAck(SoftI2cHandler *handler);
void softI2cSendByte(SoftI2cHandler *handler, unsigned char data);
unsigned char softI2cReadByte(SoftI2cHandler *handler);

// 读取指定地址的数据 byte
unsigned char softI2cReadByteFromAddr(SoftI2cHandler *handler,unsigned char devAddr7Bit, unsigned char reg);

// 写入指定地址的数据 byte
void softI2cWriteByteToAddr(SoftI2cHandler *handler,unsigned char devAddr7Bit, unsigned char reg, unsigned char data);

// 读取指定地址的数据 word
unsigned int softI2cReadWordFromAddr(SoftI2cHandler *handler,unsigned char devAddr7Bit, unsigned char reg);

// 写入指定地处的数据 word
void softI2cWriteWordToAddr(SoftI2cHandler *handler,unsigned char devAddr7Bit, unsigned char reg, unsigned int data);

// 读取指定地址的数据
void softI2cReadMultFromAddr(SoftI2cHandler *handler, unsigned char devAddr7Bit, unsigned char reg, unsigned char *buf, unsigned char count);

// 写入指定地址的数据
void softI2cWriteMultToAddr(SoftI2cHandler *handler, unsigned char devAddr7Bit, unsigned char reg, unsigned char *buf, unsigned char count);


// 开放接口，对指定硬件写入byte数据
void softI2cWriteMultData(SoftI2cHandler *handler, unsigned char devAddr7Bit, unsigned char *buf,unsigned char count);
#endif
