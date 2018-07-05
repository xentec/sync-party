/* Copyright (c) 2018 LIV-T GmbH */
#pragma once

#include "asio.hpp"
#include "logger.hpp"
#include "types.hpp"

#include <boost/asio/ip/tcp.hpp>

#include <unordered_map>

namespace mqtt
{
template <typename Socket> class client;
template <typename Socket, typename Strand> class tcp_endpoint;
}

struct MQTTClient
{
	MQTTClient(io_context &ctx, const std::string &host, const std::string &port, const std::string &id);

	void connect(std::function<void(bool clean, u8)> cb);

	using SubCB = std::function<void(const std::string& contents)>;
	void subscribe(const std::string& topic, u8 qos, SubCB cb);
	void subscribe(const std::string& topic, SubCB cb);

	void publish(const std::string& topic, const std::string& content);

private:
	loggr logger;
	std::shared_ptr<mqtt::client<mqtt::tcp_endpoint<ip::tcp::socket, io_context::strand>>> client;

	struct Sub { u8 qos; SubCB cb; };
	std::unordered_map<std::string, Sub> callbacks;
};

