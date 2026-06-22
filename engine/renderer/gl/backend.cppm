export module engine.renderer:gl_backend;

import std;
import engine.core;
using namespace engine::core;
export class gl_backend {
  public:
    gl_backend() = delete;

    static std::expected<bool, std::string> init();
    static void shutdown();

    void draw(const render_command& command/*, ...*/);
};
