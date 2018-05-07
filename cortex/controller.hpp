#pragma once

#include "asio.hpp"
#include "types.hpp"

#include <boost/asio/streambuf.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

struct Controller
{
	enum Input
	{
		LS_H = 0,
		LS_V = 1,
		RT2 = 5,
	};

	Controller(io_context& ctx, const char* dev_path);

	std::function<void(u32 time, u8 num, i16 val)> on_axis;

private:
	void recv_start();

	void recv_handle(error_code ec, usz len);

	posix::stream_descriptor sd;
	streambuf buf;
};

