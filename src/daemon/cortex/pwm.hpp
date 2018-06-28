#pragma once

#include "asio.hpp"
#include "types.hpp"

struct PWM
{
	PWM(u32 period);
	~PWM();

	void enable(bool on);
	void set_duty_cycle(u32 pwm);
private:
	struct {
		int dc, enable;
	} fds;
};

