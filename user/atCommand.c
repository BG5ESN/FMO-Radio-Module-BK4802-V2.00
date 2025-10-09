/*
 *AT Command Module Proc
 *Author:xifengzui AKA BG5ESN
 *Date:2024-10-21
 */

#include "atCommand.h"
#include "SHARECom.h"
#include "components.h"
#include "radioConvert.h"

#undef LOG_TAG
#define LOG_TAG "AT"
#define AT_CMD_FEATURE_BUFFERING_MAX 32 // max buffing command changes
#define AT_CMD_BUF_LEN (256)            // min required buffer length

// Command Basic Define
#define AT_CMD_HANDLER_PERIOD 10                             // command handler period unit ms
#define AT_CMD_COMSUME_TIMEOUT 1000                          // command comsume timeout, if the command is not comsumed in this time, the command will be discard unit ms
#define AT_CMD_RECV_BYTE_MAX 32                              // max byte received once
#define AT_CMD_SEND_BYTE_MAX 32                              // max byte send once
#define AT_CMD_MAX_LEN 128                                   // max command length
#define AT_CMD_MAX_ARG 8                                     // max arguments
#define AT_CMD_MAX_ARG_LEN (AT_CMD_MAX_LEN / AT_CMD_MAX_ARG) // max argument length
#if (A_CMD_MAX_LEN % AT_CMD_MAX_ARG)
#error "AT_CMD_MAX_LEN must be a multiple of AT_CMD_MAX_ARG"
#endif

#if (AT_CMD_MAX_ARG_LEN < 8)
#error "AT_CMD_MAX_ARG_LEN must be greater than 8"
#endif

// AT Command List
#define AT_CMD_NAME "NAME"

// Report as number
#define AT_CMD_VER "VER"

// Report mult band capability
#define AT_CMD_BANDCAP "BANDCAP"

// level between 1~10
#define AT_CMD_SQL "SQL"

// freq is define in code
#define AT_CMD_TXFREQ "TXFREQ"
#define AT_CMD_RXFREQ "RXFREQ"

// level between 0~10 0 is off
#define AT_CMD_RXVOL "RXVOL"
#define AT_CMD_TXVOL "TXVOL"

// ctcss is define in code
#define AT_CMD_TCTCSS "TCTCSS"
#define AT_CMD_RCTCSS "RCTCSS"

// freqTune
#define AT_CMD_FREQTUNE "FREQTUNE"

// NOTE: freqTune 类型已在 SHARECom.h 中调整为 int32_t 频率偏移(Hz)
// 频率偏移(Hz)的范围是 -100~100

// level LOW MID HIGH
#define AT_CMD_TXPWR "TXPWR"
#define AT_CMD_TXPWR_LIST_0 "LOW"
#define AT_CMD_TXPWR_LIST_1 "MID"
#define AT_CMD_TXPWR_LIST_2 "HIGH"

#define AT_CMD_SMETER "SMETER" // S meter level 1~9

// RF Enable/Disable
#define AT_CMD_RF "RF"
#define AT_CMD_RF_ENABLE "ENABLE"
#define AT_CMD_RF_DISABLE "DISABLE"

// System Command
#define AT_CMD_SYS "SYS"
#define AT_CMD_SYS_RESET "RESET"

// report command
#define AT_CMD_OK "OK"
#define AT_CMD_SUCCESS "SUCCESS"
#define AT_CMD_FAILED "FAILED"
#define AT_CMD_INVALID "INVALID"

typedef enum
{
    E_AT_RESULT_INVALID,
    E_AT_RESULT_OK,
    E_AT_RESULT_FAIL,
    E_AT_RESULT_SUCC
} ATCmdResult;

typedef enum
{
    E_AT_CMD_TYPE_SET,
    E_AT_CMD_TYPE_GET,
} ATCmdType;

typedef enum
{
    E_AT_CMD_ARG_TYPE_INVALID,
    E_AT_CMD_ARG_TYPE_CHAR,
    E_AT_CMD_ARG_TYPE_HEX,
    E_AT_CMD_ARG_TYPE_INT,
    E_AT_CMD_ARG_TYPE_UINT,
    E_AT_CMD_ARG_TYPE_FLOAT,
    E_AT_CMD_ARG_TYPE_STRING,
} ATCmdArgType;

typedef struct
{
    ATCmdArgType argType;

    union
    {
        char strValue[AT_CMD_MAX_ARG_LEN];
        uint32_t uintValue;
        int32_t intValue;
        float floatValue;
        char charValue;
    } raw;
} ATCmdArg;

typedef struct
{
    ATCmd cmd;                     // command
    ATCmdResult result;            // result
    ATCmdType type;                // set or get
    uint8_t argNum;                // argument number
    ATCmdArg args[AT_CMD_MAX_ARG]; // arguments
} ATCmdArgs;

static char atFeatureRing[AT_CMD_FEATURE_BUFFERING_MAX * sizeof(ATCmd) + 2]; // extra 2 bytes for the ring buffer
static char atCmdRing[AT_CMD_BUF_LEN + 2];                                   // extra 2 bytes for the ring buffer
static char atCmdProcRaw[AT_CMD_BUF_LEN];
static uint32_t atCmdComsumeTimeout = 0;
static uint32_t atCmdPeriond = 0;
static ATCmdArgs recvCmdArgs; // received command arguments
static ATCmdPort ctrl =
    {
        .recvBytes = NULL,
        .sendBytes = NULL,
};
static xRingBuf_t serialRingHandler;
static xRingBuf_t featureCmdRingHandler;
void ATCmdInit(const ATCmdPort *port)
{
    if (port == NULL)
    {
        return;
    }
    ctrl.recvBytes = port->recvBytes;
    ctrl.sendBytes = port->sendBytes;
    xRingBufInit(&serialRingHandler, (unsigned char *)atCmdRing, sizeof(atCmdRing));
    xRingBufInit(&featureCmdRingHandler, (unsigned char *)atFeatureRing, sizeof(atFeatureRing));
}

