/* Copyright (c) 2018 LIV-T GmbH */
#pragma once

#include <argh.h>
#include "def.hpp"

/**
 * @brief Contrainer for common command line options
 * @author Andrej Utz
 */
struct CommonOpts
{
	std::string name, host = def::HOST, port = def::PORT;
	bool echo_broadcast = false;

	/**
	 * @brief Update options
	 * @param p             argh parser
	 * @param use_hostname  Use system hostname if no command line parameter was given
	 */
	void parse(argh::parser& p, bool use_hostname = true);
};


