#pragma once

#include "def.hpp"
#include "logger.hpp"
#include "types.hpp"

struct Adjust
{
	struct Position { u16 left, right; };
	struct Value
	{
		operator i32() { return curr; };
	private:
		friend struct Adjust;
		i32 prev = 0, curr = 0, target = 0;
		inline i32 update(i32 new_val) { auto p = prev; prev = curr; curr = new_val; return p; }
		inline i32 diff() const { return prev - curr; }
	};

	Value speed, steer, gap, cam;
	const Position pos;

	Adjust(Position&& pos);

	void speed_update(i32 spd);
	void steer_update(i32 deg);
	void gap_update(i32 mm);
	void cam_update(i32 diff);

	std::function<void(i32 speed)> drive;
	std::function<void(i32 degree)> steering;

private:
	void adjust_speed(i32 spd);
	void adjust_steer(i32 deg);

	loggr logger;
};

