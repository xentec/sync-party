
#include "asio.hpp"
#include "types.hpp"

#include "controller.hpp"
#include "driver.hpp"

#include <fmt/format.h>

template<class I, class O>
O map(I x, I in_min, I in_max, O out_min, O out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


template<class T>
using lim = std::numeric_limits<T>;

int main()
{
	io_context ioctx;

	Controller ctrl(ioctx, "/dev/input/js0");
	Driver driver(ioctx, "/dev/ttyACM0");

	ctrl.on_axis = [&](u32 time, u8 num, i16 val)
	{
		switch(num)
		{
		case Controller::RT2:
		{
			u8 d = map(u16(lim<i16>::max() + val), u16(0), lim<u16>::max(), u8(0x30), u8(0x90));
			fmt::print("{:010} MOTOR: {:02x}\n", time, d);
			driver.drive(d);
		}
		default:
			return;
		}
	};

	ioctx.run();

	return 0;
}
