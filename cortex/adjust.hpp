#pragma once

#include "asio.hpp"
#include "types.hpp"
#include "logger.hpp"

#include <cmath>

u8 adjustspeed(u16 steer, u8 motor, int us, int cam);
u16 adjustdistance(u16 steer, u8 motor, int us);
