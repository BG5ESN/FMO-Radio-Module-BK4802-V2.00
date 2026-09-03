#include "radioConvert.h"

static const float ctcssList[] = {
    0.0, // off
    67.00,
    71.90,
    74.40,
    77.00,
    79.70,
    82.50,
    85.40,
    88.50,
    91.50,
    94.80,
    97.40,
    100.00,
    103.50,
    107.20,
    110.90,
    114.80,
    118.80,
    123.00,
    127.30,
    131.80,
    136.50,
    141.30,
    146.20,
    151.40,
    156.70,
    162.20,
    167.90,
    173.80,
    179.90,
    186.20,
    192.80,
    203.50,
    210.70,
    218.10,
    225.70,
    233.60,
    241.80,
    250.30,
};
xBool isValideCTCSS(float ctcss)
{
    // check the ctcss value is in the list
    for (int ii = 0; ii < sizeof(ctcssList) / sizeof(float); ii++)
    {
        if (isFloatEqual(ctcss, ctcssList[ii], 0.1))
        {
            return xTrue;
        }
    }
    return xFalse;
}
CTCSS_E getCTCSS(float ctcss)
{
    // get the CTCSS value
    for (int ii = 0; ii < sizeof(ctcssList) / sizeof(float); ii++)
    {
        if (isFloatEqual(ctcss, ctcssList[ii], 0.1))
        {
            return (CTCSS_E)ii;
        }
    }
    return CTCSS_OFF;
}
float getCTCSSFreq(CTCSS_E ctcss)
{
    // get the CTCSS frequency
    if (ctcss < sizeof(ctcssList) / sizeof(float))
    {
        return ctcssList[ctcss];
    }
    return 0.0;
}

xBool isVailideHamFreq(float freq)
{
    // include U V HF band
    // VHF 136-174
    if (freq >= 136.0 && freq <= 174.0)
    {
        return xTrue;
    }
    // VHF 144-148
    if (freq >= 144.0 && freq <= 148.0)
    {
        return xTrue;
    }
    // 430M
    if (freq >= 430.0 && freq <= 440.0)
    {
        return xTrue;
    }

    // 50M
    if (freq >= 50.0 && freq <= 54.0)
    {
        return xTrue;
    }
    // 28M
    if (freq >= 28.0 && freq <= 29.7)
    {
        return xTrue;
    }
    // 24M
    if (freq >= 24.89 && freq <= 24.99)
    {
        return xTrue;
    }

    // 21M
    if (freq >= 21.0 && freq <= 21.45)
    {
        return xTrue;
    }

    // 18M
    if (freq >= 18.068 && freq <= 18.168)
    {
        return xTrue;
    }
    // 14M
    if (freq >= 14.0 && freq <= 14.35)
    {
        return xTrue;
    }
    // 7M
    if (freq >= 7.0 && freq <= 7.3)
    {
        return xTrue;
    }
    // 3.5M
    if (freq >= 3.5 && freq <= 3.9)
    {
        return xTrue;
    }
    // 1.8M
    if (freq >= 1.8 && freq <= 2.0)
    {
        return xTrue;
    }

    return xFalse;
}
