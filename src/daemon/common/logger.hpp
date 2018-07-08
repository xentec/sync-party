#pragma once

#define SPDLOG_TRACE_ON
#define SPDLOG_DEBUG_ON
//#define SPDLOG_ENABLE_SYSLOG
#define SPDLOG_FINAL final
#define SPDLOG_NO_THREAD_ID
#ifdef __linux__
    #define SPDLOG_CLOCK_COARSE
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace slog = spdlog;

using loggr = std::shared_ptr<slog::logger>;

constexpr auto new_loggr = slog::stdout_color_st<slog::default_factory>;
