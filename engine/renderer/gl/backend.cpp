module;
#include <glad/gl.h>
module engine.renderer;

import std;
import engine.core;
using namespace engine::core;
std::expected<bool, std::string> gl_backend::init() {
  int version = gladLoaderLoadGL();
  if (!version)
    return std::unexpected("Failed to initialize GLAD!");
  // engine::core::logger::info(
  //     "Initialized OpenGL {}.{}",
  //     GLAD_VERSION_MAJOR(version),
  //     GLAD_VERSION_MINOR(version)
  //     );
  engine::core::logger::info(
      "OpenGL Version: {}",
      reinterpret_cast<const char*>(glGetString(GL_VERSION))
      );

  engine::core::logger::info(
      "GPU Vendor: {}",
      reinterpret_cast<const char*>(glGetString(GL_VENDOR))
      );

  engine::core::logger::info(
      "Renderer: {}",
      reinterpret_cast<const char*>(glGetString(GL_RENDERER))
      );
  return static_cast<bool>(version);
}
void gl_backend::shutdown() {

}
void gl_backend::draw(const render_command& command/*, ...*/) {

}
