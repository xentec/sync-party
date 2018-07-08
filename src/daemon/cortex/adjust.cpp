#include "adjust.hpp"

#include "driver.hpp"
#include "pwm.hpp"

#include <cmath>

const auto PI = std::asin(1) * 2.0;

constexpr auto ADJUST_DEGREE = 3;

constexpr auto CAR_LENGTH = 264;
constexpr auto CAR_WIDTH  = 195;

#define TO_RADIANS 0.01745
#define TO_DEGREES 57.2958

inline bool is_left(i32 deg) { return deg < 0; }
inline bool is_right(i32 deg) { return deg > 0; }


inline f32 rd_inner(i32 deg)
{
	return CAR_LENGTH / std::sin(deg * TO_RADIANS);
}

inline f32 rd_outer(i32 deg, i32 gap, i32 cl, i32 cr)
{
	const f32 r_i = rd_inner(deg);
	i32 b = gap + CAR_WIDTH;

	if(is_left(deg))  b *= cl;
	else
	if(is_right(deg)) b *= cr;

	return r_i + std::copysign(b, deg);
}


Adjust::Adjust(Position&& pos)
    : pos(std::move(pos))
{}


void Adjust::adjust_speed(i32 spd)
{
	if(spd && !speed.prev)
		gap.target = gap;

	if(steer)
	{
		f32 ratio = rd_inner(steer) / rd_outer(steer, gap, pos.left, pos.right);
		spd *= ratio;
	}

	spd += spd * cam;

	speed.update(spd);
	drive(speed);
}

void Adjust::speed_update(i32 spd)
{
	speed.target = spd;
	adjust_speed(spd);
}

void Adjust::adjust_steer(i32 deg)
{
	const f32 r = rd_outer(deg, gap, pos.left, pos.right);
	deg = std::asin(CAR_LENGTH / r) * TO_DEGREES;

	if(speed)
		deg += ADJUST_DEGREE * f32(gap.target - gap) / gap.target;

	steer.update(std::round(deg));
	steering(steer);

	adjust_speed(speed.target);
}

void Adjust::steer_update(i32 deg)
{
	steer.target = deg;
	adjust_steer(deg);
}

void Adjust::gap_update(i32 mm)
{
	gap.update(mm);
	adjust_steer(steer);
}

void Adjust::cam_update(i32 diff)
{
	cam.update(diff);
	adjust_speed(speed);
}