void fetchPut(ATCmd cmd)
{
    log_d("fetchPut:%d", cmd);
    xRingBufPut(&featureCmdRingHandler, (uint8_t *)&cmd, sizeof(ATCmd));
}
ATCmd fetchGet(void)
{
    int len = 0;
    ATCmd cmd;
    len = xRingBufGet(&featureCmdRingHandler, (uint8_t *)&cmd, sizeof(ATCmd));
    if (len == 0)
    {
        return E_AT_CMD_NONE;
    }
    else if (len != sizeof(ATCmd))
    {
        log_d("fetchGet failed,len:%d", len);
        return E_AT_CMD_NONE;
    }
    else if (cmd >= E_AT_CMD_MAX)
    {
        log_d("fetchGet failed,cmd:%d", cmd);
        return E_AT_CMD_NONE;
    }
    return cmd;
}

// will parse command as outArgs not related to the SHARECom
xBool ATCmdParse(ATCmdArgs *outArgs)
{
    // step1 find "AT"
    uint16_t startIdx = 0;
    memset(outArgs, 0, sizeof(ATCmdArgs));
    if (xStringStartIdxFinder(atCmdProcRaw, "AT", &startIdx) == xFalse)
    {
        return xFalse;
    }
    startIdx = startIdx + xStringLen("AT"); // go to content.
    if (atCmdProcRaw[startIdx] == '?')      // AT?
    {
        outArgs->cmd = E_AT_CMD_TEST;
        outArgs->result = E_AT_RESULT_OK;
        outArgs->type = E_AT_CMD_TYPE_GET;
        outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_INVALID;
        outArgs->argNum = 0;
        log_d("query command");
        return xTrue;
    }
    else if (atCmdProcRaw[startIdx] == '+') // AT+
    {
        startIdx = startIdx + 1; // go to command
        if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_NAME, xStringLen(AT_CMD_NAME)) == xTrue)
        {
            startIdx = startIdx + xStringLen(AT_CMD_NAME); // goto cmdType
            if (atCmdProcRaw[startIdx] == '?')             // query command
            {
                outArgs->cmd = E_AT_CMD_NAME;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query name");
                return xTrue;
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE; // the command not support
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("not supprot edit name");
                return xTrue;
            }
        }
        // version
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_VER, xStringLen(AT_CMD_VER)) == xTrue)
        {
            startIdx = startIdx + xStringLen(AT_CMD_VER); // goto cmdType
            if (atCmdProcRaw[startIdx] == '?')            // query command
            {
                outArgs->cmd = E_AT_CMD_VER;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query version");
                return xTrue;
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE; // the command not support
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("not supprot edit version");
                return xTrue;
            }
        }
        // BAND CAP
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_BANDCAP, xStringLen(AT_CMD_BANDCAP)) == xTrue)
        {
            startIdx = startIdx + xStringLen(AT_CMD_BANDCAP);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_BANDCAP;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query band capability");
                return xTrue;
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE; // the command not support
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("not support edit band capability");
                return xTrue;
            }
        }
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_SMETER, xStringLen(AT_CMD_SMETER)) == xTrue)
        {
            startIdx = startIdx + xStringLen(AT_CMD_SMETER);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_SMETER;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query S meter level");
                return xTrue;
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE; // the command not support
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("not support edit S meter level");
                return xTrue;
            }
        }
        // SQL LEVEL
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_SQL, xStringLen(AT_CMD_SQL)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_SQL);
            // parse string to uint32
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_SQL;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query SQL");
                return xTrue;
            }
            else if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                char *sepPtr[AT_CMD_MAX_ARG];
                uint16_t sepLen[AT_CMD_MAX_ARG];
                int acturalSepNum = 0;

                for (int dd = 0; dd < AT_CMD_MAX_ARG; dd++)
                {
                    sepPtr[dd] = NULL;
                    sepLen[dd] = 0;
                }

                if (xStringSeprateWithLen(atCmdProcRaw + startIdx, sepPtr, sepLen, AT_CMD_MAX_ARG, ",", &acturalSepNum) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepFailed");
                    return xTrue;
                }

                log_d("sepNum:%d", acturalSepNum);
                if (acturalSepNum != 1)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepNumError");
                    return xTrue;
                }

                // parse the string to uint32
                if (xStringnToUint32(sepPtr[0], sepLen[0], &outArgs->args[0].raw.uintValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("parse SQL failed");
                    return xTrue;
                }

                // check the value
                if (outArgs->args[0].raw.uintValue > 10)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SQL value out of range");
                    return xTrue;
                }

                log_d("set SQL level:%d", outArgs->args[0].raw.intValue);
                outArgs->cmd = E_AT_CMD_SQL;
                outArgs->result = E_AT_RESULT_SUCC;
                outArgs->type = E_AT_CMD_TYPE_SET;
                outArgs->argNum = 1;
                outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_UINT;

                return xTrue;
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE;
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("not support SQL");
                return xTrue;
            }
        }
        // TX Freq
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_TXFREQ, xStringLen(AT_CMD_TXFREQ)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_TXFREQ);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_TXFREQ;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query TX freq");
                return xTrue;
            }
            else if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                char *sepPtr[AT_CMD_MAX_ARG];
                uint16_t sepLen[AT_CMD_MAX_ARG];
                int acturalSepNum = 0;

                for (int dd = 0; dd < AT_CMD_MAX_ARG; dd++)
                {
                    sepPtr[dd] = NULL;
                    sepLen[dd] = 0;
                }

                if (xStringSeprateWithLen(atCmdProcRaw + startIdx, sepPtr, sepLen, AT_CMD_MAX_ARG, ",", &acturalSepNum) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepFailed");
                    return xTrue;
                }

                log_d("sepNum:%d", acturalSepNum);
                if (acturalSepNum != 1)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepNumError");
                    return xTrue;
                }

                // parse the string to float
                if (xStringnToFloat(sepPtr[0], sepLen[0], &outArgs->args[0].raw.floatValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("parse TX freq failed");
                    return xTrue;
                }

                // check the value
                if (isVailideHamFreq(outArgs->args[0].raw.floatValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("TX freq value out of range");
                    return xTrue;
                }

                log_d("set TX freq:%.5f", outArgs->args[0].raw.floatValue);
                outArgs->cmd = E_AT_CMD_TXFREQ;
                outArgs->result = E_AT_RESULT_SUCC;
                outArgs->type = E_AT_CMD_TYPE_SET;
                outArgs->argNum = 1;
                outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_FLOAT;

                return xTrue;
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE;
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("not support TX freq");
                return xTrue;
            }
        }
        // RX Freq
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_RXFREQ, xStringLen(AT_CMD_RXFREQ)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_RXFREQ);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_RXFREQ;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query RX freq");
                return xTrue;
            }
            else if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                char *sepPtr[AT_CMD_MAX_ARG];
                uint16_t sepLen[AT_CMD_MAX_ARG];
                int acturalSepNum = 0;

                for (int dd = 0; dd < AT_CMD_MAX_ARG; dd++)
                {
                    sepPtr[dd] = NULL;
                    sepLen[dd] = 0;
                }

                if (xStringSeprateWithLen(atCmdProcRaw + startIdx, sepPtr, sepLen, AT_CMD_MAX_ARG, ",", &acturalSepNum) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepFailed");
                    return xTrue;
                }

                log_d("sepNum:%d", acturalSepNum);
                if (acturalSepNum != 1)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepNumError");
                    return xTrue;
                }

                // parse the string to float
                if (xStringnToFloat(sepPtr[0], sepLen[0], &outArgs->args[0].raw.floatValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("parse RX freq failed");
                    return xTrue;
                }
                if (isVailideHamFreq(outArgs->args[0].raw.floatValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("RX freq out of range");
                    return xTrue;
                }

                log_d("set RX freq:%.5f", outArgs->args[0].raw.floatValue);
                outArgs->cmd = E_AT_CMD_RXFREQ;
                outArgs->result = E_AT_RESULT_SUCC;
                outArgs->type = E_AT_CMD_TYPE_SET;
                outArgs->argNum = 1;
                outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_FLOAT;

                return xTrue;
            }
        }
        // TXVOL
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_TXVOL, xStringLen(AT_CMD_TXVOL)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_TXVOL);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_TXVOL;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query TX vol");
                return xTrue;
            }
            else if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                char *sepPtr[AT_CMD_MAX_ARG];
                uint16_t sepLen[AT_CMD_MAX_ARG];
                int acturalSepNum = 0;

                for (int dd = 0; dd < AT_CMD_MAX_ARG; dd++)
                {
                    sepPtr[dd] = NULL;
                    sepLen[dd] = 0;
                }

                if (xStringSeprateWithLen(atCmdProcRaw + startIdx, sepPtr, sepLen, AT_CMD_MAX_ARG, ",", &acturalSepNum) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepFailed");
                    return xTrue;
                }

                log_d("sepNum:%d", acturalSepNum);
                if (acturalSepNum != 1)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepNumError");
                    return xTrue;
                }

                // parse the string to uint32
                if (xStringnToUint32(sepPtr[0], sepLen[0], &outArgs->args[0].raw.uintValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("parse TX vol failed");
                    return xTrue;
                }

                // check the value
                if (outArgs->args[0].raw.uintValue > 10)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("TX vol value out of range");
                    return xTrue;
                }

                log_d("set TX vol:%d", outArgs->args[0].raw.intValue);
                outArgs->cmd = E_AT_CMD_TXVOL;
                outArgs->result = E_AT_RESULT_SUCC;
                outArgs->type = E_AT_CMD_TYPE_SET;
                outArgs->argNum = 1;
                outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_UINT;
                return xTrue;
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE;
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("not support TX vol");
                return xTrue;
            }
        }
        // RXVOL
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_RXVOL, xStringLen(AT_CMD_RXVOL)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_RXVOL);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_RXVOL;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query RX vol");
                return xTrue;
            }
            else if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                char *sepPtr[AT_CMD_MAX_ARG];
                uint16_t sepLen[AT_CMD_MAX_ARG];
                int acturalSepNum = 0;

                for (int dd = 0; dd < AT_CMD_MAX_ARG; dd++)
                {
                    sepPtr[dd] = NULL;
                    sepLen[dd] = 0;
                }

                if (xStringSeprateWithLen(atCmdProcRaw + startIdx, sepPtr, sepLen, AT_CMD_MAX_ARG, ",", &acturalSepNum) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepFailed");
                    return xTrue;
                }

                log_d("sepNum:%d", acturalSepNum);
                if (acturalSepNum != 1)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepNumError");
                    return xTrue;
                }

                // parse the string to uint32
                if (xStringnToUint32(sepPtr[0], sepLen[0], &outArgs->args[0].raw.uintValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("parse RX vol failed");
                    return xTrue;
                }

                // check the value
                if (outArgs->args[0].raw.uintValue > 10)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("RX vol value out of range");
                    return xTrue;
                }

                log_d("set RX vol:%d", outArgs->args[0].raw.intValue);
                outArgs->cmd = E_AT_CMD_RXVOL;
                outArgs->result = E_AT_RESULT_SUCC;
                outArgs->type = E_AT_CMD_TYPE_SET;
                outArgs->argNum = 1;
                outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_UINT;
                return xTrue;
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE;
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("not support RX vol");
                return xTrue;
            }
        }
        // TCTCSS
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_TCTCSS, xStringLen(AT_CMD_TCTCSS)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_TCTCSS);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_TCTCSS;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query TX ctcss");
                return xTrue;
            }
            else if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                char *sepPtr[AT_CMD_MAX_ARG];
                uint16_t sepLen[AT_CMD_MAX_ARG];
                int acturalSepNum = 0;

                for (int dd = 0; dd < AT_CMD_MAX_ARG; dd++)
                {
                    sepPtr[dd] = NULL;
                    sepLen[dd] = 0;
                }

                if (xStringSeprateWithLen(atCmdProcRaw + startIdx, sepPtr, sepLen, AT_CMD_MAX_ARG, ",", &acturalSepNum) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepFailed");
                    return xTrue;
                }

                log_d("sepNum:%d", acturalSepNum);
                if (acturalSepNum != 1)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepNumError");
                    return xTrue;
                }

                // parse the string to uint32
                if (xStringnToFloat(sepPtr[0], sepLen[0], &outArgs->args[0].raw.floatValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("parse TX ctcss failed");
                    return xTrue;
                }

                // check the value
                if (isValideCTCSS(outArgs->args[0].raw.floatValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("TX ctcss value out of range");
                    return xTrue;
                }

                log_d("set TX ctcss:%d", outArgs->args[0].raw.floatValue);
                outArgs->cmd = E_AT_CMD_TCTCSS;
                outArgs->result = E_AT_RESULT_FAIL; // BK4802 不支持CTCSS功能
                outArgs->type = E_AT_CMD_TYPE_SET;
                outArgs->argNum = 1;
                outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_FLOAT;
                return xTrue;
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE;
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("not support TX ctcss");
                return xTrue;
            }
        }
        // RCTCSS
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_RCTCSS, xStringLen(AT_CMD_RCTCSS)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_RCTCSS);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_RCTCSS;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query RX ctcss");
                return xTrue;
            }
            else if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                char *sepPtr[AT_CMD_MAX_ARG];
                uint16_t sepLen[AT_CMD_MAX_ARG];
                int acturalSepNum = 0;

                for (int dd = 0; dd < AT_CMD_MAX_ARG; dd++)
                {
                    sepPtr[dd] = NULL;
                    sepLen[dd] = 0;
                }

                if (xStringSeprateWithLen(atCmdProcRaw + startIdx, sepPtr, sepLen, AT_CMD_MAX_ARG, ",", &acturalSepNum) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepFailed");
                    return xTrue;
                }

                log_d("sepNum:%d", acturalSepNum);
                if (acturalSepNum != 1)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepNumError");
                    return xTrue;
                }

                // parse the string to uint32
                if (xStringnToFloat(sepPtr[0], sepLen[0], &outArgs->args[0].raw.floatValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("parse RX ctcss failed");
                    return xTrue;
                }

                // check the value
                if (isValideCTCSS(outArgs->args[0].raw.floatValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("RX ctcss value out of range");
                    return xTrue;
                }

                log_d("set RX ctcss:%d", outArgs->args[0].raw.floatValue);
                outArgs->cmd = E_AT_CMD_RCTCSS;
                outArgs->result = E_AT_RESULT_FAIL; // BK4802 不支持CTCSS功能
                outArgs->type = E_AT_CMD_TYPE_SET;
                outArgs->argNum = 1;
                outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_FLOAT;
                return xTrue;
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE;
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("not support RX ctcss");
                return xTrue;
            }
        }
        // E_AT_CMD_TXPWR
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_TXPWR, xStringLen(AT_CMD_TXPWR)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_TXPWR);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_TXPWR;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query TX power");
                return xTrue;
            }
            else if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_TXPWR_LIST_0, xStringLen(AT_CMD_TXPWR_LIST_0)) == true)
                {
                    outArgs->cmd = E_AT_CMD_TXPWR;
                    outArgs->result = E_AT_RESULT_SUCC;
                    outArgs->type = E_AT_CMD_TYPE_SET;
                    outArgs->argNum = 1;
                    outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
                    xStringnCopy(outArgs->args[0].raw.strValue, AT_CMD_TXPWR_LIST_0, xStringLen(AT_CMD_TXPWR_LIST_0));
                    log_d("set TX power LOW");
                    return xTrue;
                }
                else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_TXPWR_LIST_1, xStringLen(AT_CMD_TXPWR_LIST_1)) == true)
                {
                    outArgs->cmd = E_AT_CMD_TXPWR;
                    outArgs->result = E_AT_RESULT_SUCC;
                    outArgs->type = E_AT_CMD_TYPE_SET;
                    outArgs->argNum = 1;
                    outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
                    xStringnCopy(outArgs->args[0].raw.strValue, AT_CMD_TXPWR_LIST_1, xStringLen(AT_CMD_TXPWR_LIST_1));
                    log_d("set TX power MID");
                    return xTrue;
                }
                else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_TXPWR_LIST_2, xStringLen(AT_CMD_TXPWR_LIST_2)) == true)
                {
                    outArgs->cmd = E_AT_CMD_TXPWR;
                    outArgs->result = E_AT_RESULT_SUCC;
                    outArgs->type = E_AT_CMD_TYPE_SET;
                    outArgs->argNum = 1;
                    outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
                    xStringnCopy(outArgs->args[0].raw.strValue, AT_CMD_TXPWR_LIST_2, xStringLen(AT_CMD_TXPWR_LIST_2));
                    log_d("set TX power HIGH");
                    return xTrue;
                }
                else
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_INVALID;
                    log_w("not support TX power");
                    return xTrue;
                }
            }
        }
        // E_AT_FREQ_TUNE
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_FREQTUNE, xStringLen(AT_CMD_FREQTUNE)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_FREQTUNE);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_FREQTUNE;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query freq tune");
                return xTrue;
            }
            else if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                char *sepPtr[AT_CMD_MAX_ARG];
                uint16_t sepLen[AT_CMD_MAX_ARG];
                int acturalSepNum = 0;

                for (int dd = 0; dd < AT_CMD_MAX_ARG; dd++)
                {
                    sepPtr[dd] = NULL;
                    sepLen[dd] = 0;
                }

                if (xStringSeprateWithLen(atCmdProcRaw + startIdx, sepPtr, sepLen, AT_CMD_MAX_ARG, ",", &acturalSepNum) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepFailed");
                    return xTrue;
                }

                log_d("sepNum:%d", acturalSepNum);
                if (acturalSepNum != 1)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("SepNumError");
                    return xTrue;
                }

                // parse the string to int32 (Hz 频偏)
                if (xStringnToInt32(sepPtr[0], sepLen[0], &outArgs->args[0].raw.intValue) == xFalse)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("parse freq tune failed");
                    return xTrue;
                }

                // 简单限制范围 ±50000 Hz
                if (outArgs->args[0].raw.intValue > 50000 || outArgs->args[0].raw.intValue < -50000)
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_FAIL;
                    log_w("freq tune out of range ±50000 Hz");
                    return xTrue;
                }

                log_d("set freq tune(offset Hz):%d", outArgs->args[0].raw.intValue);
                outArgs->cmd = E_AT_CMD_FREQTUNE;
                outArgs->result = E_AT_RESULT_SUCC;
                outArgs->type = E_AT_CMD_TYPE_SET;
                outArgs->argNum = 1;
                outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_INT;
                return xTrue;
            }
        }
        // RF Enable/Disable
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_RF, xStringLen(AT_CMD_RF)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_RF);
            if (atCmdProcRaw[startIdx] == '?')
            {
                outArgs->cmd = E_AT_CMD_RF;
                outArgs->result = E_AT_RESULT_OK;
                outArgs->type = E_AT_CMD_TYPE_GET;
                log_d("query RF enable state");
                return xTrue;
            }
            else if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_RF_ENABLE, xStringLen(AT_CMD_RF_ENABLE)) == true)
                {
                    outArgs->cmd = E_AT_CMD_RF;
                    outArgs->result = E_AT_RESULT_SUCC;
                    outArgs->type = E_AT_CMD_TYPE_SET;
                    outArgs->argNum = 1;
                    outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
                    xStringnCopy(outArgs->args[0].raw.strValue, AT_CMD_RF_ENABLE, xStringLen(AT_CMD_RF_ENABLE));
                    log_d("set RF ENABLE");
                    return xTrue;
                }
                else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_RF_DISABLE, xStringLen(AT_CMD_RF_DISABLE)) == true)
                {
                    outArgs->cmd = E_AT_CMD_RF;
                    outArgs->result = E_AT_RESULT_SUCC;
                    outArgs->type = E_AT_CMD_TYPE_SET;
                    outArgs->argNum = 1;
                    outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
                    xStringnCopy(outArgs->args[0].raw.strValue, AT_CMD_RF_DISABLE, xStringLen(AT_CMD_RF_DISABLE));
                    log_d("set RF DISABLE");
                    return xTrue;
                }
                else
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_INVALID;
                    log_w("not support RF state");
                    return xTrue;
                }
            }
        }
        // System Command (only RESET)
        else if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_SYS, xStringLen(AT_CMD_SYS)) == true)
        {
            startIdx = startIdx + xStringLen(AT_CMD_SYS);
            if (atCmdProcRaw[startIdx] == '=')
            {
                startIdx = startIdx + 1;
                if (xStringnCompare(&atCmdProcRaw[startIdx], AT_CMD_SYS_RESET, xStringLen(AT_CMD_SYS_RESET)) == true)
                {
                    outArgs->cmd = E_AT_CMD_SYS;
                    outArgs->result = E_AT_RESULT_SUCC; // 立即返回 SUCCESS，实际复位延迟执行
                    outArgs->type = E_AT_CMD_TYPE_SET;
                    outArgs->argNum = 1;
                    outArgs->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
                    xStringnCopy(outArgs->args[0].raw.strValue, AT_CMD_SYS_RESET, xStringLen(AT_CMD_SYS_RESET));
                    log_d("system reset requested");
                    return xTrue;
                }
                else
                {
                    outArgs->cmd = E_AT_CMD_NONE;
                    outArgs->result = E_AT_RESULT_INVALID;
                    log_w("not support SYS op");
                    return xTrue;
                }
            }
            else
            {
                outArgs->cmd = E_AT_CMD_NONE;
                outArgs->result = E_AT_RESULT_INVALID;
                log_w("SYS only support set RESET");
                return xTrue;
            }
        }
        // INVALID
        else
        {
            outArgs->cmd = E_AT_CMD_NONE;
            outArgs->result = E_AT_RESULT_INVALID;
            log_w("not support command");
            return xTrue;
        }
    }
    return xFalse;
}

