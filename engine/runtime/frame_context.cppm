export module engine.app:frame_context;
import std;
import engine.core;
export namespace engine::app {
  // struct frame_context {};
}
/*
import engine::ecs;

export namespace engine::app {
  struct frame_context {
    f64 delta_time{};
    u64 frame_number{};

    Entity active_camera;
    Camera* cam;
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_proj_matrix;
    mat4 inv_view_proj_matrix;

    bool first_frame = true;
#if defined (DEBUG)
    bool debug_render = false;
#endif
  };
}
*/
