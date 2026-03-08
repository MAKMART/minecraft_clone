module;
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
export module temporal_camera_system;

import ecs;
import ecs_components;
import glm;

export void camera_temporal_system(ECS& ecs)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	ecs.for_each_components<Camera, CameraTemporal, ActiveCamera>(
	    [&](Entity e, Camera& cam, CameraTemporal& temp, ActiveCamera&) {

		glm::mat4 curr_view_proj = cam.projectionMatrix * cam.viewMatrix;

		if (temp.first_frame) {
		temp.prev_view_proj = curr_view_proj;
		temp.first_frame = false;
		return;
		}
		// Advance time
		temp.prev_view_proj = curr_view_proj;
	    });
}
