export module engine.core:logger;

import :types;
import std;

export namespace engine::core {
  class logger final {
    public:
      enum class level : u8 {
        info,
        warning,
        error,
        count
      };
      // configuration
      static void set_output_stream(std::ostream* stream) noexcept {
        std::scoped_lock lock(log_mutex);
        primary_file = stream;
      }

      static void set_dual_output(std::ostream* extra) noexcept {
        std::scoped_lock lock(log_mutex);
        additional_stream = extra;
      }

      static void reset_output() noexcept {
        std::scoped_lock lock(log_mutex);
        primary_file = &std::cerr;
        additional_stream = nullptr;
      }

      static void enable_colors(bool enable) noexcept {
        if (enable_color.load(std::memory_order_relaxed) != enable)
          enable_color.store(enable, std::memory_order_relaxed);
      }
      static void set_min_log_level(level level) noexcept {
        if (min_log_level.load(std::memory_order_relaxed) != level)
          min_log_level.store(level, std::memory_order_relaxed);
      }

      // Logging interface
      template <typename... Args>
        static void info(std::format_string<Args...> fmt, Args&&... args) {
          write_log<level::info>("GLOBAL", fmt, std::forward<Args>(args)...);
        }

      template <typename... Args>
        static void warn(std::format_string<Args...> fmt, Args&&... args) {
          write_log<level::warning>("GLOBAL", fmt, std::forward<Args>(args)...);
        }

      template <typename... Args>
        static void error(std::format_string<Args...> fmt, Args&&... args) {
          write_log<level::error>("GLOBAL", fmt, std::forward<Args>(args)...);
        }

      template<level lvl, typename... Args>
        static void log(std::format_string<Args...> fmt, Args&&... args) {
          write_log<lvl>("GLOBAL", fmt, std::forward<Args>(args)...);
        }

      template <typename... Args>
        static void system_info(std::string_view system, std::format_string<Args...> fmt, Args&&... args) {
          write_log<level::info>(system, fmt, std::forward<Args>(args)...);
        }

      template <typename... Args>
        static void system_warn(std::string_view system, std::format_string<Args...> fmt, Args&&... args) {
          write_log<level::warning>(system, fmt, std::forward<Args>(args)...);
        }

      template <typename... Args>
        static void system_error(std::string_view system, std::format_string<Args...> fmt, Args&&... args) {
          write_log<level::error>(system, fmt, std::forward<Args>(args)...);
        }

      struct attr { std::string_view key; std::string_view value; };

      // Structured logging (just key = value for now)
      template<level lvl>
      static void system_structured(std::string_view system, std::string_view message, std::initializer_list<attr> fields) {
        std::string out(message);
        for (const auto& [k,v] : fields)
        {
          out += " ";
          out += k;
          out += "=";
          out += v;
        }
        write_log<lvl>(system, "{}", out);
      }

      template<level lvl>
      static void structured(std::string_view message, std::initializer_list<attr> fields) {
        std::string out(message);
        for (const auto& [k,v] : fields)
        {
          out += " ";
          out += k;
          out += "=";
          out += v;
        }
        write_log<lvl>("GLOBAL", "{}", out);
      }

      // Scoped system logger
      class scoped_logger final {
        public:
        explicit scoped_logger(std::string_view name) noexcept : system_name(name) {}

        template <typename... Args>
          void info(std::format_string<Args...> fmt, Args&&... args)
          {
            write_log(level::info, system_name, fmt, std::forward<Args>(args)...);
          }
        template <typename... Args>
          void warn(std::format_string<Args...> fmt, Args&&... args)
          {
            write_log(level::warning, system_name, fmt, std::forward<Args>(args)...);
          }
        template <typename... Args>
          void error(std::format_string<Args...> fmt, Args&&... args)
          {
            write_log(level::error, system_name, fmt, std::forward<Args>(args)...);
          }
        private:
        std::string_view system_name{};
      };
      // TEMP

