module;
#include <cstdio>
#include <ctime>
#if defined(__linux__)
#include <unistd.h>
#endif
export module logger;

import std;
export class log
{
	public:
		enum class level { INFO = 0, WARNING = 1, ERROR = 2 };
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

		static void enable_colors(bool enable) noexcept { 
			if (enable_color != enable)
				enable_color = enable;
		}
		static void set_min_log_level(level level) noexcept { 
			if (min_log_level != level)
				min_log_level = level; 
		}

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

		struct attr { std::string_view key; std::string_view value; };

		// Structured logging (just key=value for now)
		static void system_structured(std::string_view system, level level, std::string_view message, std::initializer_list<attr> fields)
		{
			std::string out(message);
			for (const auto& [k,v] : fields)
			{
				out += " ";
				out += k;
				out += "=";
				out += v;
			}
			write_log(level, system, "{}", out);
		}

		static void structured(level level, std::string_view message, std::initializer_list<attr> fields)
		{
			std::string out(message);
			for (const auto& [k,v] : fields)
			{
				out += " ";
				out += k;
				out += "=";
				out += v;
			}
			write_log(level, "GLOBAL", "{}", out);
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

	private:


		inline static std::mutex    log_mutex;
		inline static FILE *primary_file = stderr;
		inline static std::ostream* additional_stream = nullptr;
		inline static bool          enable_color      = true;
		inline static level      min_log_level     = level::INFO;
		static constexpr std::string_view reset_color = "\x1b[0m";
		inline static std::atomic<std::uint32_t> next_tid = 1;


		inline static thread_local std::uint32_t tid = next_tid.fetch_add(1, std::memory_order_relaxed);

		[[nodiscard]] static std::string_view get_color_code(level lvl) noexcept {
			switch(lvl) {
				case level::INFO:    return "\x1b[92m"; // Bright Green
				case level::WARNING: return "\x1b[93m"; // Bright Yellow
				case level::ERROR:   return "\x1b[91m"; // Bright Red
				default:             return "";
			}
		}

		[[nodiscard]] static char level_to_char(level level) noexcept
		{
			switch (level) {
				case level::INFO:
					return 'I';
				case level::WARNING:
					return 'W';
				case level::ERROR:
					return 'E';
				default:
					return '?';
			}
		}

		[[nodiscard]] static std::uint32_t thread_id_short() noexcept { return tid; }

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
			return std::format("{:02}:{:02}:{:02}.{:03}",
					tm.tm_hour,
					tm.tm_min,
					tm.tm_sec,
					ms.count());
		}


		[[nodiscard]] static std::string thread_id_string()
		{
			return std::format("TID {}", std::this_thread::get_id());
		}

		template <typename... Args>
			static void write_log(level level, std::string_view system, std::format_string<Args...> fmt, Args&&... args)
			{
				if (level < min_log_level)
					return;

				auto message = std::format(fmt, std::forward<Args>(args)...);
				const auto timestamp = current_timestamp();

				std::scoped_lock lock(log_mutex);
				FILE* out = (primary_file) ? primary_file : (level == level::INFO ? stdout : stderr);

				// Check if we should actually use colors
				// requires #include <unistd.h> on Linux
#if defined (__linux__)
				bool use_colors = enable_color && isatty(fileno(out));
#else
        bool use_colors = enable_color;
#endif

				if (use_colors) {
					std::println(out, "{} {}{}\x1b[0m T{} {}\t| {}",
							timestamp,
							get_color_code(level),
							level_to_char(level),
							tid,
							system,
							message);
				} else {
					std::println(out, "{} {} T{} {}\t| {}",
							timestamp,
							level_to_char(level),
							tid,
							system,
							message);
				}

				if (additional_stream) {
					std::println(*additional_stream, "{} {} T{} {}\t| {}",
							timestamp, level_to_char(level), tid, system, message);
				}
			}

};
