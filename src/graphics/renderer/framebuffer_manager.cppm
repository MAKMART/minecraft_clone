export module framebuffer_manager;

import std;
export import framebuffer;
import ecs;
import ecs_components;
import window_context;
import logger;

export struct FramebufferManager {
  explicit FramebufferManager(WindowContext& ctx) : win_ctx(ctx) {}

	public:

	void create(Entity e, const RenderTarget& rt) {
    auto& fb = fbs[e];
    if (!fb.valid() || fb.needs_rebuild(rt)) {
      fb.create(rt.width, rt.height, rt.attachments);
    }
	}

	void ensure(Entity e, RenderTarget& rt) {
    int width = -1, height = -1;
    switch (rt.mode) {
      case extent_mode::follow_viewport:
        rt.width = win_ctx.get_framebuffer_width();
        rt.height = win_ctx.get_framebuffer_height();
        break;
      case extent_mode::fixed:
        width = rt.width;
        height = rt.height;
        break;
      default:
        break;
    }
    create(e, rt);
	}

	const framebuffer& get(Entity e) const {
		return fbs.at(e);
	}

	void destroy(Entity e) {
		fbs.erase(e);
	}

	private:
  WindowContext& win_ctx;
	std::unordered_map<Entity, framebuffer> fbs;
};
