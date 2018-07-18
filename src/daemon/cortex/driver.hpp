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

/**
 * @brief Communication class with Driver component
 */
struct Driver
{
	/**
	 * @brief Speed limit
	 */
	static const def::Scale limit;

	/**
	 * @param ctx        Managing io_context from Asio
	 * @param dev_path   Device file of Driver (e.g. /dev/ttyACM0 for Arduino)
	 */
	Driver(io_context& ctx, const char* dev_path);

	/**
	 * @brief Motor control function
	 * @param speed  Drive speed. Will be clamped with Driver::limit
	 */
	void drive(i32 speed);
	/**
	 * @brief Query ultra sonic sensors for distance.
	 * @param pin       Pin the sensors uses
	 * @param callback  Function to call with gap distance in mm
	 */
	void gap(u8 pin, std::function<void(std::error_code, u8 mm)> callback);
	/**
	 * @brief Query analog pins
	 * @param pin       Analog pin
	 * @param callback  Function to call with voltage value
	 */
	void analog(u8 pin, std::function<void(std::error_code, u8 v)> callback);
	/**
	 * @brief Query Driver firmware version
	 * @param callback  Function to call with version
	 */
	void version(std::function<void(std::error_code, u8 ver)> callback);

private:
	using buffer_iter = buffers_iterator<const_buffers_1>;

	/**
	 * @brief Common function for sending packets
	 * @param type      Packet type
	 * @param value     Payload
	 * @param callback  Function to call with response
	 */
	void send(u8 type, u8 value, std::function<void(std::error_code, u8 cm)> callback = {});

	/**
	 * @brief Initiate packet send
	 */
	void send_start();
	/**
	 * @brief Handler for sent packets
	 * @param ec   Possible i/o error
	 * @param len  Size of sent data
	 */
	void send_handle(std::error_code ec, usz len);

	/**
	 * @brief Initiate wait for incoming packets
	 */
	void recv_start();
	/**
	 * @brief Handle incoming data
	 * @param ec   Possible i/o error
	 * @param len  Size of received data
	 */
	void recv_handle(std::error_code ec, usz len);

	/**
	 * @brief Start timeout timer
	 */
	void timeout_start();
	/**
	 * @brief Handle timeout
	 */
	void timeout_handle();

	/**
	 * @brief Handle confirmed packets
	 * @param type   Packet type
	 * @param value  Payload
	 */
	void on_packet(u8 type, u8 value);
	/**
	 * @brief Timed callback to feed speed values to Driver and delay watchdog
	 * @param ec   Possible i/o error
	 */
	void wd_feed(std::error_code ec);

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
