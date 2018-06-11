#include "driver.hpp"

Driver::Driver(boost::asio::io_context& ioctx, const char* dev_path)
	: logger(slog::stdout_color_st("driver"))
	, dev(ioctx, dev_path)
	, parse_state(SYNC)
	, speed_ctrl{ steady_timer(ioctx), Speed::STOP}
{
	dev.set_option(boost::asio::serial_port::baud_rate(115200));
	tcdrain(dev.native_handle());
	recv_start();
}

void Driver::drive(u8 speed)
{
	speed_ctrl.curr = speed;

	if(speed == Speed::STOP)
		speed_ctrl.feeder.cancel();
	else
		wd_feed({});
}

void Driver::gap(u8 sensor, std::function<void(error_code, u8)> callback)
{
	send(GAP, sensor, callback);
}

void Driver::wd_feed(error_code err)
{
	send(MOTOR, speed_ctrl.curr);

	if(err) return;
	speed_ctrl.feeder.expires_after(std::chrono::milliseconds(100));
	speed_ctrl.feeder.async_wait([this](auto ec){ wd_feed(ec); });
}

void Driver::send(Type type, u8 value, std::function<void(error_code, u8 cm)> cb)
{
//	logger->trace("TX {:02X} {:3}", type, value);

	std::ostream out(&buf_w);
	out << '[' << char(type) << char(value) << ']';
	out.flush();

	bool first = q.empty();
	q.push_back({type, cb});

	if(first)
		send_start();
}

void Driver::send_start()
{
//	logger->trace("SEND: {:02x}", fmt::join(buffers_begin(buf_w.data()), buffers_end(buf_w.data()), " "));
	dev.async_write_some(buf_w.data(), [this](auto ec, usz len){ send_handle(ec, len); });
}

void Driver::send_handle(error_code ec, usz len)
{
	buf_w.consume(len);

	if(ec && !q.empty())
	{
		Req &r = q.front();
		r.cb(ec, {});
		q.pop_front();
	}
}

void Driver::recv_start()
{
	dev.async_read_some(buf_r.prepare(32), [this](auto ec, usz len) { recv_handle(ec, len); });
}

void Driver::recv_handle(error_code ec, usz len)
{
	if(ec)
	{
		logger->error("failed to read from driver: {}", ec.message());
		return;
	}

	buf_r.commit(len);
//	logger->trace("RECV: {:02x}", fmt::join(buffers_begin(buf_r.data()), buffers_end(buf_r.data()), " "));

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
			sync++;
			buf_r.consume(usz(std::distance(pkt_begin, sync)));
			pkt_begin = sync;
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
				u8 type = u8(*pkt_begin);
				switch(type)
				{
				case PING:
				case MOTOR:
				case GAP:
				case ERR:
					logger->trace("RX 0x{:02x} {:3x}", type, pkt_begin[1]);
					on_packet(Type(type), pkt_begin[1]);
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
	while(!q.empty())
	{
		Req &r = q.front();
		bool found = r.type == type;
		if(found && r.cb)
			r.cb({}, value);

		q.pop_front();
		if(!found)
			logger->warn("tx/rx buffer async");
		else
			break;
	}
	if(q.size()) send_start();
}
