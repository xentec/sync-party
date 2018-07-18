/* Copyright (c) 2018 LIV-T GmbH */
#pragma once

#include "asio.hpp"
#include "logger.hpp"
#include "types.hpp"

#include <boost/asio/ip/tcp.hpp>

#include <unordered_map>

// forwarding declarations to not include the header only mqtt_cpp and make compilation really long
namespace mqtt
{
template <typename Socket> class client;
template <typename Socket, typename Strand> class tcp_endpoint;
}

/**
 * @brief Abstraction class for mqtt_cpp
 * @author Andrej Utz
 * @author Adrian Leher
 */
struct MQTTClient
{
	/**
	 * @brief Constructor and initializer.
	 * @param ctx   Managing io_contex from Asio
	 * @param host  Hostname of the broker to resolve and connect to
	 * @param port  Port broker is listening on
	 * @param id    MQTT id this instance shall have
	 */
	MQTTClient(io_context &ctx, const std::string &host, const std::string &port, const std::string &id);

	/**
	 * @brief Connect to broker
	 * @param callback  Will be called when a connection result is achieved
	 */
	void connect(std::function<void(bool clean, u8)> callback = {});

	/**
	 * @brief Callback type for MQTT subscriptions
	 */
	using SubCB = std::function<void(const std::string& contents)>;
	/**
	 * @brief Subscribe to topic and react will a callback to publishments.
	 * @param topic      Topic to subscribe to
	 * @param qos        MQTT QoS parameter
	 * @param callback   Will be called when the broker forwards a published message on the topic
	 */
	void subscribe(const std::string& topic, u8 qos, SubCB callback);
	/**
	 * @overload void subscribe(const std::string& topic, u8 qos, SubCB callback);
	 */
	void subscribe(const std::string& topic, SubCB callback);

	/**
	 * @brief Publish a message on a topic
	 * @param topic     Topic to publish on
	 * @param content   Payload of the message
	 */
	void publish(const std::string& topic, const std::string& content);
	/**
	 * @brief Set a predefined message the broker shall publish in the event of the connection loss.
	 * @param topic     Topic to publish on
	 * @param content   Payload of the message
	 */
	void set_will(const std::string& topic, const std::string& content);

private:
	loggr logger;
	std::shared_ptr<mqtt::client<mqtt::tcp_endpoint<ip::tcp::socket, io_context::strand>>> client;

	struct Sub { u8 qos; SubCB cb; };
	std::unordered_map<std::string, Sub> callbacks;
};