// base on COM ,process the command,full fill the result
xBool ATCmdArgsGetProc(ATCmdArgs *argsToBeProc, SHARECom *base)
{
    if (argsToBeProc == NULL || base == NULL)
    {
        log_e("argsToBeProc or base is NULL");
        return xFalse;
    }
    if (argsToBeProc->type == E_AT_CMD_TYPE_SET)
    {
        return xTrue;
    }
    if (argsToBeProc->cmd == E_AT_CMD_NONE)
    {
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_TEST)
    {
        log_d("get test condition");
        argsToBeProc->argNum = 0;
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_NAME)
    {
        log_d("get name");
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
        xStringnCopy(argsToBeProc->args[0].raw.strValue, NAME, sizeof(NAME));
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_VER)
    {
        log_d("get version");
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
        xStringnCopy(argsToBeProc->args[0].raw.strValue, VERSION, sizeof(VERSION));
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_BANDCAP)
    {
        log_d("get band capability");
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_HEX;
        argsToBeProc->args[0].raw.uintValue = base->bandCap;
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_SMETER)
    {
        log_d("get S meter level");
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_UINT;
        argsToBeProc->args[0].raw.uintValue = base->smeter;
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_SQL)
    {

        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_UINT;
        argsToBeProc->args[0].raw.uintValue = base->sql;
        log_d("get SQL level is %d", base->sql);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_TXFREQ)
    {
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_FLOAT;
        argsToBeProc->args[0].raw.floatValue = base->txFreq;
        log_d("get TX freq is %.5f", base->txFreq);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_RXFREQ)
    {
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_FLOAT;
        argsToBeProc->args[0].raw.floatValue = base->rxFreq;
        log_d("get RX freq is %.5f", base->rxFreq);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_RXVOL)
    {
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_UINT;
        argsToBeProc->args[0].raw.uintValue = base->rxVol;
        log_d("get RX vol is %d", base->rxVol);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_TXVOL)
    {
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_UINT;
        argsToBeProc->args[0].raw.uintValue = base->txVol;
        log_d("get TX vol is %d", base->txVol);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_TCTCSS)
    {
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_FLOAT;
        argsToBeProc->args[0].raw.floatValue = base->tCTCSS;
        log_d("get TX ctcss is %.1f", base->tCTCSS);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_RCTCSS)
    {
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_FLOAT;
        argsToBeProc->args[0].raw.floatValue = base->rCTCSS;
        log_d("get RX ctcss is %.1f", base->rCTCSS);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_TXPWR)
    {
        log_d("get TX power");
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
        if (base->txPwr == TX_PWR_LOW)
        {
            log_d("get TX power LOW");
            xStringnCopy(argsToBeProc->args[0].raw.strValue, AT_CMD_TXPWR_LIST_0, xStringLen(AT_CMD_TXPWR_LIST_0));
        }
        else if (base->txPwr == TX_PWR_MID)
        {
            log_d("get TX power MID");
            xStringnCopy(argsToBeProc->args[0].raw.strValue, AT_CMD_TXPWR_LIST_1, xStringLen(AT_CMD_TXPWR_LIST_1));
        }
        else if (base->txPwr == TX_PWR_HIGH)
        {
            log_d("get TX power HIGH");
            xStringnCopy(argsToBeProc->args[0].raw.strValue, AT_CMD_TXPWR_LIST_2, xStringLen(AT_CMD_TXPWR_LIST_2));
        }
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_FREQTUNE)
    {
        log_d("get freq tune");
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_INT;
        argsToBeProc->args[0].raw.intValue = base->freqTune;
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_RF)
    {
        log_d("get RF enable state");
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
        if (base->rfEnable)
        {
            xStringnCopy(argsToBeProc->args[0].raw.strValue, AT_CMD_RF_ENABLE, xStringLen(AT_CMD_RF_ENABLE));
        }
        else
        {
            xStringnCopy(argsToBeProc->args[0].raw.strValue, AT_CMD_RF_DISABLE, xStringLen(AT_CMD_RF_DISABLE));
        }
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_SYS)
    {
        // 查询不支持，只在 set 时返回 SUCCESS，无需回读参数
        argsToBeProc->argNum = 1;
        argsToBeProc->args[0].argType = E_AT_CMD_ARG_TYPE_STRING;
        xStringnCopy(argsToBeProc->args[0].raw.strValue, AT_CMD_SYS_RESET, xStringLen(AT_CMD_SYS_RESET));
        return xTrue;
    }
    log_e("not support command");
    return xFalse;
}

