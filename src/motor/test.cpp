#include "motor.h"

int main(){
    // Open the Simple Motor Controller's virtual COM port.
    const char* device ="/dev/ttyACM0";
    // Linux
    Motor Motor1(device);
    Motor1.smcSetTargetSpeedPercent(20);
    std::cout << "Target Speed: " << Motor1.smcGetTargetSpeed() << std::endl;
    sleep(6);
    Motor1.smcSetTargetSpeed(0);

    /*int fd = open(device, O_RDWR | O_NOCTTY);
    if(fd == -1){
        perror(device);
        return 1;
    }


    struct termios options;
    tcgetattr(fd, &options);
    options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tcsetattr(fd, TCSANOW, &options);
    smcExitSafeStart(fd);
    printf("Error status: 0x%04x\n", smcGetErrorStatus(fd));

    int speed = smcGetTargetSpeed(fd);
    printf("Current Target Speed is %d.\n", speed);
    printf("MaxSpeed Forward: %d\n", smcGetVariable(fd, 0x1E));
    printf("MaxAcceleration Forward: %d\n", smcGetVariable(fd, 0x1F));
    printf("MaxDeceleration Forward: %d\n", smcGetVariable(fd, 0x20));
    smcSetTargetSpeedPercent(fd, 50);
    printf("MaxSpeed Forward: %d\n", smcGetVariable(fd, 0x1E));
    sleep(5);
    smcSetTargetSpeed(fd, 1000);
    printf("Current Target Speed: %d\n", smcGetTargetSpeed(fd));
    sleep(3);
    smcSetTargetSpeedPercent(fd, 0);

    //int newSpeed = (speed <= 0) ? 1000 : 1000;
    //printf("Setting Target Speed to %d.\n", newSpeed);
/*    smcSetTargetSpeedPercent(fd, 4);
    sleep(5);
    smcSetTargetSpeedPercent(fd, 10);
    printf("Current Target Speed is %d.\n", smcGetTargetSpeed(fd));
    sleep(5);
    smcSetTargetSpeedPercent(fd, 0);
    sleep(2);
    smcSetTargetSpeedPercent(fd, -7);
    sleep(5);
    smcSetTargetSpeedPercent(fd, 0);*/

    return 0;
}
