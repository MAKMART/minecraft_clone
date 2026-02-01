#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string_view>
#include <mutex>
#include <chrono>
#include <ctime>
#include <thread>
#include <map>
#include <format>
#include <print>

class log
{
      public:
	enum class level { INFO = 0, WARNING = 1, ERROR = 2 };

      private:
	inline static std::mutex    log_mutex;
	inline static FILE *primary_file = stderr;
	inline static std::ostream* additional_stream = nullptr;
	///inline static std::ostream* primary_stream    = &std::cerr;
	inline static bool          enable_color      = true;
	inline static level      min_log_level     = level::INFO;
	static constexpr std::string_view reset_color = "\x1b[0m";

	[[nodiscard]] static std::string_view level_to_string(level level) noexcept
	{
		switch (level) {
		case level::INFO:
			return enable_color ? "\x1b[1;92mINFO" : "INFO";
		case level::WARNING:
			return enable_color ? "\x1b[1;93mWARN" : "WARN";
		case level::ERROR:
			return enable_color ? "\x1b[1;91mERROR" : "ERROR";
		default:
			return "UNKNOWN";
		}
	}

	[[nodiscard]] static std::string current_timestamp()
	{
		using namespace std::chrono;

		const auto now  = system_clock::now();
		const auto ms   = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
		const auto time = system_clock::to_time_t(now);

		std::tm tm{};
#ifdef _WIN32
		localtime_s(&tm, &time);
#else
		localtime_r(&time, &tm);
#endif

		return std::format(
				"{:02}-{:02}-{:04} {:02}:{:02}:{:02}:{:02}",
				tm.tm_mday,
				tm.tm_mon + 1,
				tm.tm_year + 1900,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec,
				ms.count()
				);
	}

	
	[[nodiscard]] inline static std::string thread_id_string()
	{
		return std::format("TID {}", std::this_thread::get_id());
	}

	template <typename... Args>
	static void write_log(level level, std::string_view system, std::format_string<Args...> fmt, Args&&... args)
	{
		if (level < min_log_level)
			return;

		std::scoped_lock lock(log_mutex);
		FILE* out = (level == level::INFO) ? stdout : stderr;

		const std::string RESET_COLOR = "\x1b[0m";
		std::string       message     = std::format(fmt, std::forward<Args>(args)...);
		
		const auto timestamp = current_timestamp();
		const auto tid       = thread_id_string();
		const auto level_str = level_to_string(level);

		std::print(
				out,
				"{} {} {} {}[{}] {}",
				timestamp,
				tid,
				level_str,
				enable_color ? reset_color : "",
				system,
				std::format(fmt, std::forward<Args>(args)...)
				);
		std::println(out);

		if (additional_stream) {
			*additional_stream << timestamp << " "
				<< tid << " ["
				<< system << "] "
				<< level_str << " "
				<< std::format(fmt, std::forward<Args>(args)...)
				<< (enable_color ? reset_color : "")
				<< '\n';
			additional_stream->flush();
		}

	}

      public:
	// configuration
	static void set_output_stream(FILE *file) noexcept
	{
		std::scoped_lock lock(log_mutex);
		primary_file = file;
	}

	static void set_dual_output(std::ostream& extra) noexcept
	{
		std::scoped_lock lock(log_mutex);
		additional_stream = &extra;
	}

	static void reset_output() noexcept
	{
		std::scoped_lock lock(log_mutex);
		primary_file = stderr;
		additional_stream = nullptr;
	}

	inline static void enable_colors(bool enable) noexcept { enable_color = enable; }
	inline static void set_min_log_level(level level) noexcept { min_log_level = level; }

	// Logging interface
	template <typename... Args>
	static void info(std::format_string<Args...> fmt, Args&&... args)
	{
		write_log(level::INFO, "GLOBAL", fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void warn(std::format_string<Args...> fmt, Args&&... args)
	{
		write_log(level::WARNING, "GLOBAL", fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void error(std::format_string<Args...> fmt, Args&&... args)
	{
		write_log(level::ERROR, "GLOBAL", fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void system_info(std::string_view system, std::format_string<Args...> fmt, Args&&... args)
	{
		write_log(level::INFO, system, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void system_warn(std::string_view system, std::format_string<Args...> fmt, Args&&... args)
	{
		write_log(level::WARNING, system, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void system_error(std::string_view system, std::format_string<Args...> fmt, Args&&... args)
	{
		write_log(level::ERROR, system, fmt, std::forward<Args>(args)...);
	}

	// Structured logging (just key=value for now)
	static void system_structured(std::string_view system, level level, std::string_view message, const std::map<std::string, std::string>& fields)
	{
		std::ostringstream msg;
		msg << message;
		for (auto it = fields.rbegin(); it != fields.rend(); ++it) {
			msg << " " << it->first << "=" << it->second;
		}
		write_log(level, system, "{}", msg.str());
	}

	static void structured(level level, std::string_view message, const std::map<std::string, std::string>& fields)
	{
		std::ostringstream msg;
		msg << message;
		for (auto it = fields.rbegin(); it != fields.rend(); ++it) {
			msg << " " << it->first << "=" << it->second;
		}
		write_log(level, "GLOBAL", "{}", msg.str());
	}

	// Scoped system logger
	class scoped_logger
	{
		std::string_view system_name;

	      public:
		explicit scoped_logger(std::string_view system) noexcept : system_name(system) {}

		template <typename... Args>
		void info(std::format_string<Args...> fmt, Args&&... args)
		{
			write_log(level::INFO, system_name, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void warn(std::format_string<Args...> fmt, Args&&... args)
		{
			write_log(level::WARNING, system_name, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void error(std::format_string<Args...> fmt, Args&&... args)
		{
			write_log(level::ERROR, system_name, fmt, std::forward<Args>(args)...);
		}
	};
};
