#include "pwm.hpp"

#include "def.hpp"
#include "util.hpp"

#include <unistd.h>
#include <fcntl.h>

#include <spdlog/fmt/bundled/format.h>
#include <experimental/filesystem>

constexpr u32 STEER_DC_PERIOD = 20000000;

constexpr def::Scale
    STEER_DC_SCALE { 1200000, 1800000 },
    STEER_DC_SCALE_CRIT { 500000, 2500000 };

namespace fs = std::experimental::filesystem;

static const fs::path path_ctl = "/sys/class/pwm/pwmchip0";
static const fs::path path_pwm = "/sys/class/pwm/pwmchip0/pwm0";

template<class Numeral>
inline isz sysfs_write(int fd, Numeral n)
{
	auto buf = fmt::FormatInt(n);
	return write(fd, buf.data(), buf.size());
}

PWM::PWM(u32 period)
{
	int fd;
	isz len;

	if(!fs::exists(path_pwm))
	{
		fd = open((path_ctl / "export").c_str(), O_WRONLY);
		if(0> fd)
			throw std::system_error(errno, std::system_category(), "path_ctl open");

		len = write(fd, "0", 1);
		if(0> len)
			throw std::system_error(errno, std::system_category(), "export");

		close(fd);
	}

	fd = open((path_pwm / "period").c_str(), O_WRONLY);
	if(0> fd)
		throw std::system_error(errno, std::system_category(), "period open");

	len = sysfs_write(fd, period);
	if(0> len)
		throw std::system_error(errno, std::system_category(), "period write");
	close(fd);


	fds.dc = open((path_pwm / "duty_cycle").c_str(), O_WRONLY);
	if(0> fds.dc)
		throw std::system_error(errno, std::system_category(), "duty_cycle");

	fds.enable = open((path_pwm / "enable").c_str(), O_WRONLY);
	if(0> fds.enable)
		throw std::system_error(errno, std::system_category(), "enable");
}

PWM::~PWM()
{
	enable(false);

	// disable pwm0
	int fd = open((path_ctl / "unexport").c_str(), O_WRONLY);
	write(fd, "0", 1);
	close(fd);
}

void PWM::enable(bool on)
{
	sysfs_write(fds.enable, i32(on));
}

void PWM::set_duty_cycle(u32 pwm)
{
	sysfs_write(fds.dc, pwm);
}


const def::Scale Steering::limit
{
	map(STEER_DC_SCALE.min, STEER_DC_SCALE_CRIT.min, STEER_DC_SCALE_CRIT.max, -90, 90),
	map(STEER_DC_SCALE.max, STEER_DC_SCALE_CRIT.min, STEER_DC_SCALE_CRIT.max, -90, 90)
};

Steering::Steering()
    : logger(new_loggr("steering"))
    , pwm_ctrl(STEER_DC_PERIOD)
{
	steer(0);
	pwm_ctrl.enable(true);
}


bool Steering::steer(i32 degree)
{
	const auto deg = clamp(degree, limit.min, limit.max);
	const auto pwm = map(deg, -90, 90, STEER_DC_SCALE_CRIT.min, STEER_DC_SCALE_CRIT.max);

	bool clamped = degree != deg;

	logger->trace("dc: {:3} -> {:7}{}", deg, pwm, clamped ? " !" : "");
	pwm_ctrl.set_duty_cycle(pwm);

	return !clamped;
}
