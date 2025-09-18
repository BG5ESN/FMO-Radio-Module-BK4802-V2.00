#ifndef __RADIO_CONVERT_H__
#define __RADIO_CONVERT_H__
#include "components.h"
#define CTCSS_0 0.0
#define CTCSS_1 67.0
#define CTCSS_2 69.3
#define CTCSS_3 71.9
#define CTCSS_4 74.4
#define CTCSS_5 77.0
#define CTCSS_6 79.7
#define CTCSS_7 82.5
#define CTCSS_8 85.4
#define CTCSS_9 88.5
#define CTCSS_10 91.5
#define CTCSS_11 94.8
#define CTCSS_12 97.4
#define CTCSS_13 100.0
#define CTCSS_14 103.5
#define CTCSS_15 107.2
#define CTCSS_16 110.9
#define CTCSS_17 114.8
#define CTCSS_18 118.8
#define CTCSS_19 123.0
#define CTCSS_20 127.3
#define CTCSS_21 131.8
#define CTCSS_22 136.5
#define CTCSS_23 141.3
#define CTCSS_24 146.2
#define CTCSS_25 151.4
#define CTCSS_26 156.7
#define CTCSS_27 162.2
#define CTCSS_28 167.9
#define CTCSS_29 173.8
#define CTCSS_30 179.9
#define CTCSS_31 186.2
#define CTCSS_32 192.8
#define CTCSS_33 203.5
#define CTCSS_34 210.7
#define CTCSS_35 218.1
#define CTCSS_36 225.7
#define CTCSS_37 233.6
#define CTCSS_38 241.8
#define CTCSS_39 250.3

typedef enum
{
    CTCSS_OFF = 0,
    CTCSS_67_0 = 1,
    CTCSS_71_9 = 2,
    CTCSS_74_4 = 3,
    CTCSS_77_0 = 4,
    CTCSS_79_7 = 5,
    CTCSS_82_5 = 6,
    CTCSS_85_4 = 7,
    CTCSS_88_5 = 8,
    CTCSS_91_5 = 9,
    CTCSS_94_8 = 10,
    CTCSS_97_4 = 11,
    CTCSS_100_0 = 12,
    CTCSS_103_5 = 13,
    CTCSS_107_2 = 14,
    CTCSS_110_9 = 15,
    CTCSS_114_8 = 16,
    CTCSS_118_8 = 17,
    CTCSS_123_0 = 18,
    CTCSS_127_3 = 19,
    CTCSS_131_8 = 20,
    CTCSS_136_5 = 21,
    CTCSS_141_3 = 22,
    CTCSS_146_2 = 23,
    CTCSS_151_4 = 24,
    CTCSS_156_7 = 25,
    CTCSS_162_2 = 26,
    CTCSS_167_9 = 27,
    CTCSS_173_8 = 28,
    CTCSS_179_9 = 29,
    CTCSS_186_2 = 30,
    CTCSS_192_8 = 31,
    CTCSS_203_5 = 32,
    CTCSS_210_7 = 33,
    CTCSS_218_1 = 34,
    CTCSS_225_7 = 35,
    CTCSS_233_6 = 36,
    CTCSS_241_8 = 37,
    CTCSS_250_3 = 38,
} CTCSS_E;

//CTCSS CONVERT
xBool isValideCTCSS(float ctcss);  // check if the CTCSS value is valid (error less than 0.01 Hz e.g: 71.9HZ in float is 71.89~71.91 to correct the float error)
CTCSS_E getCTCSS(float ctcss);     // get CTCSS value from frequency,if not found return CTCSS_OFF
float getCTCSSFreq(CTCSS_E ctcss); // get CTCSS frequency from CTCSS value if not found return 0.0
xBool isVailideHamFreq(float freq); // check if the frequency is valid for ham radio
#endif                             // __RADIO_CONVERT_H__
