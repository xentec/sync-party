#pragma once

#include "types.hpp"

u8 adjust_speed(u32 steer, u8 speed, u8 gap_cm, int cam);
u32 adjust_steer(u32 steer, u8 gap_cm);
