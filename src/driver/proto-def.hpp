#pragma once

#include <stdint.h>
#include <stddef.h>

namespace proto
{

constexpr auto BAUD = 1000000;

enum Sync : uint8_t
{
	BYTE_SYNC = '[',
	BYTE_END  = ']',
};

enum Type : uint8_t
{
	PING          = 0x0,
	VERSION       = 0x1,
	MOTOR         = 0x2,
	ULTRA_SONIC   = 0x3,
	ANALOG        = 0x4,

	_MAX
};

struct Packet
{
	Sync begin;
	Type type;
	uint8_t data;
	Sync end;
};

constexpr size_t PKT_SIZE = sizeof (Packet);

const uint8_t ERR_BIT = (1 << 7);
enum Error
{
	INVALID_ARG = 0x0,
};


enum Speed : uint8_t
{
	BACK_FULL    = 0x10,
	STOP         = 0x30,
	FORWARD_FULL = 0x90,
};

}
