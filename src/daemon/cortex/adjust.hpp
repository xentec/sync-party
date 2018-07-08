#pragma once

#include "def.hpp"
#include "logger.hpp"
#include "types.hpp"


constexpr auto STEER_MASTER_LIMIT = 8;
constexpr auto ADJUST_DEGREE = 3;

struct Adjust
{
	struct Value
	{
		operator i32() { return curr; };
	private:
		friend struct Adjust;
		i32 prev = 0, curr = 0;
		inline i32 update(i32 new_val) { auto p = prev; prev = curr; curr = new_val; return p; }
	};

	Value speed, direction, gap, cam;
	const bool is_slave;

	Adjust(bool is_slave);

	void speed_update(i32 spd);
	void direction_update(i32 deg);
	void gap_update(i32 mm);
	void cam_update(i32 diff);

	std::function<void(i32 speed)> drive;
	std::function<void(i32 degree)> steer;

private:
	loggr logger;
};

