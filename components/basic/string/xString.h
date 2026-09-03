#ifndef __XSTRING_H__
#define __XSTRING_H__
#include "xDef.h"
#include <string.h>
uint16_t xStringLen(char *str);
xBool xStringStartIdxFinder(char *rawStr, char *tgtStr, uint16_t *startIdx);
xBool xStringnCompare(char *str1, char *str2, uint16_t n);
xBool xStringCompare(char *str1, char *str2);
uint16_t xStringnCopy(char *dest, char *src, uint16_t n);
xBool xStringnToUint32(char *str, uint8_t strLen, uint32_t *value);
xBool xStringnToInt32(char *str, uint8_t strLen, int32_t *value);
xBool xStringnToFloat(char *str, uint8_t strLen, float *value);
xBool xStringnToHex(char *str, uint8_t strLen, uint32_t *value);//hex string to uint32
xBool xStringSeprate(char *src, char **sepPart, char sepPartNum, char *sepMark, int *actualSepNum);
xBool xStringSeprateWithLen(char *src, char **sepPart, uint16_t *sepLen, char sepPartNum, char *sepMark, int *actualSepNum);
uint16_t xStringUint32Toa(char *str, uint32_t value);
uint16_t xStringInt32Toa(char *str, int32_t value);
uint16_t xStringFloatToa(char *str, float value, uint8_t precision);
uint16_t xStringHexToa(char *str, uint32_t value);
#endif
