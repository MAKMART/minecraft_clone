export module frame_context;
import std;
import ecs;
import glm;
import ecs_components;

export struct frame_context {
    double delta_time = 0.0;
    std::uint64_t frame_number = 0u;

    Entity active_camera;
    Camera* cam;
    glm::mat4 view_proj_matrix;
    glm::mat4 inv_view_proj_matrix;

		bool first_frame = true;
#if defined (DEBUG)
    bool debug_render = false;
#endif
};
