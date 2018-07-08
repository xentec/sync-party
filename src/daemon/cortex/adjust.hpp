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
		i32 prev = 0, curr = 0;
		inline i32 update(i32 new_val) { auto p = prev; prev = curr; curr = new_val; return p; }
		inline i32 diff() const { return prev - curr; }
	};

	Value speed, direction, gap, cam;
	const Position pos;

	Adjust(Position&& pos);

	void speed_update(i32 spd);
	void direction_update(i32 new_deg);
	void gap_update(i32 mm);
	void cam_update(i32 diff);

	std::function<void(i32 speed)> drive;
	std::function<void(i32 degree)> steer;

private:
	loggr logger;
	i32 target_gap;
};

