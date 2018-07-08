
#include "asio.hpp"
#include "def.hpp"
#include "echo.hpp"
#include "logger.hpp"
#include "net.hpp"
#include "opts.hpp"
#include "timer.hpp"
#include "types.hpp"
#include "util.hpp"

#include "adjust.hpp"
#include "camera_opencv.hpp"
#include "driver.hpp"
#include "pwm.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/signal_set.hpp>

/* slave
 * mqtt -> driver, steering
*/

#define GAP_ARRAY_LEN 5
static std::string NAME = "sp-drv-0";

loggr logger;

template<class T, class ...Args>
std::unique_ptr<T> try_init(const std::string& name, Args&& ...args)
{
	try {
		return std::make_unique<T>(args...);
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
	struct {
		std::string pattern_path;
		f32 match_value;
	} cam;
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
	opts({"--cam-pattern"}, conf.cam.pattern_path) >> conf.cam.pattern_path;
	opts({"--cam-match-val"}, conf.cam.match_value) >> conf.cam.match_value;

	logger = slog::stdout_color_st("cortex");
	logger->info("sp-cortex v0.1");

	io_context ioctx;

	logger->info("initialising hardware...");

	auto driver = try_init<Driver>("driver", ioctx, "/dev/ttyACM0");
	auto steering = try_init<Steering>("steering");

	struct {
		i32 steer_deg = 0;
		i32 speed = 0;
		i32 speed_prev = 0;
		i32 gap_mm = conf.gap_test;
		i32 align = 0;
	} state;

	struct {
		std::unique_ptr<SyncCamera> driver;
		std::thread thread;
		std::atomic<int> value;
		int center = 0;
	} cam;

	auto drive = [&](i32 speed)
	{
		auto speed_corr = speed;

		if(conf.is_slave)
			speed_corr = adjust_speed(state.steer_deg, speed, state.gap_mm, cam.center-state.align);

		if(state.speed == speed_corr) return;
		state.speed = speed_corr;

		logger->debug("HW: motor: {:3} -> {:3} - gap: {}", speed, speed_corr, state.gap_mm);
		if(driver)
			driver->drive(speed_corr);
	};

	auto steer = [&](i32 deg)
	{
		auto deg_corr = deg;
		if(conf.is_slave){
			deg_corr = adjust_steer(deg, state.gap_mm);
		}
		else
			deg_corr = clamp(deg_corr, Steering::limit.min + STEER_MASTER_LIMIT, Steering::limit.max - STEER_MASTER_LIMIT);

		state.steer_deg = deg_corr;

		logger->debug("HW: steer: {:3} -> {:3} - gap: {}", deg, deg_corr, state.gap_mm);
		if(steering)
			steering->steer(deg_corr);

		if(conf.is_slave && state.speed)
		{
			auto speed_corr = adjust_speed(state.steer_deg, state.speed, state.gap_mm, cam.center-state.align);
			drive(speed_corr);
		}

	};


	if(conf.is_slave)
	{
		if(0> access(conf.cam.pattern_path.c_str(), R_OK))
			logger->error("cam pattern not found at {}: {}", conf.cam.pattern_path, strerror(errno));
		else
		{
			cam.driver = try_init<SyncCamera>("camera", "/dev/video0", conf.cam.pattern_path);
			if(cam.driver)
			{
				cam.driver->set_resolution(320,240);
				cam.driver->set_matchval(conf.cam.match_value);
				cam.thread = std::thread([&](auto *atom){ cam.driver->start_sync_camera(atom); }, &cam.value);

				logger->info("cam {} initialized", 0);

				cam.value.store(0);

				auto cam_timer = std::make_shared<Timer>(ioctx);
				cam_timer->start(std::chrono::milliseconds(500), [&, cam_timer](auto ec)
				{
					if(ec) return;

					state.align = cam.value.load();
					if(cam.center==0 && state.align>0) {
						cam.center=state.align;
						logger->info("CAM initialized to: {}",cam.center);
					}
					if(cam.center!=0 && state.align>=0) {
						logger->debug("CAM value: {}, diff: {}", state.align, cam.center-state.align);

						if(state.speed && std::abs(cam.center-state.align) > 3)
							drive(state.speed);
					}
					if(cam.center!=0 && state.align<0) {
						cam.center=0;
						logger->debug("CAM pattern lost, err: {}", state.align);
					}
				});
			}
		}

		if(driver)
		{
			auto gap_timer = std::make_shared<Timer>(ioctx);
			gap_timer->start(std::chrono::milliseconds(100), [&, gap_timer](auto ec)
			{
				if(ec) return;

				static u8 pin = 7, i = 0, init = 0;
				static std::array<u8, GAP_ARRAY_LEN> values;

				driver->gap(pin, [&](auto ec, u8 cm)
				{
					if(ec)
					{
						on_change(ec, [&](auto, auto ec)
						{
							logger->debug("GAP ERR {} ", ec.message());
						});
						return;
					}

					const i32 mm = cm * 10;
					if(!init) {
						values.fill(mm);
						state.gap_mm = mm;
						init = 1;
					} else
					{
						values[i] = mm;
						if(++i == values.size())
							i = 0;

						// get median of low pass
						auto a = values;
						std::sort(a.begin(), a.end());
						state.gap_mm = a[a.size()/2];
						steer(state.steer_deg);
					}

				});
			});
		}
	}


	logger->info("connecting with id {} to {}:{}", conf.common.name, conf.common.host, conf.common.port);
	MQTTClient cl(ioctx, conf.common.host, conf.common.port, conf.common.name);
	cl.connect({});

	cl.subscribe(def::MOTOR_SUB, [&](const std::string& str)
	{
		auto speed = map_dual(std::atoi(str.c_str()),
		                    def::MOTOR_SCALE.min, def::MOTOR_SCALE.max,
		                    Driver::limit.min, Driver::limit.max);
		drive(speed);
	});


	cl.subscribe(def::STEER_SUB, [&](const std::string& str)
	{

		auto deg = map(std::atoi(str.c_str()),
		              def::STEER_SCALE.min, def::STEER_SCALE.max,
		              Steering::limit.min, Steering::limit.max);
		steer(deg);
	});

	std::shared_ptr<Echo> echo;
	if(conf.common.echo_broadcast)
		echo = std::make_shared<Echo>(ioctx, 31337, conf.common.name, std::chrono::seconds(10));

	signal_set stop(ioctx, SIGINT, SIGTERM);
	stop.async_wait([&](auto, int) { ioctx.stop(); });

	logger->info("running...");
	ioctx.run();

	return 0;
}
