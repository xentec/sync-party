
#include "asio.hpp"
#include "def.hpp"
#include "echo.hpp"
#include "logger.hpp"
#include "types.hpp"
#include "util.hpp"

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
//	Controller ctrl(ioctx, Controller::Joystick, "/dev/input/js0");
	Controller ctrl(ioctx, Controller::Keyboard, "/dev/input/event20");

	logger->info("connecting to {}:{}", def::HOST, def::PORT);
	auto mqtt_cl = mqtt::make_client(ioctx, def::HOST, def::PORT);
	auto &cl = *mqtt_cl;
	cl.set_client_id("ctrl");
	cl.set_clean_session(true);
	cl.connect();

	auto forward = [&](const std::string& sub, i32 value)
	{
		auto str = fmt::FormatInt(value).c_str();
		logger->debug("PUB: {}: {:6}", sub, str);
		cl.publish(sub, str);
	};

	auto motor = [&](i32 v) { on_change(v, [&](auto v){ forward(def::MOTOR_SUB, v); }); };
	auto steer = [&](i32 v) { on_change(v, [&](auto v){ forward(def::STEER_SUB, v); }); };

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

		motor(map<i16>(input_state[Controller::RT2] - input_state[Controller::LT2],
					   axis_min*2, axis_max*2,
					   def::MOTOR_SCALE.min, def::MOTOR_SCALE.max));
		steer(map<i32, i32>(input_state[Controller::LS_H],
							Controller::axis_min, Controller::axis_max,
							def::STEER_SCALE.min, def::STEER_SCALE.max));
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

		auto i = input_state.find(code);
		if(i == input_state.end())
			return;

		i->second = val > 0;

		motor(def::MOTOR_SCALE.max * (input_state[KEY_W] - input_state[KEY_S]));
		steer(def::STEER_SCALE.max * (input_state[KEY_A] - input_state[KEY_D]));
	};

#ifdef MOCK_CLIENT
	auto test_cl = mqtt::make_client(ioctx, def::HOST, def::PORT);
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
				sub.motor = cl.subscribe(def::MOTOR_SUB, mqtt::qos::at_most_once);
				sub.steer = cl.subscribe(def::STEER_SUB, mqtt::qos::at_most_once);
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
	stop.async_wait([&](auto ec, int sig) { ioctx.stop(); });

	logger->info("running...");
	ioctx.run();

	return 0;
}
