#include "adjust.hpp"
#include <iostream>

#define CARLENGTH 264
#define CARWIDTH 195
#define MINMAXDEGREE 20
#define SPEEDDIFF 16

u8 adjustspeed(u32 steer, u8 motor, int us, int cam){
    u8 new_motor;
    static i16 init_us = us;
    static i16 init_cam = cam;
    float degree;

    if(steer==1500000){
        new_motor = motor + 16;
    }else if(steer < 1500000){

        degree = (MINMAXDEGREE*(1500000-steer))/300000;
        new_motor = motor*(1+((us+CARWIDTH)*std::sin(degree))/CARLENGTH);

    }else if(steer > 1500000){

        degree = (-MINMAXDEGREE*(1500000-steer))/300000;
        new_motor = motor*(1+((us+CARWIDTH)*std::sin(degree))/CARLENGTH);

    }
    
    if(motor > 48){
        new_motor = motor + SPEEDDIFF;
    }else if((motor < 48 ) && ((motor - (SPEEDDIFF/2)) >=  16)){
        new_motor = motor - (SPEEDDIFF/2);
    }

    if((std::abs(init_cam - cam)<= 10)){
        return new_motor; 
    }else if(init_cam < cam){
        return (new_motor - 16);
    }else{
        return (new_motor + 16);
    }
    
}

u32 adjustdistance(u32 steer, u8 motor, int us){
    u32 new_steer;
    static i16 init_us = us;
    float degree

    if(steer==1500000){
        new_steer = steer;
    }else if(steer < 1500000){
        degree = (MINMAXDEGREE*(1500000-steer))/300000);
        new_steer = 1500000-(15000*(std::asin(CARLENGTH/((CARLENGTH/sin(degree))+CARWIDTH+us))));

    }else if(steer > 1500000){
        degree = (-MINMAXDEGREE*(1500000-steer))/300000);
        new_steer = 1500000+(15000*(std::asin(CARLENGTH/((CARLENGTH/sin(degree))+CARWIDTH+us))));

    }

    if((std::abs(init_us - us)<= 10)){
        return new_steer; 
    }else if((init_us < us) && (1280000 <= steer) && (steer < 1500000)){
        return (new_steer - 30000);
    }else if((init_us > us) && (1720000 >= steer) && (steer > 1500000)){
        return (new_steer + 30000);
    } else{
        return new_steer;
    }
    
}