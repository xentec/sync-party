
#include "asio.hpp"
#include "def.hpp"
#include "echo.hpp"
#include "logger.hpp"
#include "types.hpp"
#include "util.hpp"

#include "driver.hpp"
#include "pwm.hpp"

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
	auto steering = try_init<PWM>("steering", def::STEER_DC_PERIOD);

	std::unordered_map<std::string, std::function<void(std::string)>> fn_map;
	if(driver)
	{
		fn_map.emplace(def::MOTOR_SUB, [&](const std::string& str)
		{
			i32 input = std::atoi(str.c_str());

			u8 speed = Driver::Speed::STOP;
			if(input < 0) // NOTE: |[STOP, BACK_FULL]| != |[STOP, FORWARD_FULL]|
				speed = map<u8>(-input, 0, def::MOTOR_SCALE.max, Driver::Speed::STOP, Driver::Speed::BACK_FULL);
			else if(input > 0)
				speed = map<u8>(input, 0, def::MOTOR_SCALE.max, Driver::Speed::STOP, Driver::Speed::FORWARD_FULL);

			static u8 speed_prev = Driver::Speed::STOP;
			if(speed_prev != speed)
			{
				speed_prev = speed;
				logger->debug("HW: motor: {:02x}", speed);
				driver->drive(speed);
			}
		});
	}
	if(steering)
	{
		steering->set_duty_cycle(def::STEER_DC_DEF);
		steering->enable(true);
		fn_map.emplace(def::STEER_SUB, [&](const std::string& str)
		{
			u32 pwm = map(std::atoi(str.c_str()),
						  def::STEER_SCALE.min, def::STEER_SCALE.max,
						  def::STEER_DC_SCALE.min, def::STEER_DC_SCALE.max);

			static u32 pwm_prev = 0;
			if(pwm_prev != pwm)
			{
				pwm_prev = pwm;
				logger->debug("HW: steer: {:9}", pwm);
				steering->set_duty_cycle(pwm);
			}
		});
	}

	logger->info("connecting to {}:{}", def::HOST, def::PORT);
	auto mqtt_cl = mqtt::make_client(ioctx, def::HOST, def::PORT);
	auto &cl = *mqtt_cl;

	cl.set_client_id(NAME); // TODO: id spec
	cl.set_clean_session(true);

	struct {
		u16 steer, motor;
	} sub;

	// Setup handlers
	cl.set_connack_handler([&] (bool sp, std::uint8_t connack_return_code)
	{
		auto rc_str = mqtt::connect_return_code_to_str(connack_return_code);
		logger->debug("connack: clean: {}, ret code: {}", sp, rc_str);
		if (connack_return_code != mqtt::connect_return_code::accepted)
			logger->error("failed to connect: ret code {}", rc_str);
		else
		{
			sub.motor = cl.subscribe(def::MOTOR_SUB, mqtt::qos::at_most_once);
			sub.steer = cl.subscribe(def::STEER_SUB, mqtt::qos::at_most_once);
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

		logger->trace("data: {}[{}]: {}", topic_name, packet_id.get_value_or(-1), contents);
		itr->second(contents);

		return true;
	});

	cl.connect();

	Echo echo(ioctx, 31337, NAME, std::chrono::seconds(10));

	signal_set stop(ioctx, SIGINT, SIGTERM);
	stop.async_wait([&](auto, int) { ioctx.stop(); });

	logger->info("running...");
	ioctx.run();

	return 0;
}
