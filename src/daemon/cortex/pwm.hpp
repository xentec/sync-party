#pragma once

#include "asio.hpp"
#include "def.hpp"
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

struct Steering
{
	static const def::Scale limit;

	Steering();
	bool steer(i32 deg);

private:
	PWM pwm_ctrl;
	u32 pwm_prev;
};
