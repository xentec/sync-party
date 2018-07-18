/* Copyright (c) 2018 LIV-T GmbH */
#include "timer.hpp"

Timer::Timer(io_context &ctx)
	: timer(ctx)
{}

void Timer::start(steady_timer::duration interval, TimerCB callback)
{
	fn = callback;
	ival = interval;
	run({});
}

void Timer::stop()
{
	timer.cancel();
}

void Timer::run(std::error_code ec)
{
	fn(ec);

	if(ec) return;

	timer.expires_from_now(ival);
	timer.async_wait([this](auto ec) { run(ec); });
}
