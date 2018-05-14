#include "steering.hpp"

#include "controller.hpp"
#include "util.hpp"

#include <fmt/posix.h>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

enum {
	PERIOD =      20000000,

	DC_SERVO_MIN =  500000,
	DC_SERVO_MAX = 2500000,
	DC_PHY_MIN =   1200000,
	DC_PHY_MAX =   1800000,
};

static const fs::path path_ctl = "/sys/class/pwm/pwmchip0";
static const fs::path path_pwm = "/sys/class/pwm/pwmchip0/pwm0";

Steering::Steering(io_context &ioctx)
	: sd(ioctx)
{
	return;

	int fd = open((path_ctl / "export").c_str(), O_WRONLY);
	if(0> fd)
		throw std::system_error(errno, std::system_category(), "failed to init steering: path_ctl");

	isz len = write(fd, "0\n", 2);
	if(0> len)
		throw std::system_error(errno, std::system_category(), "failed to init steering: export");

	close(fd);

	fd = open((path_pwm / "period").c_str(), O_WRONLY);
	if(0> fd)
		throw std::system_error(-fd, std::system_category(), "failed to init steering: period open");

	auto period = fmt::format("{}\n", PERIOD);
	len = write(fd, period.data(), period.size());
	if(0> len)
		throw std::system_error(errno, std::system_category(), "failed to init steering: period write");
	close(fd);

	fd = open((path_pwm / "duty_cycle").c_str(), O_WRONLY);
	if(0> fd)
		throw std::system_error(-fd, std::system_category());

	sd.assign(fd);

	steer(i8(0));

	fd = open((path_pwm / "enable").c_str(), O_WRONLY);
	if(0> fd)
		throw std::system_error(-fd, std::system_category(), "failed to init steering: enable");
	len = write(fd, "1\n", 2);
	close(fd);
}

Steering::~Steering()
{
	steer(i8(0));
	sd.close();

	// disable pwm0
	int fd = open((path_ctl / "unexport").c_str(), O_WRONLY);
	write(fd, "0\n", 2);
	close(fd);
}

void Steering::steer(i8 degree)
{
	steer(map<u32, i8>(degree, -90, 90, DC_SERVO_MIN, DC_SERVO_MAX));
}

void Steering::steer(i16 axis_input)
{
	u32 pwm = map<f32>(axis_input, Controller::min, Controller::max, DC_PHY_MIN, DC_PHY_MAX);
	steer(pwm);
}

void Steering::steer(u32 pwm)
{
	if(pwm > DC_PHY_MAX)
		pwm = DC_PHY_MAX;
	else if(pwm < DC_PHY_MIN)
		pwm = DC_PHY_MIN;

	sd.write_some(buffer(fmt::format("{}\n", pwm)));
}

