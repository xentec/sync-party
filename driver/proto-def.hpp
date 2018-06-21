#pragma once

namespace proto
{

enum
{
	BYTE_SYNC = '[',
	BYTE_END  = ']',
};

enum class Type
{
	PING          = 0x0,
	VERSION       = 0x1,
	MOTOR         = 0x2,
	ULTRA_SONIC   = 0x3,
	ANALOG        = 0x4,

	_MAX
};


const unsigned char ERR_BIT = (1 << 7);
enum Error
{
	INVALID_ARG = 0x0,
};


enum Speed
{
//	BACK_FULL    = 0x10,
	BACK_FULL    = 0x20,
	STOP         = 0x30,
	FORWARD_FULL = 0x60,
//	FORWARD_FULL = 0x90,
};

}