// base on COM ,process the command,full fill the SHARECom
xBool ATCmdArgsSetProc(ATCmdArgs *argsToBeProc, SHARECom *base)
{
    if (argsToBeProc == NULL || base == NULL)
    {
        log_e("argsToBeProc or base is NULL");
        return xFalse;
    }
    if (argsToBeProc->type == E_AT_CMD_TYPE_GET)
    {
        return xTrue;
    }

    if (argsToBeProc->cmd == E_AT_CMD_NONE)
    {
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_NAME)
    {
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_VER)
    {
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_BANDCAP)
    {
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_SMETER)
    {
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_TEST)
    {
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_SQL)
    {
        base->sql = argsToBeProc->args[0].raw.uintValue;
        fetchPut(E_AT_CMD_SQL);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_TXFREQ)
    {
        base->txFreq = argsToBeProc->args[0].raw.floatValue;
        fetchPut(E_AT_CMD_TXFREQ);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_RXFREQ)
    {
        base->rxFreq = argsToBeProc->args[0].raw.floatValue;
        fetchPut(E_AT_CMD_RXFREQ);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_RXVOL)
    {
        base->rxVol = argsToBeProc->args[0].raw.uintValue;
        fetchPut(E_AT_CMD_RXVOL);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_TXVOL)
    {
        base->txVol = argsToBeProc->args[0].raw.uintValue;
        fetchPut(E_AT_CMD_TXVOL);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_TCTCSS)
    {
        base->tCTCSS = argsToBeProc->args[0].raw.floatValue;
        fetchPut(E_AT_CMD_TCTCSS);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_RCTCSS)
    {
        base->rCTCSS = argsToBeProc->args[0].raw.floatValue;
        fetchPut(E_AT_CMD_RCTCSS);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_TXPWR)
    {
        if (xStringnCompare(argsToBeProc->args[0].raw.strValue, AT_CMD_TXPWR_LIST_0, xStringLen(AT_CMD_TXPWR_LIST_0)) == true)
        {
            base->txPwr = TX_PWR_LOW;
        }
        else if (xStringnCompare(argsToBeProc->args[0].raw.strValue, AT_CMD_TXPWR_LIST_1, xStringLen(AT_CMD_TXPWR_LIST_1)) == true)
        {
            base->txPwr = TX_PWR_MID;
        }
        else if (xStringnCompare(argsToBeProc->args[0].raw.strValue, AT_CMD_TXPWR_LIST_2, xStringLen(AT_CMD_TXPWR_LIST_2)) == true)
        {
            base->txPwr = TX_PWR_HIGH;
        }
        fetchPut(E_AT_CMD_TXPWR);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_FREQTUNE)
    {
        base->freqTune = argsToBeProc->args[0].raw.intValue; // 频偏Hz
        fetchPut(E_AT_CMD_FREQTUNE);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_RF)
    {
        if (xStringnCompare(argsToBeProc->args[0].raw.strValue, AT_CMD_RF_ENABLE, xStringLen(AT_CMD_RF_ENABLE)) == true)
        {
            base->rfEnable = 1;
        }
        else if (xStringnCompare(argsToBeProc->args[0].raw.strValue, AT_CMD_RF_DISABLE, xStringLen(AT_CMD_RF_DISABLE)) == true)
        {
            base->rfEnable = 0;
        }
        fetchPut(E_AT_CMD_RF);
        return xTrue;
    }
    else if (argsToBeProc->cmd == E_AT_CMD_SYS)
    {
        // 不修改 SHARECom 数据，只发出功能事件
        fetchPut(E_AT_CMD_SYS);
        return xTrue;
    }
    return xFalse;
}

