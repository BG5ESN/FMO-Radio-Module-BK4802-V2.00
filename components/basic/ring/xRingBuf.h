/*
 * ring Copy Buffer
 *Author:xifengzui AKA BG5ESN
 *Date:2024-10-21
 */

#ifndef __XRING_BUF_H__
#define __XRING_BUF_H__
#ifdef __cplusplus
extern "C"
{
#endif
    // 实现一个环形缓冲区
    typedef struct
    {
        unsigned char *buf; // 缓冲区指针
        unsigned int size;  // 缓冲区大小
        unsigned int in;    // 写指针
        unsigned int out;   // 读指针
    } xRingBuf_t;
    /*
     * 初始化环形缓冲区
     * rb: 环形缓冲区指针
     * buf: 环形缓冲区数据存储区指针
     * size: 环形缓冲区数据存储区大小
     *
     */
    void xRingBufInit(xRingBuf_t *rb, unsigned char *buf, unsigned int size);

    /*
     * 向环形缓冲区写入数据
     * rb: 环形缓冲区指针
     * buf: 数据存储区指针
     * len: 数据长度
     * 返回值: 成功写入的数据长度
     */

    unsigned int xRingBufPut(xRingBuf_t *rb, unsigned char *buf, unsigned int len);
    /*
     * 从环形缓冲区读取数据
     * rb: 环形缓冲区指针
     * buf: 数据存储区指针
     * len: 数据长度
     * 返回值: 成功读取的数据长度
     */
    unsigned int xRingBufGet(xRingBuf_t *rb, unsigned char *buf, unsigned int len);

    /*
     * 获取环形缓冲区有效数据长度
     * rb: 环形缓冲区指针
     * 返回值: 环形缓冲区有效数据长度
     */
    unsigned int xRingBufLen(xRingBuf_t *rb);

    /*
     * 获取环形缓冲区剩余空间
     * rb: 环形缓冲区指针
     * 返回值: 环形缓冲区剩余空间
     */

    unsigned int xRingBufFree(xRingBuf_t *rb);
    /*
     * 判断环形缓冲区是否已满
     * rb: 环形缓冲区指针
     * 返回值: 1表示已满，0表示未满
     */

    unsigned int xRingBufFull(xRingBuf_t *rb);
    /*
     * 判断环形缓冲区是否为空
     * rb: 环形缓冲区指针
     * 返回值: 1表示为空，0表示不为空
     */
    unsigned int xRingBufEmpty(xRingBuf_t *rb);

    /*
     * 清空环形缓冲区
     * rb: 环形缓冲区指针
     */
    void xRingBufClear(xRingBuf_t *rb);
    // 单元测试函数，用于测试环形缓冲区是否正常工作
    void xRingBufTest(void);
#ifdef __cplusplus
}
#endif
#endif
