#ifndef __X_MATH_H__
#define __X_MATH_H__
#include "xDef.h"

//epsilon is the precision of the float comparison,
//e.g: 0.0001 means the two float is equal if the difference is less than 0.0001
xBool isFloatEqual(float a, float b, float epsilon);

#endif
