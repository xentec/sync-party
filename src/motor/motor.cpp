#include "motor.h"

#define SERIAL_ERROR -9999
// Reads a variable from the SMC and returns it as number between 0 and 65535.
// Returns SERIAL_ERROR if there was an error.
// The 'variableId' argument must be one of IDs listed in the
// "Controller Variables" section of the user's guide.
// For variables that are actually signed, additional processing is required
// (see smcGetTargetSpeed for an example).

Motor::Motor(){
    const char* device="/dev/ttyACM0";
    fd = open(device, O_RDWR | O_NOCTTY);
    if(fd == -1){
        perror(device);
    }
    tcgetattr(fd, &options);
    options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tcsetattr(fd, TCSANOW, &options);
    smcExitSafeStart();
}

Motor::Motor(const char* device){
    fd = open(device, O_RDWR | O_NOCTTY);
    if(fd == -1){
        perror(device);
    }
    tcgetattr(fd, &options);
    options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tcsetattr(fd, TCSANOW, &options);
    smcExitSafeStart();
}


int Motor::smcGetVariable(unsigned char variableId){
    unsigned char command[] = {0xA1, variableId};
    if(write(fd, &command,sizeof(command)) == -1){
        perror("error writing");
        return SERIAL_ERROR;
    }
    unsigned char response[2];
    if(read(fd,response,2) != 2){
        perror("error reading");
        return SERIAL_ERROR;
    }
    return response[0] + 256*response[1];
}

int Motor::smcSetMotorLimit(unsigned char limitId, unsigned short value){
    unsigned char command[4]={0xA2, limitId, value&0x7F, value>>7};
    if(write(fd, &command, sizeof(command))!=0){
        perror("error writing");
        return SERIAL_ERROR;
    }
    unsigned char response[1];
    if(read(fd,response,1) !=2){
        perror("error reading");
        return SERIAL_ERROR;
    }
    return response[0];
    smcExitSafeStart();
}
// Returns the target speed (-3200 to 3200).
// Returns SERIAL_ERROR if there is an error.
int Motor::smcGetTargetSpeed(){
    int val = smcGetVariable(20);
    return val == SERIAL_ERROR ? SERIAL_ERROR : (signed short)val;
}
// Returns a number where each bit represents a different error, and the
// bit is 1 if the error is currently active.
// See the user's guide for definitions of the different error bits.
// Returns SERIAL_ERROR if there is an error.
int Motor::smcGetErrorStatus(){
    return smcGetVariable(0);
}
// Sends the Exit Safe Start command, which is required to drive the motor.
// Returns 0 if successful, SERIAL_ERROR if there was an error sending.
int Motor::smcExitSafeStart(){
    const unsigned char command = 0x83;
    if(write(fd, &command, 1) == -1){
        perror("error writing");
        return SERIAL_ERROR;
    }
    return 0;
}
// Sets the SMC's target speed (-3200 to 3200).
// Returns 0 if successful, SERIAL_ERROR if there was an error sending.
int Motor::smcSetTargetSpeed(int speed){
    unsigned char command[3];
    if(speed < 0){
        command[0] = 0x86;
        speed = -speed;
    }else{
        command[0] = 0x85;
        // Motor Forward
    }
    command[1] = speed & 0x1F;
    command[2] = speed >> 5 & 0x7F;
    printf("Hex Command: %x %x %x\n", command[0], command[1], command[2]);
    if(write(fd, command,sizeof(command)) == -1){
        perror("error writing");
        return  SERIAL_ERROR;
    }
    return 0;
}

int Motor::smcSetTargetSpeedPercent(int percent){
    unsigned char command[3];
    if(percent<0){
        command[0]=0x86;
        percent=-percent;
    }else{
        command[0]=0x85;
    }
    command[1]=0;
    command[2]=percent;
    printf("Hex Command: %x %x %x\n", command[0], command[1], command[2]);
    if(write(fd, command, sizeof(command))==-1){
        perror("error writing");
        return SERIAL_ERROR;
    }
    return 0;
}

Motor::~Motor(){
    close(fd);
}
