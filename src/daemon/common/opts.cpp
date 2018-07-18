/* Copyright (c) 2018 LIV-T GmbH */
#include "opts.hpp"

#include "logger.hpp"
#include <unistd.h>

void CommonOpts::parse(argh::parser &opts, bool use_hostname)
{
	auto log_lvl = slog::level::info;
	if (opts["-v"]) log_lvl = slog::level::debug;
	if (opts["-vv"]) log_lvl = slog::level::trace;
	slog::set_level(log_lvl);

	if(use_hostname)
	{
		std::vector<char> n(64, 0);
		if(0> gethostname(n.data(), n.size()))
		{
			if(name.empty())
				name = name + "-err";
		}
		else
			name.assign(n.begin(), n.begin()+strlen(n.data()));
	}

	opts({"-i", "--id"}, name) >> name;
	opts({"-h", "--host"}, host) >> host;
	opts({"-p", "--port"}, port) >> port;
	echo_broadcast = opts["--echo"];
}
