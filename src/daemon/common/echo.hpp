#pragma once

#include "types.hpp"
#include "asio.hpp"

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>

struct Echo
{
	Echo(io_context& ioctx, u16 port, std::string payload, std::chrono::steady_clock::duration interval);
private:
	void broadcast(std::error_code ec);

	ip::udp::socket socket;
	ip::udp::endpoint ep;
	steady_timer timer;

	std::string payload;
	std::chrono::steady_clock::duration interval;
};

