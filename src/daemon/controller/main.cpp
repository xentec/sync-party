
#include "asio.hpp"
#include "def.hpp"
#include "echo.hpp"
#include "logger.hpp"
#include "opts.hpp"
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

static const std::string NAME = "sp-ctrl";

struct {
	CommonOpts common;
	def::Scale speed;
} conf;


int main(int argc, const char* argv[])
{
	slog::set_level(slog::level::trace);
	slog::set_pattern("[%Y-%m-%d %H:%M:%S %L] %n: %v");

	conf.common.name = NAME;
	conf.speed = def::MOTOR_SCALE;

	argh::parser opts(argc, argv);
	parse_common_opts(opts, conf.common);

	opts({"--spd-max"}, conf.speed.max) >> conf.speed.max;
	opts({"--spd-min"}, conf.speed.min) >> conf.speed.min;

	auto logger = slog::stdout_color_st("app");
	logger->info("sp-controller v0.1");

	io_context ioctx;

	logger->info("initialising controller...");
	Controller ctrl(ioctx, Controller::Joystick, "/dev/input/js0");
//	Controller ctrl(ioctx, Controller::Keyboard, "/dev/input/by-path/platform-i8042-serio-0-event-kbd");

	logger->info("connecting with id {} to {}:{}", conf.common.name, conf.common.host, conf.common.port);
	auto mqtt_cl = mqtt::make_client(ioctx, conf.common.host, conf.common.port);
	auto &cl = *mqtt_cl;
	cl.set_client_id(conf.common.name);
	cl.set_clean_session(true);
	cl.connect();

	auto forward = [&](const std::string& sub, i32 value)
	{
		auto str = fmt::FormatInt(value).str();
		logger->debug("PUB: {}: {:6}", sub, str);
		cl.publish(sub, str);
	};

	auto motor = [&](i32 v) { on_change(v, [&](auto v){ forward(def::MOTOR_SUB, v); }); };
	auto steer = [&](i32 v) { on_change(v, [&](auto v){ forward(def::STEER_SUB, v); }); };

	ctrl.on_err = [&](auto)
	{
		motor(0);
		steer(0);
	};

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

		//logger->trace("on_axis: {:2} - {:6}", i->first, i->second);

		i32 speed = input_state[Controller::RT2] - input_state[Controller::LT2];
		i32 speed_mapped = map_dual(speed,
							   axis_min*2, axis_max*2,
							   conf.speed.min, conf.speed.max);

		logger->trace("speed: {:6} -> {:4}", speed, speed_mapped);


		motor(speed_mapped);
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

		logger->trace("on_key: {:2} - {:6}", i->first, i->second);

		motor(def::MOTOR_SCALE.max / 4 * (input_state[KEY_S] - input_state[KEY_W]));
		steer(def::STEER_SCALE.max * (input_state[KEY_D] - input_state[KEY_A]));
	};

#ifdef MOCK_CLIENT
	auto test_cl = mqtt::make_client(ioctx, conf.common.host, conf.common.port);
	{
		auto &cl = *test_cl;
		cl.set_client_id(conf.common.name); // TODO: id spec
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

	std::shared_ptr<Echo> echo;
	if(conf.common.echo_broadcast)
		echo = std::make_shared<Echo>(ioctx, 31337, conf.common.name, std::chrono::seconds(10));

	signal_set stop(ioctx, SIGINT, SIGTERM);
	stop.async_wait([&](auto, int) { ioctx.stop(); });

	logger->info("running...");
	ioctx.run();

	return 0;
}
