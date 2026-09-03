#ifndef __PID_H__
#define __PID_H__
typedef struct
{
    float kp;
    float ki;
    float kd;
    float setPoint;
    float lastError;
    float integral;
    float output;
} PID_t;
void PID_Init(PID_t *pid, float kp, float ki, float kd, float setPoint);
void PID_SetSetPoint(PID_t *pid, float setPoint);
float PID_Calc(PID_t *pid, float feedback);

#endif