#ifndef MOTOR_H_INCLUDE
#define MOTOR_H_INCLUDE

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>

class Motor{
    private:
        int fd;
        struct termios options;
    protected:
    public:
        Motor();
        Motor(const char* device);
        int smcGetVariable(unsigned char variableId);
        int smcSetMotorLimit(unsigned char limitId, unsigned short value);
        int smcGetTargetSpeed();
        int smcGetErrorStatus();
        int smcExitSafeStart();
        int smcSetTargetSpeed(int speed);
        int smcSetTargetSpeedPercent(int percent);
        ~Motor();
};


#endif // MOTOR_H_INCLUDE
