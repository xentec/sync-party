/* Copyright (c) 2018 LIV-T GmbH */
#include "opts.hpp"

#include "def.hpp"
#include "logger.hpp"

void parse_common_opts(argh::parser &opts, CommonOpts &conf, bool use_hostname)
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
			if(conf.name.empty())
				conf.name = conf.name + "-err";
		}
		else
			conf.name.assign(n.begin(), n.begin()+strlen(n.data()));
	}

	opts({"-i", "--id"}, conf.name) >> conf.name;
	opts({"-h", "--host"}, def::HOST) >> conf.host;
	opts({"-p", "--port"}, def::PORT) >> conf.port;
	conf.echo_broadcast = opts["--echo"];
}
