#pragma once

#include "types.hpp"
#include "proto-def.hpp"

u8 adjust_speed(u32 steer, u8 speed, i32 gap_mm, int cam);
u32 adjust_steer(u32 steer, i32 gap_mm);
