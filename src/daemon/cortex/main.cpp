
#include "asio.hpp"
#include "def.hpp"
#include "echo.hpp"
#include "logger.hpp"
#include "opts.hpp"
#include "types.hpp"
#include "util.hpp"

#include "adjust.hpp"
#include "driver.hpp"
#include "pwm.hpp"

#include <mqtt/client.hpp>
#include <mqtt/str_connect_return_code.hpp>

#include <fmt/format.h>

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>

#include "camera_opencv.h"
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
	bool is_slave;
	u32 gap_test = 0;
} conf;

int main(int argc, const char* argv[])
{
	slog::set_level(slog::level::trace);
	slog::set_pattern("[%Y-%m-%d %H:%M:%S %L] %n: %v");

	conf.common.name = NAME;

	argh::parser opts(argc, argv);
	parse_common_opts(opts, conf.common, false);

	conf.is_slave = opts["-S"];
	opts({"-g", "--gap"}, conf.gap_test) >> conf.gap_test;

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

    volatile std::atomic<int> return_value;
	int center_camera;
	return_value = 0;
    std::thread camthread (cam.start_sync_camera,&return_value);
	logger->info("Looking for pattern ...");
    while(return_value.load()==-1) {
		usleep(30000);
	}
    if(return_value.load()==-2) {
		logger->info("Tracking error");
		return -1;
	}

    if(return_value.load()>0) {
        logger->info("Tracking started: {}",return_value.load());
        center_camera = return_value.load();
	}


	logger->info("connecting with id {} to {}:{}", conf.common.name, conf.common.host, conf.common.port);
	auto mqtt_cl = mqtt::make_client(ioctx, conf.common.host, conf.common.port);
	auto &cl = *mqtt_cl;

	cl.set_client_id(conf.common.name);
	cl.set_clean_session(true);

	struct {
		u32 steer_pwm = def::STEER_DC_DEF;
		u8 speed = proto::Speed::STOP;
		u8 gap = conf.gap_test;
		i32 align = 0;
	} control_state;

	std::unordered_map<std::string, std::function<void(std::string)>> fn_map;


	fn_map.emplace(def::MOTOR_SUB, [&](const std::string& str)
	{
		i32 input = std::atoi(str.c_str());

		u8 speed = proto::Speed::STOP;
		if(input < 0) // NOTE: |[STOP, BACK_FULL]| != |[STOP, FORWARD_FULL]|
			speed = map<u8>(input, def::MOTOR_SCALE.min, 0, proto::Speed::BACK_FULL, proto::Speed::STOP);
		else if(input > 0)
			speed = map<u8>(input, 0, def::MOTOR_SCALE.max, proto::Speed::STOP, proto::Speed::FORWARD_FULL);

		if(control_state.speed != speed)
		{
			auto speed_corr = control_state.speed = speed;

			if(conf.is_slave)
				speed_corr = adjust_speed(control_state.steer_pwm, speed, control_state.gap, 0);

			logger->debug("HW: motor: {:02x} -> {:02x} - gap: {}", speed, speed_corr, control_state.gap);

			if(driver)
				driver->drive(speed);
		}
	});

	if(driver && conf.is_slave)
	{
		auto gap_timer = std::make_shared<recur_timer>(ioctx);
		gap_timer->start(std::chrono::milliseconds(100), [&, gap_timer](auto ec)
		{
			if(ec) return;

			static u8 pin = 7;

			pin = pin == 8 ? 7 : 8;

			driver->gap(pin, [&](auto ec, u8 cm)
			{
				if(ec)
					logger->debug("GAP ERR {} ", ec.message());
				else
				{
					control_state.gap = cm;
				}
			});
		});
	}


    if(driver && conf.is_slave)
    {
        auto gap_timer_cam = std::make_shared<recur_timer>(ioctx);
        gap_timer->start(std::chrono::milliseconds(100), [&, gap_timer_cam]()
        {

        logger->debug("CAM VALUE {} ", return_value.load());
        });

    }

	if(steering)
	{
		steering->set_duty_cycle(def::STEER_DC_DEF);
		steering->enable(true);
	}

	fn_map.emplace(def::STEER_SUB, [&](const std::string& str)
	{
		u32 pwm = map(std::atoi(str.c_str()),
					  def::STEER_SCALE.min, def::STEER_SCALE.max,
					  def::STEER_DC_SCALE.min, def::STEER_DC_SCALE.max);

		if(control_state.steer_pwm != pwm)
		{
			auto pwm_corr = control_state.steer_pwm = pwm;
			if(conf.is_slave)
				pwm_corr = adjust_steer(pwm, control_state.gap);

			logger->debug("HW: steer: {:7} -> {:7} - gap: {}", pwm, pwm_corr, control_state.gap);
			if(steering)
				steering->set_duty_cycle(pwm_corr);
		}
	});



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

		logger->trace("data: {}: {}", topic_name, contents);
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
    camthread.join();
	return 0;
}
