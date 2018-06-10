#pragma once

#include "asio.hpp"
#include "types.hpp"

#include <boost/asio/posix/stream_descriptor.hpp>

struct Steering
{
	Steering(io_context &ioctx);
	~Steering();

	void steer(i16 degree);
private:
	void steer_pwm(u32 pwm);

	posix::stream_descriptor sd;
};

