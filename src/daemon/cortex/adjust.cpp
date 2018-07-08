#include "adjust.hpp"

#include "pwm.hpp"

#include <cmath>


#define CAR_LENGTH 264
#define CAR_WIDTH  195
#define MINMAX_DEGREE 20.0
#define SPEED_DIFF 16
#define TO_RADIANS 0.01745
#define TO_DEGREES 57.2958
#define ADJUST_PWM 30000

const auto &steer_deg = Steering::limit;


u8 adjust_speed(i32 degree, u8 speed, i32 gap_mm, int cam)
{
	gap_mm += CAR_WIDTH;

	const f32 sin_w = 1 + (gap_mm * std::sin(degree*TO_RADIANS)/ CAR_LENGTH);
	const f32 sin_b = 1 + ((gap_mm * std::sin(degree*TO_RADIANS)/ CAR_LENGTH)/2);

	u8 new_speed = speed;
	if(proto::Speed::STOP < speed){

		if(degree < 0)
		{
			new_speed = ((speed - proto::Speed::STOP) * sin_w) + proto::Speed::STOP;
		}
		else if(degree > 0)
		{
			new_speed = ((speed - proto::Speed::STOP) / sin_w) + proto::Speed::STOP;
		}
	}else if(proto::Speed::STOP > speed){

		if(degree < 0)
		{
			new_speed = proto::Speed::STOP - ((proto::Speed::STOP - speed) * sin_b);
		}
		else if(degree > 0)
		{
			new_speed = proto::Speed::STOP - ((proto::Speed::STOP - speed) / sin_b);
		}
	}

	if (std::abs(cam) > 3) // if cam < 0: speed++; else speed--
		new_speed -= std::copysign(2, cam);

	return new_speed;
}

i32 adjust_steer(i32 degree, i32 gap_mm)
{
	f32 corr = 0;

	if(degree < 0){
		const f32 r2 = (CAR_LENGTH / std::sin(degree * TO_RADIANS)) + gap_mm + CAR_WIDTH;
		corr = -(ADJUST_DEGREE / 2.0 * std::asin(CAR_LENGTH / r2) * TO_DEGREES);
	}
	else if(degree > 0){
		const f32 r2 = (CAR_LENGTH / std::sin(degree * TO_RADIANS)) - gap_mm;
		corr = ADJUST_DEGREE / 2.0 * std::asin(CAR_LENGTH / r2) * TO_DEGREES;
	}

	i32 new_degree = corr;

	static i32 gap_prev = gap_mm;
	if((gap_prev < gap_mm) && ((steer_deg.min + ADJUST_DEGREE) <= degree) && (degree < 0))
		new_degree -= ADJUST_DEGREE;
	else if((gap_prev > gap_mm) && ((steer_deg.max - ADJUST_DEGREE) >= degree) && (degree > 0))
		new_degree += ADJUST_DEGREE;

	gap_prev = gap_mm;

	return std::round(new_degree);
}
