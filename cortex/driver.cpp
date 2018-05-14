#include "driver.hpp"


Driver::Driver(boost::asio::io_context& ioctx, const char* dev_path)
	: logger(slog::stdout_color_st("driver"))
	, dev(ioctx, dev_path)
	, wd_feed(ioctx)
	, parse_state(SYNC)
{
	dev.set_option(boost::asio::serial_port::baud_rate(115200));
	recv_start();

	wd_feed.expires_after(std::chrono::milliseconds(50));
	wd_feed.async_wait([this](auto err)
	{
		if(err) return;

		send(PING, 0);
		wd_feed.expires_after(std::chrono::milliseconds(50));
	});
}

void Driver::drive(u8 speed)
{
	send(MOTOR, speed);
}

void Driver::send(Driver::Type type, u8 value)
{
	logger->trace("S {} {}", type, value);

	std::ostream out(&buf_w);
	char data[] = { char(type), char(value) };

	out << '[' << data << ']';
	dev.async_write_some(buf_w.data(), [this](auto ec, usz len)
	{
		if(ec) return;
		buf_w.consume(len);
	});
}

void Driver::recv_start()
{
	dev.async_read_some(buf_r.prepare(16), [this](auto ec, auto len) { recv_handle(ec, len); });
}

void Driver::recv_handle(boost::system::error_code ec, usz len)
{
	if(ec)
	{
		logger->error("failed to read from driver: {}\n", ec.message());
		return;
	}

	buf_r.commit(len);

	constexpr usz pkt_size = 4;

	bool stop = false;
	do {
		buffer_iter pkt_begin(buffers_begin(buf_r.data())),
				pkt_end(buffers_end(buf_r.data()));

		switch(parse_state)
		{
		case SYNC:
			pkt_begin = std::find(pkt_begin, pkt_end, SYNC_BYTE::BEGIN);
			if(pkt_begin == pkt_end)
			{
				stop = true; break;
			}

			buf_r.consume(usz(std::distance(pkt_begin, pkt_end)));
			pkt_begin++;
			parse_state = DATA;
		case DATA:
			if(buf_r.size() < pkt_size-1)
			{
				stop = true; break;
			}
			if(auto tail = std::next(pkt_begin, pkt_size-2); *tail == SYNC_BYTE::END)
			{
				pkt_end = std::next(tail);
				switch(u8(*pkt_begin))
				{
				case PING:
				case MOTOR:
				case GAP:
				case ERR:
					on_packet(Type(*pkt_begin), *++pkt_begin);
				default: break;
				}

				buf_r.consume(usz(std::distance(pkt_begin, pkt_end)));
			}
		default:
			parse_state = SYNC;
		}
	} while(!stop);

	buf_r.consume(len);
	recv_start();
}

void Driver::on_packet(Driver::Type type, u8 value)
{

}
