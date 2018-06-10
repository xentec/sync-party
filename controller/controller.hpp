#pragma once

#include "asio.hpp"
#include "types.hpp"
#include "logger.hpp"

#include <boost/asio/streambuf.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

struct Controller
{
	enum Axis
	{
		LS_H = 0,
		LS_V = 1,
		LT2 = 2,
		RT2 = 5,
	};

	static constexpr i16 min = std::numeric_limits<i16>::min() + 1;
	static constexpr i16 max = std::numeric_limits<i16>::max();

	Controller(io_context& ctx, const char* dev_path);

	std::function<void(u32 time, Axis num, i16 val)> on_axis;

private:
	void recv_start();
	void recv_handle(error_code ec, usz len);

	loggr logger;
	posix::stream_descriptor sd;
	streambuf buf;
};

