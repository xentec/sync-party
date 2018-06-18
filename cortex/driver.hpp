#pragma once

#include "asio.hpp"
#include "types.hpp"
#include "logger.hpp"

#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>

#include <deque>

struct Driver
{
	enum Type : u8
	{
		PING  = 0x0,
		MOTOR,
		GAP,
		ANALOG,

		VERSION = 0xFE,
		ERR = 0xFF
	};

	enum Speed : u8
	{
//		BACK_FULL    = 0x10,
		BACK_FULL    = 0x20,
		STOP         = 0x30,
		FORWARD_FULL = 0x60,
//		FORWARD_FULL = 0x90,
	};

	Driver(io_context& ioctx, const char* dev_path);

	void drive(u8 speed);
	void gap(u8 sensor, std::function<void(error_code, u8 cm)> callback);
	void analog(u8 pin, std::function<void(error_code, u8 v)> callback);
	void version(std::function<void(error_code, u8 ver)> callback);

private:
	using buffer_iter = buffers_iterator<const_buffers_1>;

	void send(Type type, u8 value, std::function<void(error_code, u8 cm)> cb = {});

	void send_start();
	void send_handle(error_code ec, usz len);

	void recv_start();
	void recv_handle(error_code ec, usz len);

	void on_packet(Type type, u8 value);
	void wd_feed(error_code err);

	loggr logger;
	serial_port dev;
	streambuf buf_r, buf_w;

	enum ParseState
	{
		SYNC, DATA
	} parse_state;
	enum SYNC_BYTE { BEGIN = '[', END = ']' };

	struct Req
	{
		u8 type;
		std::function<void(error_code, u8 cm)> cb;
	};
	std::deque<Req> q;

	struct {
		steady_timer feeder;
		u8 curr;
	} speed_ctrl;
};
