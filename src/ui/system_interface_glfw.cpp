#include "ui/system_interface_glfw.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include "core/logger.hpp"

double SystemInterface::GetElapsedTime() {
    return glfwGetTime();
}

void SystemInterface::SetMouseCursor(const Rml::String &cursor_name) {
    // If you're using GLFW cursors, map cursor_name to GLFW cursors here
    // Example stub:
    // if (cursor_name == "arrow") glfwSetCursor(window, arrowCursor);
    // else if (cursor_name == "text") glfwSetCursor(window, textCursor);
}
void SystemInterface::JoinPath(Rml::String &translated_path, const Rml::String &document_path, const Rml::String &path) {
    try {
        std::filesystem::path doc_path(document_path);
        std::filesystem::path result = doc_path.parent_path() / path;
        translated_path = result.generic_string();
    } catch (const std::exception &e) {
        log::system_error("RmlUi", "JoinPath error: {}", e.what());
        translated_path = path; // fallback to raw path
    }
}

bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String &message) {
    const char *type_str = "";
    switch (type) {
        case Rml::Log::Type::LT_ALWAYS:
            type_str = "ALWAYS";
            log::system_info("RmlUi", "[{}] {}", type_str, message);
            break;
        case Rml::Log::Type::LT_ERROR:
            type_str = "ERROR";
            log::system_error("RmlUi", " {}", message);
            break;
        case Rml::Log::Type::LT_ASSERT:
            type_str = "ASSERT";
            log::system_info("RmlUi", "[{}] {}", type_str, message);
            break;
        case Rml::Log::Type::LT_WARNING:
            type_str = "WARNING";
            log::system_warn("RmlUi", " {}", message);
            break;
        case Rml::Log::Type::LT_INFO:
            type_str = "INFO";
            log::system_info("RmlUi", " {}", message);
            break;
        case Rml::Log::Type::LT_DEBUG:
            type_str = "DEBUG";
            log::system_info("RmlUi", "[{}] {}", type_str, message);
            break;
    }
    return false; // let RmlUi handle the message too
}

// Optional: clipboard support
void SystemInterface::SetClipboardText(const Rml::String &text) {
    // Implement this if you care about clipboard interaction
}

void SystemInterface::GetClipboardText(Rml::String &text) {
    // Implement this if clipboard is needed
}