xBool ATCmdSendResult(ATCmdArgs *inArgs)
{
    char sendBuf[AT_CMD_SEND_BYTE_MAX];
    memset(sendBuf, 0, AT_CMD_SEND_BYTE_MAX);
    if (inArgs == NULL)
    {
        return xFalse;
    }
    if (inArgs->result == E_AT_RESULT_INVALID)
    {
        ctrl.sendBytes((uint8_t *)AT_CMD_INVALID, xStringLen(AT_CMD_INVALID));
        ctrl.sendBytes((uint8_t *)"\n", 1);
        return xTrue;
    }
    else if (inArgs->result == E_AT_RESULT_FAIL)
    {
        ctrl.sendBytes((uint8_t *)AT_CMD_FAILED, xStringLen(AT_CMD_FAILED));
        ctrl.sendBytes((uint8_t *)"\n", 1);
        return xTrue;
    }
    else if (inArgs->result == E_AT_RESULT_SUCC)
    {
        ctrl.sendBytes((uint8_t *)AT_CMD_SUCCESS, xStringLen(AT_CMD_SUCCESS));
        ctrl.sendBytes((uint8_t *)"\n", 1);
        return xTrue;
    }
    else if (inArgs->result == E_AT_RESULT_OK)
    {
        uint16_t sendBufUsedLen = 0;
        switch (inArgs->cmd)
        {
        case E_AT_CMD_TEST:
            sendBufUsedLen = 0;
            break;
        case E_AT_CMD_NAME:
            sendBufUsedLen = xStringLen(AT_CMD_NAME);
            xStringnCopy(sendBuf, AT_CMD_NAME, sendBufUsedLen);
            break;
        case E_AT_CMD_VER:
            sendBufUsedLen = xStringLen(AT_CMD_VER);
            xStringnCopy(sendBuf, AT_CMD_VER, sendBufUsedLen);
            break;
        case E_AT_CMD_BANDCAP:
            sendBufUsedLen = xStringLen(AT_CMD_BANDCAP);
            xStringnCopy(sendBuf, AT_CMD_BANDCAP, sendBufUsedLen);
            break;
        case E_AT_CMD_SMETER:
            sendBufUsedLen = xStringLen(AT_CMD_SMETER);
            xStringnCopy(sendBuf, AT_CMD_SMETER, sendBufUsedLen);
            break;
        case E_AT_CMD_SQL:
            sendBufUsedLen = xStringLen(AT_CMD_SQL);
            xStringnCopy(sendBuf, AT_CMD_SQL, sendBufUsedLen);
            break;
        case E_AT_CMD_TXFREQ:
            sendBufUsedLen = xStringLen(AT_CMD_TXFREQ);
            xStringnCopy(sendBuf, AT_CMD_TXFREQ, sendBufUsedLen);
            break;
        case E_AT_CMD_RXFREQ:
            sendBufUsedLen = xStringLen(AT_CMD_RXFREQ);
            xStringnCopy(sendBuf, AT_CMD_RXFREQ, sendBufUsedLen);
            break;
        case E_AT_CMD_RXVOL:
            sendBufUsedLen = xStringLen(AT_CMD_RXVOL);
            xStringnCopy(sendBuf, AT_CMD_RXVOL, sendBufUsedLen);
            break;
        case E_AT_CMD_TXVOL:
            sendBufUsedLen = xStringLen(AT_CMD_TXVOL);
            xStringnCopy(sendBuf, AT_CMD_TXVOL, sendBufUsedLen);
            break;
        case E_AT_CMD_TCTCSS:
            sendBufUsedLen = xStringLen(AT_CMD_TCTCSS);
            xStringnCopy(sendBuf, AT_CMD_TCTCSS, sendBufUsedLen);
            break;
        case E_AT_CMD_RCTCSS:
            sendBufUsedLen = xStringLen(AT_CMD_RCTCSS);
            xStringnCopy(sendBuf, AT_CMD_RCTCSS, sendBufUsedLen);
            break;
        case E_AT_CMD_TXPWR:
            sendBufUsedLen = xStringLen(AT_CMD_TXPWR);
            xStringnCopy(sendBuf, AT_CMD_TXPWR, sendBufUsedLen);
            break;
        case E_AT_CMD_FREQTUNE:
            sendBufUsedLen = xStringLen(AT_CMD_FREQTUNE);
            xStringnCopy(sendBuf, AT_CMD_FREQTUNE, sendBufUsedLen);
            break;
        case E_AT_CMD_RF:
            sendBufUsedLen = xStringLen(AT_CMD_RF);
            xStringnCopy(sendBuf, AT_CMD_RF, sendBufUsedLen);
            break;
        case E_AT_CMD_SYS:
            sendBufUsedLen = xStringLen(AT_CMD_SYS);
            xStringnCopy(sendBuf, AT_CMD_SYS, sendBufUsedLen);
            break;
        default:
            break;
        }

        if (inArgs->argNum != 0)
        {
            sendBuf[sendBufUsedLen++] = ':';
        }

        for (int ii = 0; ii < inArgs->argNum; ii++)
        {
            if (inArgs->args[ii].argType == E_AT_CMD_ARG_TYPE_UINT)
            {
                sendBufUsedLen += xStringUint32Toa(sendBuf + sendBufUsedLen, inArgs->args[ii].raw.uintValue);
            }
            else if (inArgs->args[ii].argType == E_AT_CMD_ARG_TYPE_FLOAT)
            {
                sendBufUsedLen += xStringFloatToa(sendBuf + sendBufUsedLen, inArgs->args[ii].raw.floatValue, 4);
            }
            else if (inArgs->args[ii].argType == E_AT_CMD_ARG_TYPE_STRING)
            {
                xStringnCopy(sendBuf + sendBufUsedLen, inArgs->args[ii].raw.strValue, xStringLen(inArgs->args[ii].raw.strValue));
                sendBufUsedLen += xStringLen(inArgs->args[ii].raw.strValue);
            }
            else if (inArgs->args[ii].argType == E_AT_CMD_ARG_TYPE_INT)
            {
                sendBufUsedLen += xStringInt32Toa(sendBuf + sendBufUsedLen, inArgs->args[ii].raw.intValue);
            }
            else if (inArgs->args[ii].argType == E_AT_CMD_ARG_TYPE_HEX) // 以16进制显示
            {
                sendBufUsedLen += xStringHexToa(sendBuf + sendBufUsedLen, inArgs->args[ii].raw.uintValue);
            }

            if (ii != inArgs->argNum - 1)
            {
                sendBuf[sendBufUsedLen++] = ',';
            }
        }
        if (inArgs->argNum != 0)
        {
            sendBuf[sendBufUsedLen++] = '\n';
        }
        xStringnCopy(sendBuf + sendBufUsedLen, "OK\n", xStringLen("OK\n"));
        sendBufUsedLen += xStringLen("OK\n");
        ctrl.sendBytes((uint8_t *)sendBuf, sendBufUsedLen);
        return xTrue;
    }
    return xFalse;
}

