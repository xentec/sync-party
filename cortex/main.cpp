
#include "asio.hpp"
#include "types.hpp"
#include "util.hpp"

#include "controller.hpp"
#include "driver.hpp"
#include "steering.hpp"

#include <fmt/format.h>

int main()
{
	io_context ioctx;

	Controller ctrl(ioctx, "/dev/input/js0");
	Driver driver(ioctx, "/dev/ttyACM0");
	Steering steering(ioctx);

	ctrl.on_axis = [&](u32, Controller::Axis num, i16 val)
	{
		static std::unordered_map<Controller::Axis, i16> input_state
		{
			{ Controller::LS_H, 0 },
			{ Controller::LT2, Controller::min },
			{ Controller::RT2, Controller::min },
		};

		constexpr i32 axis_min = Controller::min, axis_max = Controller::max;

		auto i = input_state.find(num);
		if(i == input_state.end())
			return;

		i->second = val;

		// motor control
		//###############
		i32 input = input_state[Controller::RT2] - input_state[Controller::LT2];
		static i32 motor_input_prev = 0;
		if(motor_input_prev != input)
		{
			motor_input_prev = input;

			u8 speed = Driver::Speed::STOP;
			if(input < 0)
				speed = map<u8>(input, 0, axis_min*2, Driver::Speed::STOP, Driver::Speed::BACK_FULL);
			else
				speed = map<u8>(input, 0, axis_max*2, Driver::Speed::STOP, Driver::Speed::FORWARD_FULL);

			static u8 speed_prev = Driver::Speed::STOP;
			if(speed != speed_prev)
			{
				speed_prev = speed;
				fmt::print("MOTOR: {:5} => {:02x}\n", input, speed);
				driver.drive(speed);
			}
		}

		// steer control
		//###############
		static i16 steer_input_prev = 0;
		i16 &steer = input_state[Controller::LS_H];
		if(steer_input_prev != steer)
		{
			steer_input_prev = steer;
			steering.steer(steer);
		}
	};

	ioctx.run();

	return 0;
}
