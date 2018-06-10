
#include "asio.hpp"
#include "types.hpp"
#include "util.hpp"
#include "logger.hpp"

#include "controller.hpp"

#include <mqtt_client_cpp.hpp>

#include <boost/asio/ip/udp.hpp>

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

	auto logger = slog::stdout_color_st("ctrl");
	logger->info("sp-controller v0.1");

	io_context ioctx;

	logger->info("initialising controller...");
	Controller ctrl(ioctx, "/dev/input/js0");

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
			{ Controller::LT2, Controller::min },
			{ Controller::RT2, Controller::min },
		};

		constexpr i32 axis_min = Controller::min, axis_max = Controller::max;

		i32 input;
		auto i = input_state.find(num);
		if(i == input_state.end())
			return;

		i->second = val;

		// motor control
		//###############
		input = input_state[Controller::RT2] - input_state[Controller::LT2];
		static i32 motor_input_prev = 0;
		if(motor_input_prev != input)
		{
			motor_input_prev = input;

			u8 speed = 0;
			if(input < 0)
				speed = map<i16>(input, 0, axis_min*2, 0, -1000);
			else
				speed = map<i16>(input, 0, axis_max*2, 0, 1000);

			static u8 speed_prev = 0;
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
		i16 steer = map<i16, i32>(input, Controller::min, Controller::max, -900, 900);
		if(steer_prev != steer)
		{
			steer_prev = steer;
			auto str = fmt::format("{:6}", steer);
			logger->debug("steer: {}", str);
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

	ip::udp::socket echo(ioctx, ip::udp::v4());
	ip::udp::endpoint echo_ep(ip::address_v4::broadcast(), 31337);

	echo.set_option(ip::udp::socket::reuse_address(true));
	echo.set_option(socket_base::broadcast(true));

	auto echo_tmr = std::make_shared<steady_timer>(ioctx);
	std::function<void(error_code ec)> brc;
	brc = [&](error_code ec)
	{
		if(ec) return;
		echo.send_to(buffer("sp lol"), echo_ep);
		echo_tmr->expires_from_now(std::chrono::seconds(5));
		echo_tmr->async_wait(brc);
	};
	brc({});

	logger->debug("running...");
	ioctx.run();

	return 0;
}
