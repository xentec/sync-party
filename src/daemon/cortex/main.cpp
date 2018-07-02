
#include "asio.hpp"
#include "def.hpp"
#include "echo.hpp"
#include "logger.hpp"
#include "opts.hpp"
#include "types.hpp"
#include "util.hpp"

#include "driver.hpp"
#include "pwm.hpp"

#include <mqtt/client.hpp>
#include <mqtt/str_connect_return_code.hpp>

#include <fmt/format.h>

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>

#include "camera/camera_opencv.h"
#include <pthread.h>
#include <unistd.h>


/* slave
 * mqtt -> driver, steering
*/

static std::string NAME = "sp-drv-0";

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

struct
{
	CommonOpts common;
} conf;

int main(int argc, const char* argv[])
{
	slog::set_level(slog::level::trace);
	slog::set_pattern("[%Y-%m-%d %H:%M:%S %L] %n: %v");

	conf.common.name = NAME;

	argh::parser opts(argc, argv);
	parse_common_opts(opts, conf.common, false);

	logger = slog::stdout_color_st("cortex");
	logger->info("sp-cortex v0.1");

	io_context ioctx;

	logger->info("initialising hardware...");

	auto driver = try_init<Driver>("driver", ioctx, "/dev/ttyACM0");
	auto steering = try_init<PWM>("steering", def::STEER_DC_PERIOD);

        SyncCamera cam(0);

        logger->info("cam {} initialized",0);

        cam.set_resolution(320,240);
        cam.set_matchval(0.7);
        cam.set_pattern("OTH_logo_small_2.png");

        pthread_t thread;
        camera_thread_data data;
        volatile int return_value;
        int center_camera;
        return_value = 0;
        data.cap = &cam;
        data.returnvalue = &return_value;
        pthread_create(&thread,NULL,StartSyncCamera,(void*) &data);
        logger->info("Looking for pattern ...");
        while(return_value==-1) {
            usleep(30000);
        }
        if(return_value==-2) {
            logger->info("Tracking error");
            return -1;
        }

        if(return_value>0) {
            logger->info("Tracking started: {}",return_value);
            center_camera = return_value;
        }


	logger->info("connecting with id {} to {}:{}", conf.common.name, conf.common.host, conf.common.port);
	auto mqtt_cl = mqtt::make_client(ioctx, conf.common.host, conf.common.port);
	auto &cl = *mqtt_cl;

	cl.set_client_id(conf.common.name);
	cl.set_clean_session(true);

	std::unordered_map<std::string, std::function<void(std::string)>> fn_map;
	if(driver)
	{
		fn_map.emplace(def::MOTOR_SUB, [&](const std::string& str)
		{
			i32 input = std::atoi(str.c_str());

			u8 speed = proto::Speed::STOP;
			if(input < 0) // NOTE: |[STOP, BACK_FULL]| != |[STOP, FORWARD_FULL]|
				speed = map<u8>(input, def::MOTOR_SCALE.min, 0, proto::Speed::BACK_FULL, proto::Speed::STOP);
			else if(input > 0)
				speed = map<u8>(input, 0, def::MOTOR_SCALE.max, proto::Speed::STOP, proto::Speed::FORWARD_FULL);

			static u8 speed_prev = proto::Speed::STOP;
			if(speed_prev != speed)
			{
				speed_prev = speed;
                                logger->info("Camera value: {}",return_value);
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

	std::shared_ptr<Echo> echo;
	if(conf.common.echo_broadcast)
		echo = std::make_shared<Echo>(ioctx, 31337, conf.common.name, std::chrono::seconds(10));

	signal_set stop(ioctx, SIGINT, SIGTERM);
	stop.async_wait([&](auto, int) { ioctx.stop(); });

	logger->info("running...");
	ioctx.run();

	return 0;
}
