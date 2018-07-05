
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
	auto steering = try_init<PWM>("steering", def::STEER_DC_PERIOD);

	struct {
		i32 steer_pwm = def::STEER_DC_DEF;
		u8 speed = proto::Speed::STOP;
		u8 speed_prev = proto::Speed::STOP;
		u8 gap = conf.gap_test;
		i32 align = 0;
	} state;

	auto steer = [&](i32 pwm_corr, i32 pwm_old)
	{
		if(state.steer_pwm == pwm_corr) return;

		pwm_corr = clamp(pwm_corr, def::STEER_DC_SCALE.min, def::STEER_DC_SCALE.max);

		state.steer_pwm = pwm_corr;
		logger->debug("HW: steer: {:7} -> {:7} - gap: {}", pwm_old, pwm_corr, state.gap);
		if(steering)
			steering->set_duty_cycle(pwm_corr);
	};

	auto drive = [&](auto speed_corr, auto speed_old)
	{
		if(state.speed == speed_corr) return;

		state.speed = speed_corr;
		logger->debug("HW: motor: {:02x} -> {:02x} - gap: {}", speed_old, speed_corr, state.gap);
		if(driver)
			driver->drive(speed_corr);
	};


	struct {
		std::unique_ptr<SyncCamera> driver;
		std::thread thread;
		std::atomic<int> value;
		int center = 0;
	} cam;

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

						if(conf.is_slave && state.speed != proto::Speed::STOP)
						{
							auto speed_corr = state.speed;
							if(std::abs(cam.center-state.align)<=3) {}
							else if(0 > cam.center-state.align)
								speed_corr += 2;
							else if(0 < cam.center-state.align)
								speed_corr -= 2;

							drive(speed_corr, state.speed);
						}
					}
					if(cam.center!=0 && state.align<0) {
						cam.center=0;
						logger->debug("CAM pattern lost, err: {}", state.align);
					}


				});
			}
		}
	}

	logger->info("connecting with id {} to {}:{}", conf.common.name, conf.common.host, conf.common.port);
	MQTTClient cl(ioctx, conf.common.host, conf.common.port, conf.common.name);
	cl.connect({});

	cl.subscribe(def::MOTOR_SUB, [&](const std::string& str)
	{
		using proto::Speed;

		i32 input = std::atoi(str.c_str());
		u8 speed = map_dual<u8>(input,
								def::MOTOR_SCALE.min, 0, def::MOTOR_SCALE.max,
								Speed::BACK_FULL, Speed::STOP, Speed::FORWARD_FULL);

		if(state.speed != speed)
		{
			auto speed_corr = speed;

			if(conf.is_slave)
				speed_corr = adjust_speed(state.steer_pwm, speed, state.gap, cam.center-state.align);

			drive(speed_corr, speed);
		}
	});

	if(driver && conf.is_slave)
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
					logger->debug("GAP ERR {} ", ec.message());
				else
				{
					if(!init) {
						values.fill(cm);
						state.gap = cm;
						init = 1;
					} else
					{
						if(i >= values.size()) {
							i=0;
						}
						values[i] = cm;
						i++;

						auto a = values;
						std::sort(a.begin(), a.end());
						auto median = a[a.size()/2];

						auto pwm_corr = state.steer_pwm;
						if(state.gap > median){
							pwm_corr += 30000;
						}else if(state.gap < median){
							pwm_corr -= 30000;
						}
						state.gap = median;
						steer(pwm_corr, state.steer_pwm);
					}
				}
			});
		});
	}

	if(steering)
	{
		steering->set_duty_cycle(def::STEER_DC_DEF);
		steering->enable(true);
	}

	cl.subscribe(def::STEER_SUB, [&](const std::string& str)
	{
		constexpr u32 STEER_DC_MASTER_LIMIT = 1713876;

		u32 pwm = map(std::atoi(str.c_str()),
					  def::STEER_SCALE.min, def::STEER_SCALE.max,
					  def::STEER_DC_SCALE.min, def::STEER_DC_SCALE.max);

		auto pwm_corr = pwm;
		if(conf.is_slave){
			pwm_corr = adjust_steer(pwm, state.gap);
		}
		else if(pwm > STEER_DC_MASTER_LIMIT){
			pwm_corr = STEER_DC_MASTER_LIMIT;
		}
		steer(pwm_corr, pwm);

		if(conf.is_slave && state.speed != proto::Speed::STOP)
		{
			auto speed = state.speed;
			auto speed_corr = adjust_speed(state.steer_pwm, speed, state.gap, cam.center-state.align);
			drive(speed, speed_corr);
		}
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
