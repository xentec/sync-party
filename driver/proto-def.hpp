#pragma once

namespace proto
{

enum {
	BYTE_SYNC = '[',
	BYTE_END = ']',
};

enum Type
{
	PING  = 0x0,
	MOTOR,
	ULTRA_SONIC,
	ANALOG,

	VERSION = 0xFE,
	ERR = 0xFF
};

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
