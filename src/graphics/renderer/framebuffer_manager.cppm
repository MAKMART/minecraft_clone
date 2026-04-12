export module framebuffer_manager;

import std;
export import framebuffer;
import ecs;
import ecs_components;
import window_context;

export struct FramebufferManager {
  explicit FramebufferManager(WindowContext& ctx) : win_ctx(ctx) {}

	public:

	void create(Entity e, RenderTarget& rt) {
		auto& fb = fbs[e];
		fb.create(rt.width, rt.height, rt.attachments);
	}

	void ensure(Entity e, const RenderTarget& rt) {
		auto& fb = fbs[e];
    int width = -1, height = -1;
    switch (rt.mode) {
      case extent_mode::follow_viewport:
        width = win_ctx.get_width();
        height = win_ctx.get_height();
        break;
      case extent_mode::fixed:
        width = rt.width;
        height = rt.height;
        break;
      default:
        break;
    }
    if (!fb.valid() || fb.needs_rebuild(rt)) {
      fb.create(width, height, rt.attachments);
    }
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
