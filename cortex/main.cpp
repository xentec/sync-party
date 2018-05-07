
#include "types.hpp"

#include <boost/asio.hpp>
#include <fmt/format.h>

#include <linux/joystick.h>

using namespace boost::asio;
using boost::system::error_code;

template<class I, class O>
O map(I x, I in_min, I in_max, O out_min, O out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

struct Controller
{
	enum Input
	{
		LS_H = 0,
		LS_V = 1,
		RT2 = 5,
	};

	Controller(io_context& ctx, const char* dev_path)
		: sd(ctx)
	{
		int fd = open(dev_path, O_RDONLY | O_NONBLOCK);
		if(0> fd)
			throw std::system_error(-fd, std::system_category());

		sd.assign(fd);

		recv_start();
	}

	std::function<void(u32 time, u8 num, i16 val)> on_axis;

private:
	void recv_start()
	{
		sd.async_read_some(buf.prepare(sizeof(js_event)), [this](auto ec, auto len) { recv_handle(ec, len); });
	}

	void recv_handle(error_code ec, usz len)
	{
		if(ec)
		{
			fmt::print("failed to read from controller: {}\n", ec.message());
			return;
		}

		constexpr usz pkt_size = sizeof(js_event);

		buf.commit(len);
		while(buf.size() >= pkt_size)
		{
			auto &ev = *static_cast<const js_event*>(buf.data().data());
			if(on_axis && (ev.type & JS_EVENT_AXIS))
			{
				on_axis(ev.time, ev.number, ev.value);
			}

			buf.consume(pkt_size);
		}
		recv_start();
	}

	posix::stream_descriptor sd;
	streambuf buf;
};

struct Driver
{
	enum Type
	{
	  PING  = 0x0,
	  MOTOR = 0x1,
	  GAP   = 0x2,

	  ERR = 0xFF
	};

	Driver(io_context& ioctx, const char* dev_path)
		: dev(ioctx, dev_path)
	{
		dev.set_option(serial_port::baud_rate(115200));
		recv_start();
	}

	void drive(u8 speed)
	{
		send(MOTOR, speed);
	}

private:
	using buffer_iter = buffers_iterator<const_buffer>;

	void send(Type type, u8 value)
	{
		std::ostream out(&buf_w);
		char data[] = { char(type), char(value) };

		out << '[' << data << ']';
		dev.async_write_some(buf_w.data(), [this](auto ec, usz len)
		{
			if(ec) return;
			buf_w.consume(len);
		});
	}

	void recv_start()
	{
		dev.async_read_some(buf_r.prepare(16), [this](auto ec, auto len) { recv_handle(ec, len); });
	}

	void recv_handle(error_code ec, usz len)
	{
		if(ec)
		{
			fmt::print("failed to read from driver: {}\n", ec.message());
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
				if(buf_r.size() < pkt_size)
				{
					stop = true; break;
				}
				if(auto tail = std::next(pkt_begin, pkt_size); *tail == SYNC_BYTE::END)
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

	void on_packet(Type type, u8 value)
	{

	}

	serial_port dev;
	streambuf buf_r, buf_w;

	enum ParseState
	{
		SYNC, DATA
	} parse_state = SYNC;
	enum SYNC_BYTE { BEGIN = '[', END = ']' };
};

template<class T>
using lim = std::numeric_limits<T>;

int main()
{
	io_context ioctx;

	Controller ctrl(ioctx, "/dev/input/js0");
	Driver driver(ioctx, "/dev/ttyACM0");

	ctrl.on_axis = [&](u32 time, u8 num, i16 val)
	{
		switch(num)
		{
		case Controller::RT2:
		{
			u8 d = map(u16(lim<i16>::max() + val), u16(0), lim<u16>::max(), u8(0x30), u8(0x90));
			fmt::print("{:010} MOTOR: {:02x}\n", time, d);
			driver.drive(d);
		}
		default:
			return;
		}
	};

	ioctx.run();

	return 0;
}
