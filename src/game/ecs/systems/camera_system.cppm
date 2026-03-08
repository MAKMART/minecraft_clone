module;
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
export module camera_system;
import ecs;
import ecs_components;
import glm;

export void camera_system(ECS& ecs)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	ecs.for_each_components<Camera, Transform, ActiveCamera>(
	    [&](Entity e, Camera& cam, Transform& transform, ActiveCamera&) {

		
		transform.rot = glm::normalize(transform.rot);

		cam.forward = transform.rot * glm::vec3(0, 0, -1);
		cam.right   = transform.rot * glm::vec3(1, 0, 0);
		cam.up      = transform.rot * glm::vec3(0, 1, 0);

		cam.viewMatrix =
		glm::mat4_cast(glm::conjugate(transform.rot)) *
		glm::translate(glm::mat4(1.0f), -transform.pos);

		// Note that we use RH_ZO because we use reverse-Z depth, if you want to change to forward-Z just change to RH_NO and switch the far with the near plane and vice-versa
		    cam.projectionMatrix = glm::perspectiveRH_ZO(
		        glm::radians(cam.fov),
		        cam.aspect_ratio,
		        cam.far_plane,
		        cam.near_plane);
	    });
}
