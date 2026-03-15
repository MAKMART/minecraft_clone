module;
//#include <unordered_map>
#include "graphics/renderer/framebuffer.hpp"
export module framebuffer_manager;

import std;
import ecs;
import ecs_components;

export struct FramebufferManager {
	FramebufferManager() = default;

	public:

	void create(Entity e, RenderTarget& rt) {
		auto& fb = fbs[e];
		fb.create(rt.width, rt.height, rt.attachments);
	}

	void ensure(Entity e, const RenderTarget& rt) {
		auto& fb = fbs[e];
		if (!fb.valid() || fb.needs_rebuild(rt)) {
			fb.create(rt.width, rt.height, rt.attachments);
		}
	}

	const framebuffer& get(Entity e) const {
		return fbs.at(e);
	}

	void destroy(Entity e) {
		fbs.erase(e);
	}

	private:

	std::unordered_map<Entity, framebuffer> fbs;
};
