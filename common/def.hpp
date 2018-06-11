#pragma once

namespace def {

struct Scale { int min, max; };

constexpr auto HOST = "base";
constexpr unsigned short PORT = 4444;

constexpr auto STEER_SUB = "sp/steer";
constexpr Scale
	STEER_SCALE { -900, 900 },
	STEER_DC_SCALE { 1200000, 1800000 },
	STEER_DC_SCALE_CRIT { 500000, 2500000 };
constexpr auto STEER_DC_DEF = STEER_DC_SCALE.min + (STEER_DC_SCALE.max - STEER_DC_SCALE.min)/2;
constexpr auto STEER_DC_PERIOD = 20000000;

constexpr auto MOTOR_SUB = "sp/motor";
constexpr Scale MOTOR_SCALE { -200, 200 };

}
