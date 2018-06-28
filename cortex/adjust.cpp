#include "adjust.hpp"
#include <iostream>

#define CARLENGTH 264
#define CARWIDTH 195
#define MINMAXDEGREE 20
#define SPEEDDIFF 16
#define TORADIANS 0.01745
#define TODEGREES 57.2958

extern loggr logger;


u8 adjustspeed(u32 steer, u8 motor, int us, int cam){
    u8 new_motor;

    static i16 init_cam = cam;
    float degree;

    if(steer==def::STEER_DC_DEF){
        new_motor = motor;
    }else if(steer < def::STEER_DC_DEF){

        degree = (MINMAXDEGREE*(def::STEER_DC_DEF-steer))/((float)(def::STEER_DC_SCALE.max - def::STEER_DC_SCALE.min)/2);
        new_motor = motor*(1+((us+CARWIDTH)*std::sin(degree*TORADIANS))/CARLENGTH);

    }else if(steer > def::STEER_DC_DEF){

        degree = (-MINMAXDEGREE*(def::STEER_DC_DEF-steer))/((float)(def::STEER_DC_SCALE.max - def::STEER_DC_SCALE.min)/2);
        new_motor = motor*(1+((us+CARWIDTH)*std::sin(degree*TORADIANS))/CARLENGTH);

    }
    if((std::abs(init_cam - cam)<= 5)){
        return new_motor; 
    }else if(init_cam < cam){
        return (new_motor - 16);
    }else{
        return (new_motor + 16);
    }
  
}

u32 adjustdistance(u32 steer, int us){
    u32 new_steer;
    static i16 init_us = us;
    float degree;
	
    if(steer == def::STEER_DC_DEF){
	new_steer = steer;
    }else if(steer < def::STEER_DC_DEF){    
        degree = (MINMAXDEGREE*(def::STEER_DC_DEF-steer))/(((float)def::STEER_DC_SCALE.max - def::STEER_DC_SCALE.min)/2);
        new_steer = def::STEER_DC_DEF-(15000*(std::asin(CARLENGTH/((CARLENGTH/sin(degree*TORADIANS))+CARWIDTH+us))*TODEGREES));

    }else if(steer > def::STEER_DC_DEF){
        degree = (-MINMAXDEGREE*(def::STEER_DC_DEF-steer))/(((float)(def::STEER_DC_SCALE.max - def::STEER_DC_SCALE.min))/2);
	new_steer = def::STEER_DC_DEF+(15000*(std::asin(CARLENGTH/((CARLENGTH/sin(degree*TORADIANS))+CARWIDTH+us))*TODEGREES));
    }
    
   if((std::abs(init_us - us)<= 10)){
        return new_steer; 
    }else if((init_us < us) && (1280000 <= steer) && (steer < def::STEER_DC_DEF)){
        return (new_steer - 30000);
    }else if((init_us > us) && (1720000 >= steer) && (steer > def::STEER_DC_DEF)){
        return (new_steer + 30000);
    } else{
        return new_steer;
    }
   
}
