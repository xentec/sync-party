
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
		i32 update_interval_ms = 100;
		std::string pattern_path = "pattern.png";
		u32 width = 320, height = 240;
		f32 match_value = 0.6;
	} cam;
} conf;

int main(int argc, const char* argv[])
{
	slog::set_level(slog::level::trace);
	slog::set_pattern("[%Y-%m-%d %H:%M:%S %L] %n: %v");

	// custom default common values
	conf.common.name = NAME;

	// handle command line options
	argh::parser opts(argc, argv);
	conf.common.parse(opts, conf.is_slave);

	conf.is_slave = opts["-S"];
	opts({"-g", "--gap"}, conf.gap_test) >> conf.gap_test;
	opts({"--cam-interval"}, conf.cam.update_interval_ms) >> conf.cam.update_interval_ms;
	opts({"--cam-pattern"}, conf.cam.pattern_path) >> conf.cam.pattern_path;
	opts({"--cam-match-val"}, conf.cam.match_value) >> conf.cam.match_value;

	// let's go!
	logger = new_loggr("cortex");
	logger->info("sp-cortex v0.1");

	io_context ioctx;

	logger->info("initialising hardware...");

	auto driver = try_init<Driver>("driver", ioctx, "/dev/ttyACM0");
	auto steering = try_init<Steering>("steering");

	// hardcoded test scenario
	Adjust adj(Adjust::Line{1, conf.is_slave});

	// set control callbacks
	adj.drive = [&](auto speed){ if(driver) driver->drive(speed); };
	adj.steering = [&](auto deg){ if(steering) steering->steer(deg); };
	adj.gap_update(conf.gap_test);

	logger->info("connecting with id {} to {}:{}", conf.common.name, conf.common.host, conf.common.port);
	MQTTClient cl(ioctx, conf.common.host, conf.common.port, conf.common.name);
	cl.connect();

	// camera data
	struct {
		std::unique_ptr<SyncCamera> driver;
		std::thread thread;
		std::atomic<int> value;
		int center = 0, align = 0;
	} cam;

	if(conf.is_slave)
	{
		// setup camera
		// first we need a pattern to match
		if(0> access(conf.cam.pattern_path.c_str(), R_OK))
			logger->error("cam pattern not found at {}: {}", conf.cam.pattern_path, strerror(errno));
		else
		{
			// then initialize hardware
			cam.driver = try_init<SyncCamera>("camera", "/dev/video0", conf.cam.pattern_path);
			if(cam.driver)
			{
				cam.driver->set_resolution(conf.cam.width, conf.cam.height);
				cam.driver->set_matchval(conf.cam.match_value);
				cam.thread = std::thread([&](auto *atom){ cam.driver->start_sync_camera(atom); }, &cam.value);

				logger->info("cam {} initialized", 0);

				cam.value.store(0);

				// check the offset in intervals
				auto cam_timer = std::make_shared<Timer>(ioctx);
				cam_timer->start(std::chrono::milliseconds(conf.cam.update_interval_ms), [&, cam_timer](auto ec)
				{
					if(ec) return;

					auto align = cam.value.load();
					if(cam.center==0 && align>0) {
						cam.center = align;
						logger->info("CAM initialized to: {}",cam.center);
					}
					if(cam.center!=0 && align>=0) {
						adj.cam_update(f32(cam.center-align) / conf.cam.width);
					}
					if(cam.center!=0 && align<0) {
						cam.center=0;
						logger->debug("CAM pattern lost, err: {}", align);
					}
				});
			}
		}

		if(driver && conf.gap_test == 0)
		{
			// start gap updater
			auto gap_timer = std::make_shared<Timer>(ioctx);
			gap_timer->start(std::chrono::milliseconds(50), [&, gap_timer](auto ec)
			{
				if(ec) return;

				static u8 pin = 7, i = 0, init = 0;
				static std::array<u8, 3> values;

				driver->gap(pin, [&](auto ec, u8 mm)
				{
					if(ec)
					{
						on_change(ec, [&](auto, auto ec)
						{
							logger->debug("GAP ERR {} ", ec.message());
						});
						return;
					}

					if(!init) {
						values.fill(mm);
						init = 1;
					} else
					{
						values[i] = mm;
						if(++i == values.size())
							i = 0;

						// get median of low pass
						auto a = values;
						std::sort(a.begin(), a.end());
						mm = a[a.size()/2];
					}

					adj.gap_update(mm);
					cl.publish("sp/gap", fmt::format("{}", mm));
				});
			});
		}
	}

	cl.subscribe("sp/gap", [&](const std::string& str)
	{
		i32 mm = std::atoi(str.c_str());
		if(!adj.gap)
			adj.gap_inner(mm);
	});

	// receive speed input
	cl.subscribe(def::MOTOR_SUB, [&](const std::string& str)
	{
		// map network speed to ours
		auto speed = map_dual(std::atoi(str.c_str()),
		                      def::MOTOR_SCALE.min, def::MOTOR_SCALE.max,
		                      Driver::limit.min, Driver::limit.max);
		adj.speed_update(speed);
	});

	// ...and steer input
	cl.subscribe(def::STEER_SUB, [&](const std::string& str)
	{
		// map network degree to ours
		auto deg = map(std::atoi(str.c_str()),
		               def::STEER_SCALE.min, def::STEER_SCALE.max,
		               Steering::limit.min, Steering::limit.max);
		adj.steer_update(deg);
	});

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
