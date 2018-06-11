#pragma once

namespace def {

struct Scale { int min, max; };

constexpr auto HOST = "base";
constexpr unsigned short PORT = 4444;

constexpr auto STEER_SUB = "sp/steer";
constexpr Scale STEER_SCALE { -900, 900 };

constexpr auto MOTOR_SUB = "sp/motor";
constexpr Scale MOTOR_SCALE { -200, 200 };

}
