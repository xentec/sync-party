#include "driver.hpp"

#include "util.hpp"

#include "proto-def.hpp"


constexpr auto TIMEOUT_TIME = std::chrono::milliseconds(100);
constexpr auto TIMEOUT_RETRIES = 10;

using namespace proto;

namespace serial_opts
{

struct hang_up
{
	explicit hang_up(bool val): value_(val) {}

	unsigned int value() const { return value_; };

	inline void store(termios& storage, boost::system::error_code&) const
	{
		if(value_)
			storage.c_cflag |= HUPCL;
		else
			storage.c_cflag &= ~HUPCL;
	}

	inline void load(const termios& storage, boost::system::error_code&)
	{
		value_ = storage.c_cflag & HUPCL;
	}

private:
	bool value_;
};

}

constexpr auto limit_factor = 4.0 / 10.0;

const def::Scale Driver::limit
{
	i32((Speed::BACK_FULL - Speed::STOP) * limit_factor),
	i32((Speed::FORWARD_FULL - Speed::STOP) * limit_factor)
};


Driver::Driver(boost::asio::io_context& ctx, const char* dev_path)
    : logger(new_loggr("driver"))
    , dev(ctx, dev_path)
    , timer(ctx)
    , timeout_num(0)
    , parse_state(SYNC)
    , speed_ctrl{ steady_timer(ctx), Speed::STOP }
{
	dev.set_option(serial_port::baud_rate(BAUD /*115200*/));
	dev.set_option(serial_opts::hang_up(false));
	recv_start();

	version([this](auto ec, u8 v)
	{
		if(ec)
		{
			logger->error("failed to fetch version: {}", ec.message());
			dev.close();
		}
		else
			logger->debug("firmware version {}", v);
	});
}

void Driver::drive(i32 speed)
{
	speed_ctrl.curr = Speed::STOP + clamp(speed, limit.min, limit.max);

	if(speed_ctrl.curr == Speed::STOP)
		speed_ctrl.feeder.cancel();
	else
		wd_feed({});
}

void Driver::gap(u8 pin, std::function<void(std::error_code, u8)> callback)
{
	send(Type::ULTRA_SONIC, pin, callback);
}

void Driver::analog(u8 pin, std::function<void (std::error_code, u8)> callback)
{
	send(Type::ANALOG, pin, callback);
}

void Driver::version(std::function<void (std::error_code, u8)> callback)
{
	send(Type::VERSION, 0, callback);
}

void Driver::wd_feed(std::error_code ec)
{
	send(Type::MOTOR, speed_ctrl.curr);

	if(ec) return;
	speed_ctrl.feeder.expires_after(TIMEOUT_TIME);
	speed_ctrl.feeder.async_wait([this](auto ec){ wd_feed(ec); });
}

void Driver::send(u8 type, u8 value, std::function<void(std::error_code, u8 cm)> callback)
{
	if(q.size() > 16 && !(type == Type::MOTOR && value == Speed::STOP))
	{
		logger->warn("dropping {}:{}", u8(type), value);
		return;
	}

	logger->trace("TX {:02X} {:3}", u8(type), value);

	std::ostream out(&buf_w);
	out << '[' << char(type) << char(value) << ']';
	out.flush();

	bool first = q.empty();
	q.push_back({u8(type), callback});

	if(first)
		send_start();
	else
		logger->trace("Q: {}", q.size());
}

void Driver::send_start()
{
//	logger->trace("SEND START: {:02x}", fmt::join(buffers_begin(buf_w.data()), buffers_end(buf_w.data()), " "));
	dev.async_write_some(buf_w.data(), [this](auto ec, usz len){ send_handle(ec, len); });
}

void Driver::send_handle(std::error_code ec, usz len)
{
//	logger->trace("SEND END ({})", len);
	if(ec && !q.empty())
	{
		Req &r = q.front();
		r.cb(ec, {});
		q.pop_front();
		return;
	}

	timeout_start();
}

void Driver::timeout_start()
{
	timer.expires_from_now(TIMEOUT_TIME);
	timer.async_wait([this](auto ec)
	{
		if(ec) return;

		timeout_handle();
	});
}

void Driver::timeout_handle()
{
	timeout_num += 1;
	if(timeout_num < TIMEOUT_RETRIES)
	{
		logger->trace("retry ({})", timeout_num);
		send_start();
	} else if(!q.empty())
	{
		q.front().cb(std::make_error_code(std::errc::timed_out), {});
		q.pop_front();
	}
}

void Driver::recv_start()
{
//	logger->trace("RECV START");
	dev.async_read_some(buf_r.prepare(32), [this](auto ec, usz len) { recv_handle(ec, len); });
}

void Driver::recv_handle(std::error_code ec, usz len)
{
	if(ec)
	{
		if(ec.value() != boost::system::errc::operation_canceled)
			logger->error("failed to read from driver: {}", ec.message());
		return;
	}

	buf_r.commit(len);
//	logger->trace("RECV END: {:02x}", fmt::join(buffers_begin(buf_r.data()), buffers_end(buf_r.data()), " "));


	bool stop = false;
	do {
		buffer_iter pkt_begin(buffers_begin(buf_r.data())),
		            pkt_end(buffers_end(buf_r.data()));

		switch(parse_state)
		{
		case SYNC:
		{
			buffer_iter sync = std::find(pkt_begin, pkt_end, BYTE_SYNC);
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
		{
			if(buf_r.size() < PKT_SIZE-1)
			{
				stop = true; break;
			}

			auto tail = std::next(pkt_begin, PKT_SIZE-2);
			if(*tail == BYTE_END)
			{
				pkt_end = std::next(tail);
				on_packet(pkt_begin[0], pkt_begin[1]);
				buf_r.consume(usz(std::distance(pkt_begin, pkt_end)));
				buf_w.consume(PKT_SIZE);
			}
		}
		default:
			parse_state = SYNC;
		}
	} while(!stop);

	if(q.size())
		timeout_start();

	recv_start();
}

void Driver::on_packet(u8 type, u8 value)
{
	timer.cancel();
	timeout_num = 0;

	bool err = (type & ERR_BIT) > 0;
	type &= ~ERR_BIT;

	logger->trace("RX 0x{:02x} {:3x}", type, value);

	if(type >= u8(Type::_MAX)) return;

	while(!q.empty())
	{
		Req &r = q.front();
		bool found = r.type == type;
		if(found && r.cb)
		{
			if(!err)
				r.cb({}, value);
			else
				r.cb(std::make_error_code(std::errc::protocol_error), {});
		}

		q.pop_front();
		if(!found)
			logger->warn("tx/rx async");
		else
			break;
	}
}
