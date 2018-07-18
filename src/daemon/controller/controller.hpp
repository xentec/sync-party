#pragma once

#include "asio.hpp"
#include "logger.hpp"
#include "types.hpp"

#include <boost/asio/streambuf.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

/**
 * @brief Listens for input events from an input device and calls apropriate callbacks
 * @author Andrej Utz
 */
struct Controller
{
	/**
	 * @brief Types of supported input devices
	 */
	enum Type
	{
		Joystick, ///< Joystick type with buttons and axis, e.g. a gamepad
		Keyboard, ///< Keyboard type with keys only e.g. a regular keyboard
	};

	/**
	 * @brief The kind of input axis from a gamepad
	 */
	enum Axis
	{
		LS_H = 0, ///< Left stick horizontal position
		LS_V = 1, ///< Left stick vertical position
		LT2 = 2,  ///< Left lower trigger
		RT2 = 5,  ///< Right lower trigger
	};

	/**
	 * @brief The kind if state a button (e.g from a keyboard) can have
	 */
	enum KeyState
	{
		Pressed, Released, Repeating
	};

	/**
	 * @brief Mininmal possible axis state (-2^15 + 1)
	 */
	static constexpr i16 axis_min = std::numeric_limits<i16>::min() + 1;
	/**
	 * @brief Maximal possible axis state (2^15)
	 */
	static constexpr i16 axis_max = std::numeric_limits<i16>::max();

	/**
	 * @brief Constructor and initializer
	 *
	 * Should an error happen while opening the device then controller will try to open it again
	 * every second.
	 * @param ctx        Managing io_context from Asio
	 * @param type       Type of input device
	 * @param dev_path   Device file to listen on (e.g. /dev/input/js0 for a gamepad)
	 */
	Controller(io_context& ctx, Type type, const std::string& dev_path);

	/**
	 * @brief Return the input type the instance uses
	 * @return the device type
	 */
	Type get_type() const;

	/**
	 * @brief Callback for axis change events
	 */
	std::function<void(u32 time, Axis num, i16 val)> on_axis;
	/**
	 * @brief Callback for button/key presses
	 */
	std::function<void(u32 time, u32 keycode, KeyState val)> on_key;
	/**
	 * @brief Callback for error handling (e.g. a device disconnect)
	 */
	std::function<void(std::error_code ec)> on_err;

private:
	/**
	 * @brief Initiate wait for an input event
	 */
	void recv_start();
	/**
	 * @brief Handler for input events
	 * @param ec   Possible i/o error
	 * @param len  Combined size of received events
	 */
	void recv_handle(std::error_code ec, usz len);
	/**
	 * @brief Handler for joystick type events
	 * @param ec   Possible i/o error
	 * @param len  Combined size of received events
	 */
	void recv_handle_js(std::error_code ec, usz len);
	/**
	 * @brief Handler for keyboard type events
	 * @param ec   Possible i/o error
	 * @param len  Combined size of received events
	 */
	void recv_handle_kb(std::error_code ec, usz len);

	/**
	 * @brief This callback is assigned to a specific input type handler at instance creation.
	 */
	std::function<void(std::error_code ec, usz len)> recv_handler;

	loggr logger;
	Type type;
	/**
	 * @brief Asio abstraction for a file descriptor
	 */
	posix::stream_descriptor sd;
	/**
	 * @brief Buffer for input events
	 */
	streambuf buf;

	std::string dev_path;
	steady_timer timer_recover;
	/**
	 * @brief Open device file
	 * @return possible i/o error
	 */
	std::error_code dev_open();
};

