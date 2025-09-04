#pragma once
#include <GLFW/glfw3.h>
class Application;
// class Renderer;
//	Maybe implement in future

struct WindowContext {
	Application*  app    = nullptr;
	GLFWwindow*   window = nullptr;
	// Renderer* renderer = nullptr;
};
