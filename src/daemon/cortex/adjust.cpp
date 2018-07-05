#include "adjust.hpp"

#include "def.hpp"
#include "logger.hpp"

#include <cmath>

#define CAR_LENGTH 264
#define CAR_WIDTH  195
#define MINMAX_DEGREE 20.0
#define SPEED_DIFF 16
#define TO_RADIANS 0.01745
#define TO_DEGREES 57.2958
#define ADJUSTDEGREE 30000

inline f32 deg(u32 steer)
{
	auto dc_diff = std::abs(def::STEER_DC_DEF - i32(steer));
	return dc_diff * MINMAX_DEGREE / ((def::STEER_DC_SCALE.max - def::STEER_DC_SCALE.min) / 2);
}

u8 adjust_speed(u32 steer, u8 speed, u8 gap_cm, int cam)
{
	const f32 degree = deg(steer);
	const u32 gap_mm = gap_cm * 10 + CAR_WIDTH;

	const f32 sin_w = 1 + (gap_mm * std::sin(degree*TO_RADIANS)/ CAR_LENGTH);
	const f32 sin_b = 1 + ((gap_mm * std::sin(degree*TO_RADIANS)/ CAR_LENGTH)/2);
		
	u8 new_speed = speed;
	if(proto::Speed::STOP < speed){

		if(steer < def::STEER_DC_DEF)
		{
			new_speed = ((speed - proto::Speed::STOP) * sin_w) + proto::Speed::STOP;
		}
		else if(steer > def::STEER_DC_DEF)
		{
			new_speed = ((speed - proto::Speed::STOP) / sin_w) + proto::Speed::STOP;
		}
	}else if(proto::Speed::STOP > speed){

		if(steer < def::STEER_DC_DEF)
		{
			new_speed = proto::Speed::STOP - ((proto::Speed::STOP - speed) * sin_b);
		}
		else if(steer > def::STEER_DC_DEF)
		{
			new_speed = proto::Speed::STOP - ((proto::Speed::STOP - speed) / sin_b);
		}
	}

    if (std::abs(cam) <= 3) {
		return new_speed;
	}
    else if(0 > cam)
        new_speed += 2;
    else if(0 < cam)
        new_speed -= 2;

	return new_speed;
}

u32 adjust_steer(u32 steer, u8 gap_cm)
{
	const f32 degree = deg(steer);
	const u32 gap_mm = gap_cm * 10;

	f32 corr = 0;

	if(steer < def::STEER_DC_DEF){
		const f32 r2 = (CAR_LENGTH / std::sin(degree * TO_RADIANS)) + gap_mm + CAR_WIDTH;
		corr = -(15000 * std::asin(CAR_LENGTH / r2) * TO_DEGREES);		
	}
	else if(steer > def::STEER_DC_DEF){
		const f32 r2 = (CAR_LENGTH / std::sin(degree * TO_RADIANS)) - gap_mm;
		corr = 15000 * std::asin(CAR_LENGTH / r2) * TO_DEGREES;
	}
	

	u32 new_steer = def::STEER_DC_DEF + corr;

	static u8 init_cm = gap_cm;
	if((init_cm < gap_cm) && ((def::STEER_DC_SCALE.min + ADJUSTDEGREE) <= steer) && (steer < def::STEER_DC_DEF))
		new_steer -= ADJUSTDEGREE;
	else if((init_cm > gap_cm) && ((def::STEER_DC_SCALE.max - ADJUSTDEGREE) >= steer) && (steer > def::STEER_DC_DEF))
		new_steer += ADJUSTDEGREE;

	return new_steer;
}
