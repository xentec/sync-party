/* Copyright (c) 2018 LIV-T GmbH */
#pragma once

#include <argh.h>
#include "def.hpp"

struct CommonOpts
{
	std::string name, host = def::HOST, port = def::PORT;
	bool echo_broadcast = false;

	void parse(argh::parser& p, bool use_hostname = true);
};


