export module engine.renderer:gl_backend:framebuffer_manager;
//
// import std;
// export import :framebuffer;
// import engine.core;
// import engine.ecs;
//
// export struct FramebufferManager {
//
//   explicit FramebufferManager() = default;
//
// 	public:
//
// 	void create(Entity e, const RenderTarget& rt) {
//     auto& fb = fbs[e];
//     if (!fb.valid() || fb.needs_rebuild(rt)) {
//       fb.create(rt.width, rt.height, rt.attachments);
//     }
// 	}
//
// 	void ensure(Entity e, RenderTarget& rt, viewport_extent extent) {
//     u32 width{};
//     u32 height{};
//     switch (rt.mode) {
//       case extent_mode::follow_viewport:
//         rt.width = extent.width;
//         rt.height = extent.height;
//         break;
//       case extent_mode::fixed:
//         width = rt.width;
//         height = rt.height;
//         break;
//       default:
//         break;
//     }
//     create(e, rt);
// 	}
//
// 	const framebuffer& get(Entity e) const {
// 		return fbs.at(e);
// 	}
//
// 	void destroy(Entity e) {
// 		fbs.erase(e);
// 	}
//
// 	private:
// 	std::unordered_map<Entity, framebuffer> fbs;
// };
