module engine.runtime;

import std;
import engine.core;
import engine.platform;
import engine.renderer;

using namespace engine::platform;
using namespace engine::core;
using namespace engine::renderer;
namespace engine {
  void run(App& application) {
    app_config config = application.get_config();
    platform_context context;
    events::event_dispatcher dispatcher;
    events::event_queue event_queue;
    Window window(config.window, dispatcher);
    input_action_map input_map;
    input_state input(dispatcher);
    Renderer renderer;

    application.window = &window;
    application.input = &input;
    application.input_map = &input_map;
    application.renderer = &renderer;

    dispatcher.subscribe<events::window_resize>([](const events::window_resize& resize_event) {
        logger::info("Window resized: {} x {}", resize_event.width, resize_event.height);
        });
    dispatcher.subscribe<events::mouse_button>([](const events::mouse_button& mouse_button_event) {
        logger::info("Mouse button {} pressed", mouse_button_event.button);
        logger::info("Window close event dispatched");
        });
    dispatcher.subscribe<events::window_close>([](const events::window_close close_event) {
        logger::info("Window close event dispatched");
        });
    dispatcher.subscribe<events::window_focus>([](const events::window_focus& focus_event) {
          logger::info("Window {} focus", focus_event.focused ? "gained" : "lost");
        });

    window.set_scroll_callback(
        [&input](f64 x_offset, f64 y_offset){
        input.on_scroll_callback(x_offset, y_offset);
        }
        );
    window.set_cursor_position_callback(
        [&input](f64 x_pos, f64 y_pos){
        input.on_cursor_pos_callback(x_pos, y_pos);
        }
        );
    window.register_callbacks();

    application.on_init();
    application.running = true;

    f64 last_time = platform_context::get_time();

    while (application.running && !window.should_close()) {
      f64 now = platform_context::get_time();
      f64 dt = now - last_time;
      last_time = now;


      // Internal engine logic here (e.g., poll events)


      window.poll_events();
      event_queue.flush(dispatcher);  // Where should this go, before or after on_update???
      application.on_update(dt);



      renderer.start_frame();
      application.on_render();
      renderer.end_frame();

      window.present();
    }
    renderer.shutdown();
    application.on_shutdown();

  }
}
