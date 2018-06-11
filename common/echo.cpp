#include "echo.hpp"

Echo::Echo(io_context& ioctx, u16 port, std::string payload, std::chrono::steady_clock::duration interval)
	: socket(ioctx, ip::udp::v4())
	, ep(ip::address_v4::broadcast(), port)
	, timer(ioctx)
	, payload(std::move(payload))
	, interval(interval)
{
	socket.set_option(ip::udp::socket::reuse_address(true));
	socket.set_option(socket_base::broadcast(true));

	broadcast({});
}

void Echo::broadcast(error_code ec)
{
	if(ec) return;
	socket.send_to(buffer(payload), ep);
	timer.expires_from_now(interval);
	timer.async_wait([this](auto ec){ broadcast(ec); });
}

