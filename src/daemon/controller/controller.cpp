#include "controller.hpp"

#include <linux/input.h>
#include <linux/joystick.h>


Controller::Controller(io_context& ctx, Type type, const std::string &dev_path)
	: logger(new_loggr("ctrl"))
	, type(type)
	, sd(ctx)
	, dev_path(dev_path)
	, timer_recover(ctx)
{
	switch(type)
	{
	case Type::Joystick: recv_handler = [this](auto e, auto l) { recv_handle_js(e,l); }; break;
	case Type::Keyboard: recv_handler = [this](auto e, auto l) { recv_handle_kb(e,l); }; break;
	default: break;
	}

	auto ec = dev_open();
	if(ec)
	{
		logger->error("failed to open {}: {}", dev_path, ec.message());
		logger->info("trying to recover...");
	}
}

std::error_code Controller::dev_open()
{
	int fd = open(dev_path.c_str(), O_RDONLY | O_NONBLOCK);
	if(0> fd)
	{
		auto ec = std::error_code(errno, std::system_category());
		logger->debug("failed to open {}: {}", dev_path, ec.message());

		timer_recover.expires_after(std::chrono::seconds(1));
		timer_recover.async_wait([this](auto ec) { if(!ec) dev_open(); });
		return ec;
	}

	sd.assign(fd);
	recv_start();

	return {};
}

Controller::Type Controller::get_type() const
{
	return type;
}

void Controller::recv_start()
{
	sd.async_read_some(buf.prepare(64), [this](auto ec, auto len) { recv_handle(ec, len); });
}

void Controller::recv_handle(std::error_code ec, usz len)
{
	if(ec)
	{
		if(ec == std::errc::operation_canceled) return;

		logger->error("failed to read: {}", ec.message());
		if(on_err) on_err(ec);
		if(ec == std::errc::no_such_device)
		{
			sd.close();
			dev_open();
		}

		return;
	}

	recv_handler(ec, len);
}

void Controller::recv_handle_js(std::error_code, usz len)
{
	constexpr usz pkt_size = sizeof(js_event);

	buf.commit(len);
	while(buf.size() >= pkt_size)
	{
		auto &ev = *static_cast<const js_event*>(buf.data().data());
		if(on_axis && (ev.type & JS_EVENT_AXIS))
		{
//			logger->trace("on_axis: {:4} - {} {:6}", ev.time % 1000, ev.number, ev.value);
			on_axis(ev.time, Axis(ev.number), ev.value);
		}

		buf.consume(pkt_size);
	}
	recv_start();
}

void Controller::recv_handle_kb(std::error_code, usz len)
{
	constexpr usz pkt_size = sizeof(input_event);

	buf.commit(len);
	while(buf.size() >= pkt_size)
	{
		auto &ev = *static_cast<const input_event*>(buf.data().data());
		if(on_key && (ev.type == EV_KEY))
		{
			u32 time = ev.time.tv_sec * 1000 + ev.time.tv_usec / 1000;
//			logger->trace("on_key: {:4} - {:3} {}", time % 1000, ev.code, ev.value);
			on_key(time, ev.code, KeyState(ev.value));
		}

		buf.consume(pkt_size);
	}
	recv_start();
}
