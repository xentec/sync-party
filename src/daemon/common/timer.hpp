/* Copyright (c) 2018 LIV-T GmbH */
#pragma once

#include "asio.hpp"
#include "types.hpp"

#include <boost/asio/steady_timer.hpp>

/**
 * @brief Recuring steady timer helper class
 */
struct Timer
{
	/**
	 * @brief Initialize the timer
	 * @param ctx  Managing io_context from Asio
	 */
	Timer(io_context &ctx);

	/**
	 * @brief Callback type for timer events
	 */
	using TimerCB = std::function<void(std::error_code)>;

	/**
	 * @brief Start this timer to call a callback in given intervals
	 * @warning The callback will also be called at the start.
	 * @param interval  Duration between calls
	 * @param callback  Called function object
	 */
	void start(steady_timer::duration interval, TimerCB callback);
	/**
	 * @brief Stop this timer
	 * Callback will not be called.
	 */
	void stop();

private:
	/**
	 * @brief Internal callback handler
	 * @param ec Possible system error
	 */
	void run(std::error_code ec);

	TimerCB fn;
	steady_timer timer;
	steady_timer::duration ival;
};
