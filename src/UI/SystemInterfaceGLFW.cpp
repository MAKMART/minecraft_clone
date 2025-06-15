#include "SystemInterfaceGLFW.h"
#include <GLFW/glfw3.h>
#include <iostream>

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
        std::cerr << "JoinPath error: " << e.what() << std::endl;
        translated_path = path; // fallback to raw path
    }
}

bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String &message) {
    const char *type_str = "";
    switch (type) {
    case Rml::Log::Type::LT_ALWAYS:
        type_str = "ALWAYS";
        break;
    case Rml::Log::Type::LT_ERROR:
        type_str = "ERROR";
        break;
    case Rml::Log::Type::LT_ASSERT:
        type_str = "ASSERT";
        break;
    case Rml::Log::Type::LT_WARNING:
        type_str = "WARNING";
        break;
    case Rml::Log::Type::LT_INFO:
        type_str = "INFO";
        break;
    case Rml::Log::Type::LT_DEBUG:
        type_str = "DEBUG";
        break;
    }
    std::clog << "[RmlUi][" << type_str << "] " << message << std::endl;
    return false; // let RmlUi handle the message too
}

// Optional: clipboard support
void SystemInterface::SetClipboardText(const Rml::String &text) {
    // Implement this if you care about clipboard interaction
}

void SystemInterface::GetClipboardText(Rml::String &text) {
    // Implement this if clipboard is needed
}
