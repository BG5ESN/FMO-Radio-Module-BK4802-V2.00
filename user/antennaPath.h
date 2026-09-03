#ifndef __ANTENNA_PATH_H__
#define __ANTENNA_PATH_H__

typedef enum
{
    ANTENNA_PATH_ATTENUATOR = 0,
    ANTENNA_PATH_FILTER = 1,
} ANTENNA_PATH;

void antennaPathInit(void);
void antennaPathCtrl(ANTENNA_PATH path);

#endif