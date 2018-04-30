
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

		buf.commit(len);
		auto &ev = *buffer_cast<const js_event*>(buf.data());

		//fmt::print("time: {}, type: {}, number: {}, value: {}\n", ev.time, ev.type, ev.number, ev.value);

		if(on_axis && (ev.type & JS_EVENT_AXIS))
		{
			on_axis(ev.time, ev.number, ev.value);
		}


		buf.consume(len);
		recv_start();
	}

	posix::stream_descriptor sd;
	streambuf buf;

public:
	std::function<void(u32 time, u8 num, i16 val)> on_axis;
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
		std::ostream out(&buf_w);
		char data[2] = { MOTOR, char(speed) };

		out << '[' << data << ']';
		dev.async_write_some(buf_w.data(), [this](auto ec, usz len)
		{
			if(ec) return;
			buf_w.consume(len);
		});
	}

private:
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
/*
		constexpr usz pktSize = 4;
		buf_r.commit(len);

		const_buffers_1::const_iterator begin;

		bool stop = false;
		do {
			auto pkt(buf_r.data());

			switch(parseState)
			{
			case SYNC:
				begin = std::find(pkt.begin(), pkt.end(), SYNC_BYTE::BEGIN);
				if(pkt.empty())
				{
					stop = true; break;
				}

				pkt.advance(); // drop sync byte
				buf_r.consume(1);
				parseState = DATA;
			case DATA:
				if(pkt.size() < pktSize+1)
				{
					stop = true; break;
				}
	//			if(auto tail = std::next(pkt.begin(), pktSize); *tail == 0xAB) // [SYNC [pkt] SYNC+pktSize] // TODO: C++17
				{
					auto tail = std::next(pkt.begin(), pktSize);
					if(*tail == 0xAB)
					{
						pkt.end() = std::next(tail);
						if(!valid(pkt))
						{
							logger->debug("RECV: crc fail: {:02X}", fmt::join(pkt.begin(), pkt.end(), " "));
						}
						else
						{
							DBG(logger, "RECV: {:02X}", fmt::join(pkt.begin(), pkt.end(), " "));
							on_packet(pkt);
							bufs.r.consume(pkt.size());
						}
					}
				}
			default:
				parseState = SYNC;
			}
		} while(!stop);

*/

		buf_r.commit(len);
//		auto &ev = *buffer_cast<const input_event*>(buf.data());
		fmt::print("RECV: {}\n", len);
		buf_r.consume(len);
		recv_start();
	}

	serial_port dev;
	streambuf buf_r, buf_w;

	enum ParseState
	{
		SYNC, DATA
	} parseState = SYNC;
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
