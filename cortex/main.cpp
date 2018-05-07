
#include "asio.hpp"
#include "types.hpp"

#include "controller.hpp"
#include "driver.hpp"

#include <fmt/format.h>

template<class O, class I>
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

	ctrl.on_axis = [&](u32 time, Controller::Axis num, i16 val)
	{
		static u8 motor_speed_prev = Driver::Speed::STOP;
		static std::unordered_map<Controller::Axis, i16> input {
			{ Controller::LT2, 0 },
			{ Controller::RT2, 0 },
		};

		input[num] = val;

		i32 motor_input = input[Controller::RT2] - input[Controller::LT2];

		u8 speed = Driver::Speed::STOP;
		if(motor_input < 0)
			speed = map<u8>(motor_input, 0, -i32(lim<u16>::max()), Driver::Speed::STOP, Driver::Speed::BACK_FULL);
		else
			speed = map<u8>(motor_input, 0,  i32(lim<u16>::max()), Driver::Speed::STOP, Driver::Speed::FORWARD_FULL);

		if(speed != motor_speed_prev)
		{
			motor_speed_prev = speed;
			fmt::print("{:10} MOTOR: {:5} => {:02x}\n", time, motor_input, speed);
			driver.drive(speed);
		}
	};

	ioctx.run();

	return 0;
}
