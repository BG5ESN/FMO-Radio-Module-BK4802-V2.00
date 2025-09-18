#include "xRingBuf.h"
/*
 * 初始化环形缓冲区
 * rb: 环形缓冲区指针
 * buf: 环形缓冲区数据存储区指针
 * size: 环形缓冲区数据存储区大小
 *
 */
void xRingBufInit(xRingBuf_t *rb, unsigned char *buf, unsigned int size)
{
    rb->buf = buf;
    rb->size = size;
    rb->in = 0;
    rb->out = 0;
}

/*
 * 向环形缓冲区写入数据
 * rb: 环形缓冲区指针
 * buf: 数据存储区指针
 * len: 数据长度
 * 返回值: 成功写入的数据长度
 */

unsigned int xRingBufPut(xRingBuf_t *rb, unsigned char *buf, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++)
    {
        if (xRingBufFull(rb))
        {
            break;
        }
        rb->buf[rb->in] = buf[i];
        rb->in = (rb->in + 1) % rb->size;
    }
    return i;
}
/*
 * 从环形缓冲区读取数据
 * rb: 环形缓冲区指针
 * buf: 数据存储区指针
 * len: 数据长度
 * 返回值: 成功读取的数据长度
 */
unsigned int xRingBufGet(xRingBuf_t *rb, unsigned char *buf, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++)
    {
        if (xRingBufEmpty(rb))
        {
            break;
        }
        if (buf != 0)
        {
            buf[i] = rb->buf[rb->out];
        }
        rb->out = (rb->out + 1) % rb->size;
    }
    return i;
}

void xRingBufClear(xRingBuf_t *rb)
{
    rb->in = 0;
    rb->out = 0;
}

/*
 * 获取环形缓冲区有效数据长度
 * rb: 环形缓冲区指针
 * 返回值: 环形缓冲区有效数据长度
 */
unsigned int xRingBufLen(xRingBuf_t *rb)
{
    return (rb->in - rb->out + rb->size) % rb->size;
}

/*
 * 获取环形缓冲区剩余空间
 * rb: 环形缓冲区指针
 * 返回值: 环形缓冲区剩余空间
 */

unsigned int xRingBufFree(xRingBuf_t *rb)
{
    return (rb->out - rb->in + rb->size - 1) % rb->size;
}
/*
 * 判断环形缓冲区是否已满
 * rb: 环形缓冲区指针
 * 返回值: 1表示已满，0表示未满
 */
unsigned int xRingBufFull(xRingBuf_t *rb)
{
    return (rb->in + 1) % rb->size == rb->out;
}

/*
 * 判断环形缓冲区是否为空
 * rb: 环形缓冲区指针
 * 返回值: 1表示为空，0表示不为空
 */
unsigned int xRingBufEmpty(xRingBuf_t *rb)
{
    return rb->in == rb->out;
}

// 单元测试函数，用于测试环形缓冲区是否正常工作
void xRingBufTest(void)
{

    unsigned char xRingMem[100];
    // 生成一个随机数组
    unsigned char xRingTestDataPack1[3] = {0x01, 0x02, 0x03};
    // 生成第二个随机数组
    unsigned char xRingTestDataPack2[3] = {0x04, 0x05, 0x06};
    // 用于读取的缓存
    unsigned char xRingTestReadBuf[10];

    // 初始化环形缓冲区
    xRingBuf_t xRingBuf;
    xRingBufInit(&xRingBuf, xRingMem, sizeof(xRingMem));

    // 向环形缓冲区写入第一个测试数组
    xRingBufPut(&xRingBuf, xRingTestDataPack1, sizeof(xRingTestDataPack1));
    // 通过printf打印xRingbuf的剩余空间，有效数据长度 应该是97，3
    // printf("xRingBufFree:%d, xRingBufLen:%d\r\n", xRingBufFree(&xRingBuf), xRingBufLen(&xRingBuf));

    // 从环形缓冲区读取所有数据
    xRingBufGet(&xRingBuf, xRingTestReadBuf, xRingBufLen(&xRingBuf));
    // 打印读出的数据
    // printf("xRingTestReadBuf:%02x %02x %02x\r\n", xRingTestReadBuf[0], xRingTestReadBuf[1], xRingTestReadBuf[2]);

    // 向环形缓冲区写入第二个测试数组
    xRingBufPut(&xRingBuf, xRingTestDataPack2, sizeof(xRingTestDataPack2));
    // 读取1个字节
    xRingBufGet(&xRingBuf, xRingTestReadBuf, 1);
    // 打印读出的数据
    // printf("xRingTestReadBuf:%02x\r\n", xRingTestReadBuf[0]);
    // 通过printf打印xRingbuf的剩余空间，有效数据长度 应该是98，2
    // printf("xRingBufFree:%d, xRingBufLen:%d\r\n", xRingBufFree(&xRingBuf), xRingBufLen(&xRingBuf));
}
