#pragma once
#include <RmlUi/Core/SystemInterface.h>
#include <filesystem>

class SystemInterface : public Rml::SystemInterface
{

      public:
	// Returns the number of seconds since the program started
	double GetElapsedTime() override;

	// Handle mouse cursor changes from RmlUi (optional)
	void SetMouseCursor(const Rml::String& cursor_name) override;

	// Join document path with a relative path
	void JoinPath(Rml::String& translated_path, const Rml::String& document_path, const Rml::String& path) override;

	// Optional: logging override
	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;

	// Optional: clipboard support
	void SetClipboardText(const Rml::String& text) override;

	void GetClipboardText(Rml::String& text) override;

	// Optional: localization, system metrics, etc. can be overridden as needed
};
