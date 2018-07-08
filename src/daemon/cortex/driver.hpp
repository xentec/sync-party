#pragma once

#include "asio.hpp"
#include "def.hpp"
#include "logger.hpp"
#include "types.hpp"

#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>

#include <deque>

struct Driver
{
	static const def::Scale limit;

	Driver(io_context& ioctx, const char* dev_path);

	void drive(i32 speed);
	void gap(u8 sensor, std::function<void(std::error_code, u8 cm)> callback);
	void analog(u8 pin, std::function<void(std::error_code, u8 v)> callback);
	void version(std::function<void(std::error_code, u8 ver)> callback);

private:
	using buffer_iter = buffers_iterator<const_buffers_1>;

	void send(u8 type, u8 value, std::function<void(std::error_code, u8 cm)> cb = {});

	void send_start();
	void send_handle(std::error_code ec, usz len);

	void recv_start();
	void recv_handle(std::error_code ec, usz len);

	void timeout_start();
	void timeout_handle();

	void on_packet(u8 type, u8 value);
	void wd_feed(std::error_code err);

	loggr logger;
	serial_port dev;
	streambuf buf_r, buf_w;
	steady_timer timer;
	u8 timeout_num;

	enum ParseState
	{
		SYNC, DATA
	} parse_state;

	struct Req
	{
		u8 type;
		std::function<void(std::error_code, u8 cm)> cb;
	};
	std::deque<Req> q;

	struct {
		steady_timer feeder;
		u8 curr;
	} speed_ctrl;
};
