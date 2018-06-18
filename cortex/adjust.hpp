#pragma once

#include "asio.hpp"
#include "types.hpp"
#include "logger.hpp"

#include <cmath>

u8 adjustspeed(u32 steer, u8 motor, int us, int cam);
u32 adjustdistance(u32 steer, u8 motor, int us);
