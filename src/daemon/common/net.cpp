/* Copyright (c) 2018 LIV-T GmbH */
#include "net.hpp"

#include <mqtt/client.hpp>
#include <mqtt/str_connect_return_code.hpp>

MQTTClient::MQTTClient(io_context &ctx, const std::string &host, const std::string &port, const std::string &id)
    : logger(new_loggr("net"))
    , client(mqtt::make_client(ctx, host, port))
{
	client->set_clean_session(true);
	client->set_client_id(id);

	client->set_close_handler([this]
	{
		logger->info("disconnected");
	});

	client->set_publish_handler([this](u8, auto,
	                       const std::string& topic_name,
	                       const std::string& contents)
	{
		auto itr = callbacks.find(topic_name);
		if(itr == callbacks.end())
			return true;

		logger->trace("data: {}: {}", topic_name, contents);
		itr->second.cb(contents);

		return true;
	});
}

void MQTTClient::connect(std::function<void (bool, u8)> callback)
{
	client->set_connack_handler([this, callback] (bool sp, u8 connack_return_code)
	{
		auto rc_str = mqtt::connect_return_code_to_str(connack_return_code);
		logger->debug("connack: clean: {}, ret code: {}", sp, rc_str);

		if (connack_return_code != mqtt::connect_return_code::accepted)
		{
			logger->error("failed to connect: ret code {}", rc_str);
			return true;
		}

		logger->info("connected");

		if(callback)
			callback(sp, connack_return_code);

		for(const auto& p: callbacks)
			client->async_subscribe(p.first, p.second.qos);

		return true;
	});
	client->connect([this](auto ec)
	{
		if(ec)
			logger->error("failed to connect: {}", ec.message());
	});
}

void MQTTClient::subscribe(const std::string &topic, u8 qos, SubCB callback)
{
	callbacks.emplace(topic, Sub{qos, callback});
	if(client->connected())
		client->async_subscribe(topic, qos);
}

void MQTTClient::subscribe(const std::string &topic, MQTTClient::SubCB callback)
{
	subscribe(topic, mqtt::qos::exactly_once, callback);
}

void MQTTClient::publish(const std::string &topic, const std::string &content)
{
	client->async_publish(topic, content);
}

void MQTTClient::set_will(const std::string& topic, const std::string& content)
{
	client->set_will(mqtt::will(topic, content));
}
