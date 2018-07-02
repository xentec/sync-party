#include "adjust.hpp"

#include "def.hpp"
#include "logger.hpp"

#include <cmath>

#define CAR_LENGTH 264
#define CAR_WIDTH  195
#define MINMAX_DEGREE 20
#define SPEEDDIFF 16
#define TO_RADIANS 0.01745
#define TO_DEGREES 57.2958

constexpr f64 C = MINMAX_DEGREE * (def::STEER_DC_SCALE.max - def::STEER_DC_SCALE.min)/2.0;

u8 adjust_speed(u32 steer, u8 speed, u8 gap_cm, int cam)
{
	u8 new_speed = speed;

	const u32 mm = gap_cm * 10;
	const u32 gap = mm+CAR_WIDTH;
	const f32 degree = std::abs(f32(def::STEER_DC_DEF - steer)) / C;
	const f32 sin_w = 1 + (gap * std::sin(degree*TO_RADIANS));

	if(steer < def::STEER_DC_DEF)
	{
		new_speed = speed * (sin_w / CAR_LENGTH);
	}
	else if(steer > def::STEER_DC_DEF)
	{
		new_speed = speed * (CAR_LENGTH / sin_w);
	}

	static i16 init_cam = cam;
	if(init_cam < cam)
		new_speed -= 16;
	else
		new_speed += 16;

	return new_speed;
}

u32 adjust_steer(u32 steer, u8 gap_cm)
{
	const u16 mm = gap_cm * 10;
	const f32 degree = std::abs(f32(def::STEER_DC_DEF - steer)) / C;

	const f32 r2 = (CAR_LENGTH / std::sin(degree * TO_RADIANS)) + CAR_WIDTH+mm;
	f32 corr = 15000 * std::asin(CAR_LENGTH / r2) * TO_DEGREES;

	if(steer < def::STEER_DC_DEF)
		corr = -corr;

	u32 new_steer = def::STEER_DC_DEF + corr;

	static u8 init_cm = gap_cm;
	if((init_cm < gap_cm) && (1230000 <= steer) && (steer < def::STEER_DC_DEF))
		new_steer -= 30000;
	else if((init_cm > gap_cm) && (1770000 >= steer) && (steer > def::STEER_DC_DEF))
		new_steer += 30000;


	return new_steer;
}
