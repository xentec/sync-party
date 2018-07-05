#pragma once

#include "asio.hpp"
#include "logger.hpp"
#include "types.hpp"

#include <boost/asio/streambuf.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

struct Controller
{
	enum Type
	{
		Joystick, Keyboard
	};
	enum Axis
	{
		LS_H = 0,
		LS_V = 1,
		LT2 = 2,
		RT2 = 5,
	};
	enum KeyState
	{
		Pressed, Released, Repeating
	};

	static constexpr i16 axis_min = std::numeric_limits<i16>::min() + 1;
	static constexpr i16 axis_max = std::numeric_limits<i16>::max();

	Controller(io_context& ctx, Type type, const char* dev_path);

	Type get_type() const;

	std::function<void(u32 time, Axis num, i16 val)> on_axis;
	std::function<void(u32 time, u32 keycode, KeyState val)> on_key;

private:
	void recv_start();
	void recv_handle(std::error_code ec, usz len);
	void recv_handle_js(std::error_code ec, usz len);
	void recv_handle_kb(std::error_code ec, usz len);

	std::function<void(std::error_code ec, usz len)> recv_handler;

	loggr logger;
	Type type;
	posix::stream_descriptor sd;
	streambuf buf;
};

