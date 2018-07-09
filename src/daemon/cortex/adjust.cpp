#include "adjust.hpp"

#include "util.hpp"

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


inline f32 r_inner(i32 deg)
{
	return CAR_LENGTH / std::sin(deg * TO_RADIANS);
}

inline f32 r_outer(i32 deg, i32 gap, i32 b_n = 0)
{
	const f32 r_i = r_inner(deg);
	const i32 b = gap + CAR_WIDTH;
	return r_i + std::copysign(b * b_n, deg);
}

inline f32 r_me(i32 deg, i32 gap, const Adjust::Line& line)
{
	const u16 in_cars = is_left(deg) ? line.pos : line.max - line.pos;
	return r_outer(deg, gap, in_cars);
}

Adjust::Adjust(Line&& line)
	: line(std::move(line))
	, logger(new_loggr("adjust"))
{
	logger->info("pos: {} line length: {}", line.pos, line.max);
}


void Adjust::adjust_speed(f32 spd)
{
	if(spd && !speed.prev)
		gap.target = gap;

	f32 r = 0.0;
	if(steer.target)
	{
		const f32 ro = r_outer(steer.target, gap.target, line.max);
		r = r_me(steer.target, gap.target, line);
		spd = r/ro * speed.target;
	}

	spd += spd * cam;

	speed.update(std::round(spd));
	on_change(speed.curr, [&](auto speed_prev, auto speed)
	{
		logger->debug("M: {:3} => {:3} - gap: {:3} cam: {:3} r: {:6.4}", speed_prev, speed, i32(gap), i32(cam), r);
		drive(speed);
	});
}

void Adjust::speed_update(i32 spd)
{
	speed.target = spd;
	adjust_speed(spd);
}

void Adjust::adjust_steer(f32 deg)
{
	const f32 ro = r_me(deg, gap.target, line);
	deg = std::asin(CAR_LENGTH / ro) * TO_DEGREES;

	if(speed)
		deg += ADJUST_DEGREE * f32(gap.target - gap) / gap.target;

	steer.update(std::round(deg));
	on_change(steer.curr, [this](auto deg_prev, auto deg)
	{
		logger->debug("S: {:3} -> {:3} - gap: {:3} cam: {:3}", deg_prev, deg, i32(gap), i32(cam));
		steering(deg);
		adjust_speed(speed.target);
	});
}

void Adjust::steer_update(i32 deg)
{
	steer.target = deg;
	adjust_steer(deg);
}

void Adjust::gap_update(i32 mm)
{
	gap.update(mm);
	adjust_steer(steer.target);
}

void Adjust::cam_update(i32 diff)
{
	cam.update(diff);
	adjust_speed(speed.target);
}
