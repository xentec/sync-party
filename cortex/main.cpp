
#include "asio.hpp"
#include "types.hpp"
#include "util.hpp"

#include "driver.hpp"
#include "steering.hpp"

#include "logger.hpp"

#include <mqtt/client.hpp>
#include <mqtt/str_connect_return_code.hpp>

#include <fmt/format.h>

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>

/* slave
 * mqtt -> driver, steering
*/

static std::string NAME = "drv-1";

loggr logger;

template<class T, class ...Args>
std::shared_ptr<T> try_init(const std::string& name, Args&& ...args)
{
	try {
		return std::make_shared<T>(args...);
	} catch(std::runtime_error& ex)
	{
		logger->error("failed to init {}: {}", name, ex.what());
	}
	return nullptr;
}

int main()
{
	slog::set_level(slog::level::trace);
	slog::set_pattern("[%Y-%m-%d %H:%M:%S %L] %n: %v");

	logger = slog::stdout_color_st("cortex");
	logger->info("sp-cortex v0.1");

	io_context ioctx;

	logger->info("initialising hardware...");

	auto driver = try_init<Driver>("driver", ioctx, "/dev/ttyACM0");
	auto steering = try_init<Steering>("steering", ioctx);

	std::unordered_map<std::string, std::function<void(std::string)>> fn_map;
	if(steering)
	{
		fn_map.emplace("sp/steer", [&](const std::string& str)
			{
			   static i16 steer_input_prev = 0;
			   i16 steer = std::atoi(str.c_str());
			   if(steer_input_prev != steer)
			   {
				   steer_input_prev = steer;
				   steering->steer(steer);
			   }
			});
	}

	if(driver)
	{
		fn_map.emplace("sp/motor", [&](const std::string& str)
			{
				static i32 motor_input_prev = 0;
				i32 input = std::atoi(str.c_str());
				if(motor_input_prev != input)
				{
					motor_input_prev = input;

					u8 speed = Driver::Speed::STOP;
					if(input < 0)
						speed = map<u8>(input, 0, -1000, Driver::Speed::STOP, Driver::Speed::BACK_FULL);
					else
						speed = map<u8>(input, 0, 1000, Driver::Speed::STOP, Driver::Speed::FORWARD_FULL);

					static u8 speed_prev = Driver::Speed::STOP;
					if(speed != speed_prev)
					{
						speed_prev = speed;
						logger->debug("motor: {:02x}", speed);
						driver->drive(speed);
					}
				}
			});
	}


	auto mqtt_cl = mqtt::make_client(ioctx, "localhost", 4444);
	auto &cl = *mqtt_cl;

	cl.set_client_id(NAME); // TODO: id spec
	cl.set_clean_session(true);

	struct {
		u16 steer, motor;
	} sub;

	// Setup handlers
	cl.set_connack_handler([&] (bool sp, std::uint8_t connack_return_code)
	{
		logger->debug("connack: clean: {}, ret code: {}", sp, mqtt::connect_return_code_to_str(connack_return_code));
		if (connack_return_code == mqtt::connect_return_code::accepted)
		{
			sub.steer = cl.subscribe("/sp/steer", mqtt::qos::at_most_once);
			sub.motor = cl.subscribe("/sp/motor", mqtt::qos::at_most_once);
		}
		return true;
	});
	cl.set_close_handler([&]
	{
		logger->info("disconnected");
	});

	cl.set_publish_handler([&](u8 fixed_header, boost::optional<u16> packet_id, const std::string& topic_name, const std::string& contents)
	{
		auto itr = fn_map.find(topic_name);
		if(itr == fn_map.end())
			return true;

		itr->second(contents);

		return true;
	});

	cl.connect();


	ip::udp::socket echo(ioctx, ip::udp::v4());
	ip::udp::endpoint echo_ep(ip::address_v4::broadcast(), 31337);

	echo.set_option(ip::udp::socket::reuse_address(true));
	echo.set_option(socket_base::broadcast(true));

	auto echo_tmr = std::make_shared<steady_timer>(ioctx);
	std::function<void(error_code ec)> brc;
	brc = [&](error_code ec)
	{
		if(ec) return;
		echo.send_to(buffer(NAME), echo_ep);
		echo_tmr->expires_from_now(std::chrono::seconds(5));
		echo_tmr->async_wait(brc);
	};
	brc({});

	signal_set stop(ioctx, SIGINT, SIGTERM);
	stop.async_wait([&](auto ec, int sig)
	{
		ioctx.stop();
	});


	logger->debug("running...");
	ioctx.run();

	return 0;
}
