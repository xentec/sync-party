#pragma once

#include "types.hpp"
#include "def.hpp"

constexpr auto STEER_MASTER_LIMIT = 8;
constexpr auto ADJUST_DEGREE = 3;

i32 adjust_speed(i32 degree, i32 speed, i32 gap_mm, int cam);
i32 adjust_steer(i32 degree, i32 gap_mm);