      inline static std::mutex imgui_mutex;
      inline static std::vector<std::string> imgui_buffer;
      inline static std::size_t max_imgui_lines{1000};
      // TEMP

    private:

      inline static std::mutex    log_mutex;
      inline static std::ostream* primary_file{&std::cerr};
      inline static std::ostream* additional_stream{nullptr};
      inline static std::atomic_bool enable_color{true};
      inline static std::atomic<level> min_log_level{level::info};
      static constexpr std::string_view reset_color{"\x1b[0m"};
      inline static std::atomic_uint32_t next_tid{0};
      inline static thread_local std::uint32_t tid{next_tid.fetch_add(1, std::memory_order_relaxed)};
      inline static constexpr level build_log_level{level::info};

      // TEMP

      static void push_imgui_log(std::string_view line) {
        std::scoped_lock lock(imgui_mutex);

        imgui_buffer.emplace_back(line);

        if (imgui_buffer.size() > max_imgui_lines)
          imgui_buffer.erase(imgui_buffer.begin());
      }
      // TEMP




      [[nodiscard]] static std::string_view get_color_code(level lvl) noexcept {
        switch(lvl) {
          case level::info:    return "\x1b[92m"; // Bright Green
          case level::warning: return "\x1b[93m"; // Bright Yellow
          case level::error:   return "\x1b[91m"; // Bright Red
          default:             return "";
        }
      }

      [[nodiscard]] static char level_to_char(level level) noexcept {
        switch (level) {
          case level::info:
            return 'I';
          case level::warning:
            return 'W';
          case level::error:
            return 'E';
          default:
            return '?';
        }
      }

      [[nodiscard]] static std::uint32_t thread_id_short() noexcept { return tid; }

      [[nodiscard]] static auto current_timestamp() {
        std::array<char, 32> buffer{};
        auto now = std::chrono::system_clock::now();
        auto ms = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

        // %T formats as HH:MM:SS.mmm automatically based on the time_point's precision
        std::format_to(buffer.data(), "{:%T}", ms);
        return buffer;
      }



//       [[nodiscard]] static std::string current_timestamp() {
//         using namespace std::chrono;
//
//         const auto now  = system_clock::now();
//         const auto ms   = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
//         const auto time = system_clock::to_time_t(now);
//
//         std::tm tm{};
// #ifdef _WIN32
//         localtime_s(&tm, &time);
// #else
//         localtime_r(&time, &tm);
// #endif
//         return std::format("{:02}:{:02}:{:02}.{:03}",
//             tm.tm_hour,
//             tm.tm_min,
//             tm.tm_sec,
//             ms.count());
//       }


      template <level lvl, typename... Args> requires (lvl < level::count)
        static void write_log(std::string_view system, std::format_string<Args...> fmt, Args&&... args) {
          if constexpr (lvl < build_log_level)
            return;

          if (lvl < min_log_level.load(std::memory_order_relaxed))
            return;

          auto message = std::format(fmt, std::forward<Args>(args)...);
          const auto timestamp = current_timestamp();
          std::ostream* out;
          std::ostream* extra;

          {
            std::scoped_lock lock(log_mutex);

            out = primary_file;
            extra = additional_stream;
          }

          bool use_colors = enable_color.load(std::memory_order_relaxed);

          std::string final_string;
          if (use_colors) {
            final_string = std::format("{} {}{}\x1b[0m T{} {}\t| {}",
                timestamp,
                get_color_code(lvl),
                level_to_char(lvl),
                tid,
                system,
                message);
          } else {
            final_string = std::format("{} {} T{} {}\t| {}",
                timestamp,
                level_to_char(lvl),
                tid,
                system,
                message);
          }
          // push_imgui_log(final_string);
          if (additional_stream) {
            std::println(*extra, "{}", final_string);
          } else {
            std::println(*out, "{}", final_string);
          }
        }

  };
}
