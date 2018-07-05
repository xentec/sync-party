#include "pwm.hpp"

#include <unistd.h>
#include <fcntl.h>

#include <spdlog/fmt/bundled/format.h>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

static const fs::path path_ctl = "/sys/class/pwm/pwmchip0";
static const fs::path path_pwm = "/sys/class/pwm/pwmchip0/pwm0";

PWM::PWM(u32 period)
{
	int fd;
	isz len;

	if(!fs::exists(path_pwm))
	{
		fd = open((path_ctl / "export").c_str(), O_WRONLY);
		if(0> fd)
			throw std::system_error(errno, std::system_category(), "path_ctl open");

		len = write(fd, "0\n", 2);
		if(0> len)
			throw std::system_error(errno, std::system_category(), "export");

		close(fd);
	}

	fd = open((path_pwm / "period").c_str(), O_WRONLY);
	if(0> fd)
		throw std::system_error(errno, std::system_category(), "period open");

	auto period_str = fmt::format("{}\n", period);
	len = write(fd, period_str.data(), period_str.size());
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
	write(fd, "0\n", 2);
	close(fd);
}

void PWM::enable(bool on)
{
	auto str = fmt::format("{:d}\n", on);
	write(fds.enable, str.data(), str.size());
}

void PWM::set_duty_cycle(u32 pwm)
{
	auto str = fmt::format("{}\n", pwm);
	write(fds.dc, str.data(), str.size());
}

