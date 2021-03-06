
#include "asio.hpp"
#include "def.hpp"
#include "echo.hpp"
#include "logger.hpp"
#include "net.hpp"
#include "opts.hpp"
#include "types.hpp"
#include "util.hpp"

#include "controller.hpp"

#include <boost/asio/signal_set.hpp>
#include <linux/input-event-codes.h>

/* master
 * controller -> mqtt
 * slave
 * mqtt -> driver, steering
*/

static const std::string NAME = "sp-ctrl";

struct {
	CommonOpts common;
	std::string dev_path = "/dev/input/js0";
	def::Scale speed = def::MOTOR_SCALE;
} conf;


int main(int argc, const char* argv[])
{
	slog::set_level(slog::level::trace);
	slog::set_pattern("[%Y-%m-%d %H:%M:%S %L] %n: %v");

	// custom default common values
	conf.common.name = NAME;

	// handle command line options
	argh::parser opts(argc, argv);
	conf.common.parse(opts);

	Controller::Type ctrl_type = Controller::Joystick;
	if(opts[{"-K", "--keyboard"}]) ctrl_type = Controller::Keyboard;
	opts({"-D", "--device"}, conf.dev_path) >> conf.dev_path;
	opts({"--spd-max"}, conf.speed.max) >> conf.speed.max;
	opts({"--spd-min"}, conf.speed.min) >> conf.speed.min;

	// let's go!
	auto logger = new_loggr("app");
	logger->info("sp-controller v0.1");

	io_context ioctx;

	logger->info("initialising controller...");
	Controller ctrl (ioctx, ctrl_type, conf.dev_path);

	logger->info("connecting with id {} to {}:{}", conf.common.name, conf.common.host, conf.common.port);
	MQTTClient cl (ioctx, conf.common.host, conf.common.port, conf.common.name);

	cl.set_will(def::MOTOR_SUB, "0");
	cl.connect();

	// helper for publishing MQTT messages
	auto forward = [&](const std::string& sub, i32 value_old, i32 value)
	{
		auto str = fmt::format_int(value).str();
		logger->debug("PUB: {}: {:3} -> {:3}", sub, value_old, str);
		cl.publish(sub, str);
	};

	// helpers for publishing respective messages of motor/steering
	auto motor = [&](i32 v){ on_change(v, [&](auto p, auto v){ forward(def::MOTOR_SUB, p, v); }); };
	auto steer = [&](i32 v){ on_change(v, [&](auto p, auto v){ forward(def::STEER_SUB, p, v); }); };

	// stop when controller went missing
	bool err = false;
	ctrl.on_err = [&](auto){ err = true, motor(0), steer(0); };

	// handle gamepad input events
	ctrl.on_axis = [&](u32, Controller::Axis num, i16 val)
	{
		// track axis state
		static std::unordered_map<Controller::Axis, i16> input_state_default
		{
			{ Controller::LS_H, 0 }, // left and right
			{ Controller::LT2, Controller::axis_min }, // backward
			{ Controller::RT2, Controller::axis_min }, // forward
		};
		static std::unordered_map<Controller::Axis, i16> input_state = input_state_default;

		if(err) // reset input state on error
			input_state = input_state_default;

		auto i = input_state.find(num);
		if(i == input_state.end())
			return;

		i->second = val;

		//logger->trace("on_axis: {:2} - {:6}", i->first, i->second);

		// calculate speed diff: forward - back
		i32 speed_input = input_state[Controller::RT2] - input_state[Controller::LT2];
		i32 speed_mapped =
		        map_dual(speed_input,
		                 Controller::axis_min * 2,  Controller::axis_max * 2,
		                 conf.speed.min, conf.speed.max);

		logger->trace("speed: {:6} -> {:4}", speed_input, speed_mapped);
		motor(speed_mapped);

		i32 steer_input = input_state[Controller::LS_H];
		i32 steer_mapped =
		        map<i32, i32>(steer_input,
		                      Controller::axis_min, Controller::axis_max,
		                      def::STEER_SCALE.min, def::STEER_SCALE.max);

		logger->trace("steer: {:6} -> {:4}", steer_input, steer_mapped);
		steer(steer_mapped);
	};

	// handle keyboard events
	ctrl.on_key = [&](u32, u32 code, Controller::KeyState val)
	{
		// track key state
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

		// simple binary input: key down -> full speed
		motor(map_dual(input_state[KEY_W] - input_state[KEY_S], -1, 1, conf.speed.min, conf.speed.max));
		steer(def::STEER_SCALE.max * (input_state[KEY_D] - input_state[KEY_A]));
	};

	// in case the daemon needs to be found on a convoluted network
	std::shared_ptr<Echo> echo;
	if(conf.common.echo_broadcast)
		echo = std::make_shared<Echo>(ioctx, 31337, conf.common.name, std::chrono::seconds(10));

	// stop on system signal
	signal_set stop(ioctx, SIGINT, SIGTERM);
	stop.async_wait([&](auto, int) { ioctx.stop(); });

	// fire off the event loop
	logger->info("running...");
	ioctx.run();

	return 0;
}
