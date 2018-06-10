#define BOOST_ASIO_NO_DEPRECATED 1

#include "driver.hpp"


Driver::Driver(boost::asio::io_context& ioctx, const char* dev_path)
	: logger(slog::stdout_color_st("driver"))
	, dev(ioctx, dev_path)
	, wd_feeder(ioctx)
	, parse_state(SYNC)
{
	dev.set_option(boost::asio::serial_port::baud_rate(115200));
	recv_start();

	wd_feed({});
}

void Driver::drive(u8 speed)
{
	if(buf_w.size()) return;

	send(MOTOR, speed);
}

void Driver::gap(u8 sensor, std::function<void(error_code, u8)> callback)
{
	send(GAP, sensor, callback);
}

void Driver::wd_feed(error_code err)
{
	if(err) return;

	send(PING, 0);
	wd_feeder.expires_after(std::chrono::milliseconds(50));
	wd_feeder.async_wait([this](auto ec){ wd_feed(ec); });
}

void Driver::send(Type type, u8 value, std::function<void(error_code, u8 cm)> cb)
{
	logger->trace("TX {:02X} {:3}", type, value);

	std::ostream out(&buf_w);
	char data[] = { char(type), char(value) };

	out << '[' << data << ']';
	out.flush();

	q_w.push_back({type, cb});

	send_start();
}

void Driver::send_start()
{
	dev.async_write_some(buf_w.data(), [this](auto ec, usz len){ send_handle(ec, len); });
}

void Driver::send_handle(error_code ec, usz len)
{
	Req *r = nullptr;
	if(!q_w.empty())
		r = &q_w.front();

	buf_w.consume(len);

	if(ec)
	{
		r->cb(ec, {});
	} else if(r)
	{
		q_r.push_back(q_w.front());
		q_w.pop_front();
	}

	if(buf_w.size()) send_start();
}

void Driver::recv_start()
{
	dev.async_read_some(buf_r.prepare(32), [this](auto ec, usz len) { recv_handle(ec, len); });
}

void Driver::recv_handle(boost::system::error_code ec, usz len)
{
	if(ec)
	{
		logger->error("failed to read from driver: {}", ec.message());
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
		{
			buffer_iter sync = std::find(pkt_begin, pkt_end, SYNC_BYTE::BEGIN);
			if(sync == pkt_end)
			{
				stop = true; break;
			}

			buf_r.consume(usz(std::distance(pkt_begin, sync)));
			pkt_begin = sync+1;
			parse_state = DATA;
		}
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
					logger->trace("RX {:02X} {:3}");
					on_packet(Type(*pkt_begin), *++pkt_begin);
				default: break;
				}

				buf_r.consume(usz(std::distance(pkt_begin, pkt_end)));
			}
		default:
			parse_state = SYNC;
		}
	} while(!stop);
	recv_start();
}

void Driver::on_packet(Type type, u8 value)
{
	while(!q_r.empty())
	{
		Req &r = q_r.front();
		bool found = r.type == type;
		if(found && r.cb)
			r.cb({}, value);

		q_r.pop_front();
		if(!found)
			logger->warn("tx/rx buffer async");
		else
			break;
	}
}
