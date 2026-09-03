#include "string.h"
#include "xString.h"
#include "xDef.h"
#include "stdio.h"
#include "elog.h"
uint16_t xStringLen(char *str)
{
    // return the length of the xString
    return strlen(str);
}

xBool xStringStartIdxFinder(char *rawStr, char *tgtStr, uint16_t *startIdx)
{
    // try find the tgtStr in the rawStr and return the start index of the tgtStr
    // return xTrue if found,otherwise return xFalse
    char *p = strstr(rawStr, tgtStr);
    if (p == NULL)
    {
        return xFalse;
    }

    *startIdx = p - rawStr;
    return xTrue;
}

xBool xStringnCompare(char *str1, char *str2, uint16_t n)
{
    // compare the two xString
    // return xTrue if the two xString is the same,otherwise return xFalse
    if (str1 == NULL || str2 == NULL)
    {
        return xFalse;
    }
    if (n == 0)
    {
        return xFalse;
    }
    if (strncmp(str1, str2, n) == 0)
    {
        return xTrue;
    }
    return xFalse;
}

xBool xStringCompare(char *str1, char *str2)
{
    // compare the two xString
    // return xTrue if the two xString is the same,otherwise return xFalse
    if (strcmp(str1, str2) == 0)
    {
        return xTrue;
    }
    return xFalse;
}

uint16_t xStringnCopy(char *dest, char *src, uint16_t n)
{
    // copy the n byte from src to dest
    // return the length of the copied xString
    strncpy(dest, src, n);
    return n;
}

uint16_t xStringCopy(char *dest, char *src)
{
    // copy the xString from src to dest
    // return the length of the copied xString
    strcpy(dest, src);
    return strlen(src);
}

xBool xStringnToUint32(char *str, uint8_t strLen, uint32_t *value)
{
    // convert string to int32
    // return xTrue if success,otherwise return xFalse
    //   *value = atoi(str);
    uint8_t i = 0;

    *value = 0;

    for (i = 0; i < strLen; i++)
    {
        if (str[i] >= '0' && str[i] <= '9')
        {
            *value = *value * 10 + (str[i] - '0');
        }
        else
        {
            return xFalse;
        }
    }
    return xTrue;
}

xBool xStringnToInt32(char *str, uint8_t strLen, int32_t *value)
{
    // convert string to int32
    // return xTrue if success,otherwise return xFalse
    //   *value = atoi(str);
    uint8_t i = 0;
    int8_t sign = 1;

    *value = 0;

    for (i = 0; i < strLen; i++)
    {
        if (i == 0 && str[i] == '-')
        {
            sign = -1;
        }
        else if (str[i] >= '0' && str[i] <= '9')
        {
            *value = *value * 10 + (str[i] - '0');
        }
        else
        {
            return xFalse;
        }
    }
    *value = *value * sign;
    return xTrue;
}
xBool xStringnToFloat(char *str, uint8_t strLen, float *value)
{
    // convert string to float
    // return xTrue if success,otherwise return xFalse
    //*value = atof(str);
    uint8_t i = 0;
    uint8_t point = 0;
    float decimalPlace = 0.1;
    int sign = 1;

    *value = 0;

    if (str[0] == '-')
    {
        sign = -1;
        i = 1; // 从下一个字符开始处理
    }

    for (; i < strLen; i++)
    {
        if (str[i] >= '0' && str[i] <= '9')
        {
            if (point)
            {
                *value = *value + (str[i] - '0') * decimalPlace;
                decimalPlace *= 0.1;
            }
            else
            {
                *value = *value * 10 + (str[i] - '0');
            }
        }
        else if (str[i] == '.')
        {
            if (point) // 如果已经有一个小数点，则返回错误
            {
                return xFalse;
            }
            point = 1;
        }
        else
        {
            return xFalse;
        }
    }
    *value *= sign;
    return xTrue;
}

xBool xStringnToHex(char *str, uint8_t strLen, uint32_t *value)
{
    // convert hex string to uint32
    // return xTrue if success,otherwise return xFalse
    uint8_t i = 0;
    *value = 0;

    for (i = 0; i < strLen; i++)
    {
        if (str[i] >= '0' && str[i] <= '9')
        {
            *value = *value * 16 + (str[i] - '0');
        }
        else if (str[i] >= 'a' && str[i] <= 'f')
        {
            *value = *value * 16 + (str[i] - 'a' + 10);
        }
        else if (str[i] >= 'A' && str[i] <= 'F')
        {
            *value = *value * 16 + (str[i] - 'A' + 10);
        }
        else
        {
            return xFalse;
        }
    }
    return xTrue;
}

xBool xStringSeprateWithLen(char *src, char **sepPart, uint16_t *sepLen, char sepPartNum, char *sepMark, int *actualSepNum)
{
    if (src == NULL || sepPart == NULL || sepLen == NULL || sepMark == NULL || actualSepNum == NULL)
    {
        return xFalse;
    }

    int count = 0;
    char *token = strtok(src, sepMark);

    while (token != NULL && count < sepPartNum)
    {
        sepPart[count] = token;
        sepLen[count] = strlen(token);
        count++;
        token = strtok(NULL, sepMark);
    }

    if (count < sepPartNum)
    {
        sepPart[count] = NULL;
        sepLen[count] = 0;
    }
    *actualSepNum = count;

    return xTrue;
}

xBool xStringSeprate(char *src, char **sepPart, char sepPartNum, char *sepMark, int *actualSepNum)
{
    if (src == NULL || sepPart == NULL || sepMark == NULL || actualSepNum == NULL)
    {
        return xFalse;
    }

    int count = 0;
    char *token = strtok(src, sepMark);

    while (token != NULL && count < sepPartNum)
    {
        sepPart[count++] = token;
        token = strtok(NULL, sepMark);
    }

    if (count < sepPartNum)
    {
        sepPart[count] = NULL;
    }

    *actualSepNum = count;
    return xTrue;
}

uint16_t xStringUint32Toa(char *str, uint32_t value)
{
    // convert uint32 to string
    // return the length of the converted xString
    return sprintf(str, "%d", value);
}

uint16_t xStringInt32Toa(char *str, int32_t value)
{
    // convert int32 to string
    // return the length of the converted xString
    return sprintf(str, "%d", value);
}

uint16_t xStringFloatToa(char *str, float value, uint8_t precision)
{
    // convert float to string
    // return the length of the converted xString
    return sprintf(str, "%.*f", precision, value);
}

uint16_t xStringHexToa(char *str, uint32_t value)
{
    // convert uint32 to hex string
    // return the length of the converted xString
    return sprintf(str, "%x", value);
}