// ==============================================================================
// Copyright 2020 The LatticeX Foundation
// This file is part of the Rosetta library.
//
// The Rosetta library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Rosetta library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with the Rosetta library. If not, see <http://www.gnu.org/licenses/>.
// ==============================================================================
#pragma once

/**
 * thread-safe log based on spdlog
 */
#include <sstream>
#include <string.h>
#include <string>
#include <atomic>
#include <map>
#include "spdlog/logger_stream.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <spdlog/async.h>

#if !defined(__FILENAME__)
#define __FILENAME__ (strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif

class logger;

class Logger {
	private:
		Logger();

	public:
		static Logger& Get();
		void release();
		virtual ~Logger() { 
			release(); 
		}

	public:
		int get_level(void) { 
			return level_; 
		}
		void log_to_stdout(bool flag = true) {
			to_stdout_ = flag; 
		}
		bool get_log_to_stdout(void) const {
			return to_stdout_;
		}
		void set_level(int level);
		void set_filename(const std::string& filename, const std::string& task_id="");
		void set_pattern(const std::string& pattern);
		std::string get_filename(void) const;
		std::shared_ptr<spdlog::logger> get_logger(const std::string& task_id);

	private:
		//static std::mutex mutex_;
		static pthread_rwlock_t rwlock_;
		std::atomic<bool> to_stdout_; // default is true
		std::atomic<bool> to_file_{false}; // false
		std::atomic<int> level_ ;
		std::string pattern_;
		std::map<std::string, std::string> filename_taskid_;
		std::map<std::string, std::shared_ptr<spdlog::logger>> taskid_logger_;
		std::string latest_filename_;
};

extern std::shared_ptr<spdlog::logger> g_default_logger;
extern Logger* g_logger;

#define SPDLOG_LOGGER_CALL_FUNCTION(logger, level, ...) (logger)->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, level, ##__VA_ARGS__)

#define TTRACE_(task_id, ...) \
	do {\
		if (Logger::Get().get_log_to_stdout()) { \
			SPDLOG_LOGGER_CALL_FUNCTION(spdlog::default_logger(), spdlog::level::trace, ##__VA_ARGS__); \
		} \
		std::shared_ptr<spdlog::logger> logger = Logger::Get().get_logger(task_id); \
		if (logger != nullptr && logger != spdlog::default_logger())  {\
			SPDLOG_LOGGER_CALL_FUNCTION(Logger::Get().get_logger(task_id), spdlog::level::trace, ##__VA_ARGS__); \
		} \
	} while(0)
#define TDEB_(task_id, ...) \
	do {\
		if (Logger::Get().get_log_to_stdout()) { \
			SPDLOG_LOGGER_CALL_FUNCTION(spdlog::default_logger(), spdlog::level::debug, ##__VA_ARGS__); \
		} \
		std::shared_ptr<spdlog::logger> logger = Logger::Get().get_logger(task_id); \
		if (logger != nullptr && logger != spdlog::default_logger())  {\
			SPDLOG_LOGGER_CALL_FUNCTION(Logger::Get().get_logger(task_id), spdlog::level::debug, ##__VA_ARGS__); \
		} \
	} while(0)
#define TAUDIT_(task_id, ...) \
	do {\
		if (Logger::Get().get_log_to_stdout()) { \
			SPDLOG_LOGGER_CALL_FUNCTION(spdlog::default_logger(), spdlog::level::audit, ##__VA_ARGS__); \
		} \
		std::shared_ptr<spdlog::logger> logger = Logger::Get().get_logger(task_id); \
		if (logger != nullptr && logger != spdlog::default_logger())  {\
			SPDLOG_LOGGER_CALL_FUNCTION(Logger::Get().get_logger(task_id), spdlog::level::audit, ##__VA_ARGS__); \
		} \
	} while(0)
#define TINFO_(task_id, ...) \
	do {\
		if (Logger::Get().get_log_to_stdout()) { \
			SPDLOG_LOGGER_CALL_FUNCTION(spdlog::default_logger(), spdlog::level::info, ##__VA_ARGS__); \
		} \
		std::shared_ptr<spdlog::logger> logger = Logger::Get().get_logger(task_id); \
		if (logger != nullptr && logger != spdlog::default_logger())  {\
			SPDLOG_LOGGER_CALL_FUNCTION(Logger::Get().get_logger(task_id), spdlog::level::info, ##__VA_ARGS__); \
		} \
	} while(0)
#define TWARN_(task_id, ...) \
	do {\
		if (Logger::Get().get_log_to_stdout()) { \
			SPDLOG_LOGGER_CALL_FUNCTION(spdlog::default_logger(), spdlog::level::warn, ##__VA_ARGS__); \
		} \
		std::shared_ptr<spdlog::logger> logger = Logger::Get().get_logger(task_id); \
		if (logger != nullptr && logger != spdlog::default_logger())  {\
			SPDLOG_LOGGER_CALL_FUNCTION(Logger::Get().get_logger(task_id), spdlog::level::warn, ##__VA_ARGS__); \
		} \
	} while(0)
#define TERROR_(task_id, ...) \
	do {\
		if (Logger::Get().get_log_to_stdout()) { \
			SPDLOG_LOGGER_CALL_FUNCTION(spdlog::default_logger(), spdlog::level::err, ##__VA_ARGS__); \
		} \
		std::shared_ptr<spdlog::logger> logger = Logger::Get().get_logger(task_id); \
		if (logger != nullptr && logger != spdlog::default_logger())  {\
			SPDLOG_LOGGER_CALL_FUNCTION(Logger::Get().get_logger(task_id), spdlog::level::err, ##__VA_ARGS__); \
		} \
	} while(0)
#define TFATAL_(task_id, ...) \
	do {\
		if (Logger::Get().get_log_to_stdout()) { \
			SPDLOG_LOGGER_CALL_FUNCTION(spdlog::default_logger(), spdlog::level::critical, ##__VA_ARGS__); \
		} \
		std::shared_ptr<spdlog::logger> logger = Logger::Get().get_logger(task_id); \
		if (logger != nullptr && logger != spdlog::default_logger())  {\
			SPDLOG_LOGGER_CALL_FUNCTION(Logger::Get().get_logger(task_id), spdlog::level::critical, ##__VA_ARGS__); \
		} \
	} while(0)

#include"loggers.hpp"
