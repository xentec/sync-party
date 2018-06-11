#include "controller.hpp"

#include <linux/input.h>
#include <linux/joystick.h>

Controller::Controller(io_context& ctx, Type type, const char* dev_path)
	: logger(slog::stdout_color_st("ctrl"))
	, type(type)
	, sd(ctx)
{
	int fd = open(dev_path, O_RDONLY | O_NONBLOCK);
	if(0> fd)
		throw std::system_error(errno, std::system_category(), "open");

	sd.assign(fd);

	switch(type)
	{
	case Type::Joystick: recv_handle = [this](auto e, auto l) { recv_handle_js(e,l); }; break;
	case Type::Keyboard: recv_handle = [this](auto e, auto l) { recv_handle_kb(e,l); }; break;
	default: break;
	}

	recv_start();
}

void Controller::recv_start()
{
	sd.async_read_some(buf.prepare(64), recv_handle);
}

void Controller::recv_handle_js(error_code ec, usz len)
{
	if(ec)
	{
		logger->error("failed to read from controller: {}", ec.message());
		return;
	}

	constexpr usz pkt_size = sizeof(js_event);

	buf.commit(len);
	while(buf.size() >= pkt_size)
	{
		auto &ev = *static_cast<const js_event*>(buf.data().data());
		if(on_axis && (ev.type & JS_EVENT_AXIS))
		{
			logger->trace("on_axis: {:4} - {} {:6}", ev.time % 1000, ev.number, ev.value);
			on_axis(ev.time, Axis(ev.number), ev.value);
		}

		buf.consume(pkt_size);
	}
	recv_start();
}

void Controller::recv_handle_kb(error_code ec, usz len)
{
	if(ec)
	{
		logger->error("failed to read from controller: {}", ec.message());
		return;
	}

	constexpr usz pkt_size = sizeof(input_event);

	buf.commit(len);
	while(buf.size() >= pkt_size)
	{
		auto &ev = *static_cast<const input_event*>(buf.data().data());
		if(on_key && (ev.type == EV_KEY))
		{
			u32 time = ev.time.tv_sec * 1000 + ev.time.tv_usec / 1000;
			logger->trace("on_key: {:4} - {:3} {}", time % 1000, ev.code, ev.value);
			on_key(time, ev.code, KeyState(ev.value));
		}

		buf.consume(pkt_size);
	}
	recv_start();
}
