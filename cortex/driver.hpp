#pragma once

#include "asio.hpp"
#include "types.hpp"
#include "logger.hpp"


#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>



struct Driver
{
	enum Type : u8
	{
		PING  = 0x0,
		MOTOR = 0x1,
		GAP   = 0x2,

		ERR = 0xFF
	};

	enum Speed : u8
	{
		BACK_FULL    = 0x10,
		STOP         = 0x30,
		FORWARD_FULL = 0x90,
	};

	Driver(io_context& ioctx, const char* dev_path);

	void drive(u8 speed);

private:
	using buffer_iter = buffers_iterator<const_buffer>;

	void send(Type type, u8 value);

	void recv_start();
	void recv_handle(error_code ec, usz len);

	void on_packet(Type type, u8 value);
	void wd_feed(error_code err);

	loggr logger;
	serial_port dev;
	steady_timer wd_feeder;
	streambuf buf_r, buf_w;

	enum ParseState
	{
		SYNC, DATA
	} parse_state;
	enum SYNC_BYTE { BEGIN = '[', END = ']' };
};
