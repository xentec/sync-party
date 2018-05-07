#include "controller.hpp"

#include <linux/joystick.h>

Controller::Controller(io_context& ctx, const char* dev_path)
	: sd(ctx)
{
	int fd = open(dev_path, O_RDONLY | O_NONBLOCK);
	if(0> fd)
		throw std::system_error(-fd, std::system_category());

	sd.assign(fd);

	recv_start();
}

void Controller::recv_start()
{
	sd.async_read_some(buf.prepare(sizeof(js_event)), [this](auto ec, auto len) { recv_handle(ec, len); });
}

void Controller::recv_handle(boost::system::error_code ec, usz len)
{
	if(ec)
	{
//		fmt::print("failed to read from controller: {}\n", ec.message());
		return;
	}

	constexpr usz pkt_size = sizeof(js_event);

	buf.commit(len);
	while(buf.size() >= pkt_size)
	{
		auto &ev = *static_cast<const js_event*>(buf.data().data());
		if(on_axis && (ev.type & JS_EVENT_AXIS))
		{
			on_axis(ev.time, Axis(ev.number), ev.value);
		}

		buf.consume(pkt_size);
	}
	recv_start();
}
