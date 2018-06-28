/* Copyright (c) 2018 LIV-T GmbH */
#pragma once

#include <argh.h>

struct CommonOpts {
	std::string name, host, port;
	bool echo_broadcast;
};

void parse_common_opts(argh::parser& p, CommonOpts& opts, bool use_hostname = true);
