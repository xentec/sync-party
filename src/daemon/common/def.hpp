#pragma once

/**
 * @brief Main definitions for daemons
*/

namespace def
{

struct Scale
{
	int min, max;
	int range =
	        ((min > 0) ? 1 : -1) * min +
	        ((max > 0) ? 1 : -1) * max;
};

constexpr auto HOST = "sp-master";
constexpr auto PORT = "4444";

constexpr auto STEER_SUB = "sp/steer";
constexpr Scale	STEER_SCALE { -90, 90 };
constexpr auto STEER_DEF = (STEER_SCALE.min + STEER_SCALE.max) / 2;

constexpr auto MOTOR_SUB = "sp/motor";
constexpr Scale MOTOR_SCALE { -16, 16 };

}
