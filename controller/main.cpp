
#include "asio.hpp"
#include "types.hpp"
#include "util.hpp"
#include "logger.hpp"
#include "echo.hpp"

#include "controller.hpp"

#include <mqtt/client.hpp>
#include <linux/input-event-codes.h>

/* master
 * controller -> mqtt
 * slave
 * mqtt -> driver, steering
*/

static std::string NAME = "sp-1";


int main()
{
	slog::set_level(slog::level::trace);
	slog::set_pattern("[%Y-%m-%d %H:%M:%S %L] %n: %v");

	auto logger = slog::stdout_color_st("app");
	logger->info("sp-controller v0.1");

	io_context ioctx;

	logger->info("initialising controller...");
	Controller ctrl(ioctx, Controller::Joystick, "/dev/input/js0");

	auto mqtt_cl = mqtt::make_client(ioctx, "localhost", 4444);
	auto &cl = *mqtt_cl;
	cl.set_client_id("ctrl");
	cl.set_clean_session(true);
	cl.connect();

	ctrl.on_axis = [&](u32, Controller::Axis num, i16 val)
	{
		static std::unordered_map<Controller::Axis, i16> input_state
		{
			{ Controller::LS_H, 0 },
			{ Controller::LT2, Controller::axis_min },
			{ Controller::RT2, Controller::axis_min },
		};

		constexpr i32 axis_min = Controller::axis_min, axis_max = Controller::axis_max;

		auto i = input_state.find(num);
		if(i == input_state.end())
			return;

		i->second = val;

		i32 input;

		// motor control
		//###############
		input = input_state[Controller::RT2] - input_state[Controller::LT2];
		static i32 motor_input_prev = 0;
		if(motor_input_prev != input)
		{
			motor_input_prev = input;

			static i16 speed_prev = 0;
			i16 speed = 0;

			if(input < 0)
				speed = map<i16>(input, 0, axis_min*2, 0, -1000);
			else if(input > 0)
				speed = map<i16>(input, 0, axis_max*2, 0, 1000);

			if(speed != speed_prev)
			{
				speed_prev = speed;
				logger->debug("motor: {:6} => {:5}", input, speed);
				cl.publish("sp/motor", fmt::FormatInt(speed).str());
			}
		}

		// steer control
		//###############
		input = input_state[Controller::LS_H];
		static i16 steer_prev = 0;
		i16 steer = map<i16, i32>(input, Controller::axis_min, Controller::axis_max, -900, 900);
		if(steer_prev != steer)
		{
			steer_prev = steer;
			auto str = fmt::format("{:6}", steer);
			logger->debug("steer: {}", str);
			cl.publish("sp/steer", str);
		}
	};

	ctrl.on_key = [&](u32, u32 code, Controller::KeyState val)
	{
		static std::unordered_map<u32, i32> input_state
		{
			{ KEY_W, 0 },
			{ KEY_S, 0 },
			{ KEY_A, 0 },
			{ KEY_D, 0 },
		};

		i32 input;
		auto i = input_state.find(code);
		if(i == input_state.end())
			return;

		i->second = val > 0;

		// motor control
		//###############
		input = input_state[KEY_W] - input_state[KEY_S];
		static i32 motor_input_prev = 0;
		if(motor_input_prev != input)
		{
			motor_input_prev = input;

			static i16 speed_prev = 0;
			i16 speed = input * 1000;

			if(speed != speed_prev)
			{
				speed_prev = speed;
				auto str = fmt::FormatInt(speed).c_str();
				logger->debug("motor: {:6}", str);
				cl.publish("sp/motor", str);
			}
		}

		// steer control
		//###############
		input = input_state[KEY_A] - input_state[KEY_D];
		static i16 steer_prev = 0;
		i16 steer = input * 9000;
		if(steer_prev != steer)
		{
			steer_prev = steer;
			auto str = fmt::FormatInt(steer).c_str();
			logger->debug("steer: {:6}", str);
			cl.publish("sp/steer", str);
		}
	};

#ifdef MOCK_CLIENT
	auto test_cl = mqtt::make_client(ioctx, "localhost", 4444);
	{
		auto &cl = *test_cl;
		cl.set_client_id("drv-1"); // TODO: id spec
		cl.set_clean_session(true);
		cl.connect();

		struct {
			u16 steer, motor;
		} sub;

		// Setup handlers
		cl.set_connack_handler([&] (bool sp, std::uint8_t connack_return_code)
		{
			console->debug("connack: clean: {}, ret code: {}", sp, mqtt::connect_return_code_to_str(connack_return_code));
			if (connack_return_code == mqtt::connect_return_code::accepted)
			{
				sub.steer = cl.subscribe("sp/steer", mqtt::qos::at_most_once);
				sub.motor = cl.subscribe("sp/motor", mqtt::qos::at_most_once);
			}
			return true;
		});
		cl.set_close_handler([&]
		{
			console->info("disconnected");
		});

		cl.set_publish_handler([&](u8 fixed_header, boost::optional<u16> packet_id, const std::string& topic_name, const std::string& contents)
		{
			console->info("pub: hdr: {}, id: {}, topic: {}, content: {}", fixed_header, packet_id.get_value_or(-1), topic_name, contents);
			return true;
		});
	}
#endif

	Echo echo(ioctx, 31337, NAME, std::chrono::seconds(10));


	signal_set stop(ioctx, SIGINT, SIGTERM);
	stop.async_wait([&](auto ec, int sig)
	{
		ioctx.stop();
	});


	logger->debug("running...");
	ioctx.run();

	return 0;
}
