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

i32 car_num_left = 1;
i32 car_num_right = 0;

inline bool is_left(i32 deg) { return deg < 0; }
inline bool is_right(i32 deg) { return deg > 0; }


inline f32 rd_inner(i32 deg)
{
	return CAR_LENGTH / std::sin(deg * TO_RADIANS);
}

inline f32 rd_outer(i32 deg, i32 gap)
{
	const f32 r_i = rd_inner(deg);
	i32 b = gap + CAR_WIDTH;

	if(is_left(deg))
		b *= car_num_left;

	if(is_right(deg))
		b *= car_num_right;

	return r_i + std::copysign(b, deg);
}


Adjust::Adjust(bool is_slave)
    : is_slave(is_slave)
{}

void Adjust::speed_update(i32 spd)
{
	if(spd && !speed.prev)
		target_gap = gap;

	if(direction)
	{
		f32 ratio = rd_inner(direction) / rd_outer(direction, gap);
		spd *= ratio;
	}

	spd += spd * cam;

	speed.update(spd);
	drive(speed);
}

void Adjust::direction_update(i32 new_deg)
{
	const f32 r = rd_outer(new_deg, gap);
	f32 deg = std::asin(CAR_LENGTH / r) * TO_DEGREES;

	if(speed)
		deg += ADJUST_DEGREE * f32(target_gap - gap) / target_gap;

	direction.update(std::round(deg));
	steer(direction);

	speed_update(speed);
}

void Adjust::gap_update(i32 mm)
{
	gap.update(mm);
	direction_update(direction);
}

void Adjust::cam_update(i32 diff)
{
	cam.update(diff);
	speed_update(speed);
}
