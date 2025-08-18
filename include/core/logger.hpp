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

class log {
public:
    enum class LogLevel { INFO = 0, WARNING = 1, ERROR = 2 };

private:
    inline static std::mutex log_mutex;
    inline static std::ostream* additional_stream = nullptr;
    inline static std::ostream* primary_stream = &std::cerr;
    inline static bool enable_color = true;
    inline static LogLevel min_log_level = LogLevel::INFO;

    static std::string_view level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::INFO:    return enable_color ? "\x1b[1;92m[INFO]" : "[INFO]";
            case LogLevel::WARNING: return enable_color ? "\x1b[1;93m[WARN]" : "[WARN]";
            case LogLevel::ERROR:   return enable_color ? "\x1b[1;91m[ERROR]" : "[ERROR]";
            default:                return "[UNKNOWN]";
        }
    }

    static std::string current_timestamp() {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        auto time = system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time);

        std::ostringstream oss;
        oss << "[" << std::put_time(&tm, "%d-%m-%Y %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count() << "]";
        return oss.str();
    }

    static std::string thread_id_string() {
        std::ostringstream oss;
        oss << "[TID " << std::this_thread::get_id() << "]";
        return oss.str();
    }

    template<typename... Args>
    static void write_log(LogLevel level, std::string_view system, std::format_string<Args...> fmt, Args&&... args) {
        if (level < min_log_level) return;

        std::scoped_lock lock(log_mutex);
        auto& out = *primary_stream;

        const std::string RESET_COLOR = "\x1b[0m";
        std::string message = std::format(fmt, std::forward<Args>(args)...);

        std::string prefix = current_timestamp() + " " + thread_id_string() + " [" + std::string(system) + "] " + std::string(level_to_string(level));
        std::string full = prefix + " " + message + (enable_color ? RESET_COLOR : "") + "\n";

        out << full;
        out.flush();

        if (additional_stream) {
            *additional_stream << full;
            additional_stream->flush();
        }
    }

public:
    // Config
    static void set_output_stream(std::ostream& stream) {
        std::scoped_lock lock(log_mutex);
        primary_stream = &stream;
    }

    static void set_dual_output(std::ostream& extra) {
        std::scoped_lock lock(log_mutex);
        additional_stream = &extra;
    }

    static void reset_output_stream() {
        std::scoped_lock lock(log_mutex);
        primary_stream = &std::cerr;
        additional_stream = nullptr;
    }

    static void enable_colors(bool enable) {
        enable_color = enable;
    }

    static void set_min_log_level(LogLevel level) {
        min_log_level = level;
    }

    // Logging interface
    template<typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args) {
        write_log(LogLevel::INFO, "GLOBAL", fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warn(std::format_string<Args...> fmt, Args&&... args) {
        write_log(LogLevel::WARNING, "GLOBAL", fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args) {
        write_log(LogLevel::ERROR, "GLOBAL", fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void system_info(std::string_view system, std::format_string<Args...> fmt, Args&&... args) {
        write_log(LogLevel::INFO, system, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void system_warn(std::string_view system, std::format_string<Args...> fmt, Args&&... args) {
        write_log(LogLevel::WARNING, system, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void system_error(std::string_view system, std::format_string<Args...> fmt, Args&&... args) {
        write_log(LogLevel::ERROR, system, fmt, std::forward<Args>(args)...);
    }

    // Structured logging (just key=value for now)
    static void system_structured(std::string_view system, LogLevel level, std::string_view message, const std::map<std::string, std::string>& fields) {
        std::ostringstream msg;
        msg << message;
        for (auto it = fields.rbegin(); it != fields.rend(); ++it) {
            msg << " " << it->first << "=" << it->second;
        }
        write_log(level, system, "{}", msg.str());
    }

    static void structured(LogLevel level, std::string_view message, const std::map<std::string, std::string>& fields) {
        std::ostringstream msg;
        msg << message;
        for (auto it = fields.rbegin(); it != fields.rend(); ++it) {
            msg << " " << it->first << "=" << it->second;
        }
        write_log(level, "GLOBAL", "{}", msg.str());
    }

    // Scoped system logger
    class scoped_logger {
        std::string_view system_name;
    public:
        scoped_logger(std::string_view system) : system_name(system) {}

        template<typename... Args>
        void info(std::format_string<Args...> fmt, Args&&... args) {
            write_log(LogLevel::INFO, system_name, fmt, std::forward<Args>(args)...);
        }
        template<typename... Args>
        void warn(std::format_string<Args...> fmt, Args&&... args) {
            write_log(LogLevel::WARNING, system_name, fmt, std::forward<Args>(args)...);
        }
        template<typename... Args>
        void error(std::format_string<Args...> fmt, Args&&... args) {
            write_log(LogLevel::ERROR, system_name, fmt, std::forward<Args>(args)...);
        }
    };
};
