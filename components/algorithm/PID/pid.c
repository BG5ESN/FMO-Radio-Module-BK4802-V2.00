#include "pid.h"
#include "elog.h"
void PID_Init(PID_t *pid, float kp, float ki, float kd, float setPoint)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->setPoint = setPoint;
    pid->lastError = 0.0;
    pid->integral = 0.0;
    pid->output = 0.0;
    //log_i("PID Init - kp: %.3f, ki: %.3f, kd: %.3f, setPoint: %.3f",
    //      pid->kp, pid->ki, pid->kd, pid->setPoint);
}
void PID_SetSetPoint(PID_t *pid, float setPoint)
{
    pid->setPoint = setPoint;
}
float PID_Calc(PID_t *pid, float feedback)
{
    float error = pid->setPoint - feedback;
    pid->integral += error;

    pid->output = pid->kp * error + pid->ki * pid->integral + pid->kd * (error - pid->lastError);
    pid->lastError = error;

    // log_i("PID Calc - setPoint: %.3f, feedback: %.3f, error: %.3f, integral: %.3f, output: %.3f",
    //       pid->setPoint, feedback, error, pid->integral, pid->output);
    return pid->output;
}

#if 0

//example
#include <stdio.h>
int main()
{
    PID_t pid;
    PID_Init(&pid, 1, 0.1, 0.1, 100);
    for (int i = 0; i < 100; i++)
    {
        float output = PID_Calc(&pid, i);
        printf("output:%f\n", output);
    }
    return 0;
}
#endif