// AT Command Handler Process Command Every 100ms
void ATCmdHandler(SHARECom *com)
{
    uint8_t recvBuf[AT_CMD_RECV_BYTE_MAX];
    uint16_t recvLen = 0;
    uint16_t storeLen = 0;
    int endTagIndex = 0;

    xBool isFoundEndTag = xFalse;

    // 添加参数检查
    if (com == NULL || ctrl.recvBytes == NULL || ctrl.sendBytes == NULL)
    {
        log_e("Invalid parameters or uninitialized controller");
        return;
    }

    // wait for the command handler period
    if (atCmdPeriond > millis())
    {
        return;
    }

    atCmdPeriond = millis() + AT_CMD_HANDLER_PERIOD;
    memset(atCmdProcRaw, 0, AT_CMD_BUF_LEN);
    recvLen = ctrl.recvBytes(recvBuf, AT_CMD_RECV_BYTE_MAX);
    if (recvLen == 0)
    {
        return;
    }
    log_d("recvLen:%d", recvLen);

    // step1: put the received data into the ring buffer and check for end tags
    for (endTagIndex = 0; endTagIndex < recvLen; endTagIndex++)
    {
        // 检查换行符组合：\r\n, \n\r, \r, \n
        if (recvBuf[endTagIndex] == '\n' || recvBuf[endTagIndex] == '\r')
        {
            isFoundEndTag = xTrue;
            // 修复索引越界问题
            while (endTagIndex + 1 < recvLen && (recvBuf[endTagIndex + 1] == '\n' || recvBuf[endTagIndex + 1] == '\r'))
            {
                endTagIndex++;
            }
            break;
        }
    }
    log_d("isFoundEndTag:%d", isFoundEndTag);

    if (isFoundEndTag == false)
    {
        // 只存储可打印字符到环形缓冲区，并检查缓冲区空间
        for (int j = 0; j < recvLen; j++)
        {
            if (recvBuf[j] >= 0x20 && recvBuf[j] <= 0x7E)
            {
                if (xRingBufFull(&serialRingHandler))
                {
                    log_w("ring buffer is full, drop the data");
                    xRingBufClear(&serialRingHandler);
                    return;
                }
                xRingBufPut(&serialRingHandler, &recvBuf[j], 1);
            }
        }
        return;
    }

    // assemble the command,include(1)the previous data in the ring buffer (2)the current data of recvBuf
    storeLen = xRingBufGet(&serialRingHandler, (unsigned char *)atCmdProcRaw, AT_CMD_BUF_LEN);

    // put the current data of recvBuf into the command buffer (only printable chars before end tag)
    for (int j = 0; j < endTagIndex; j++)
    {
        if (recvBuf[j] >= 0x20 && recvBuf[j] <= 0x7E && storeLen < AT_CMD_BUF_LEN - 1)
        {
            atCmdProcRaw[storeLen++] = recvBuf[j];
        }
    }

    // 确保字符串结束
    atCmdProcRaw[storeLen] = '\0';

    log_d("found command:%s", atCmdProcRaw);

    // put the rest data back to the ring buffer (skip the end tag)
    endTagIndex++; // 跳过当前的换行符
    if (endTagIndex < recvLen)
    {
        for (int j = endTagIndex; j < recvLen; j++)
        {
            if (recvBuf[j] >= 0x20 && recvBuf[j] <= 0x7E)
            {
                // 检查缓冲区空间
                if (!xRingBufFull(&serialRingHandler))
                {
                    xRingBufPut(&serialRingHandler, &recvBuf[j], 1);
                }
                else
                {
                    log_w("ring buffer full when storing remaining data");
                    break;
                }
            }
        }
    }

    // 如果命令为空，直接返回
    if (storeLen == 0)
    {
        return;
    }

    // step2: parse the command
    if (ATCmdParse(&recvCmdArgs) == xFalse)
    {
        log_w("parse command failed");
        return;
    }
    if (ATCmdArgsSetProc(&recvCmdArgs, com) == xFalse)
    {
        log_w("set command failed");
        return;
    }

    if (ATCmdArgsGetProc(&recvCmdArgs, com) == xFalse)
    {
        log_w("get command failed");
        return;
    }

    if (ATCmdSendResult(&recvCmdArgs) == xFalse)
    {
        log_w("send result failed");
        return;
    }
}

ATCmd FetchATCmd(void)
{
    return fetchGet();
}