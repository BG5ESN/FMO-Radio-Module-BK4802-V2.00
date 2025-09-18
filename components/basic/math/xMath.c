#include "xMath.h"
xBool isFloatEqual(float a, float b, float epsilon)
{
    // compare two float number
    if (a - b < epsilon && a - b > -epsilon)
    {
        return xTrue;
    }
    return xFalse;
}
