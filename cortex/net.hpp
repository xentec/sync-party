#pragma once

#include "asio.hpp"
#include "types.hpp"

#include <mqtt/client.hpp>

struct Slave
{
	Slave(io_context &ioctx, const std::string& client_id, const std::string& host);

	void connect();
private:

	void on_error(error_code ec);
	void on_close();

	std::shared_ptr<mqtt::client<mqtt::tcp_endpoint<ip::tcp::socket, io_context::strand>>> mqcl;
};

