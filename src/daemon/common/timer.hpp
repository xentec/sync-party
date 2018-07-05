/* Copyright (c) 2018 LIV-T GmbH */
#pragma once

#include "asio.hpp"
#include "types.hpp"

#include <boost/asio/steady_timer.hpp>

struct Timer
{
	Timer(io_context &ioctx);

	using TimerCB = std::function<void(std::error_code)>;
	void start(steady_timer::duration interval, TimerCB cb);
	void stop();
private:
	void run(std::error_code ec);

	TimerCB fn;
	steady_timer timer;
	steady_timer::duration ival;
};
