// import engine.platform;
// import engine.core;
// namespace window_tests {
//   using namespace engine::platform;
//   using namespace engine::core;
//   TEST(WINDOW, window_creation) {
//     platform_context platform;
//     Window window(1920, 1080, "my window");
//     input_action_map action_map;
//     input_state state;
//     state.set_window(&window);
//     action_id close_window = action_map.create_action("close_window");
//     action_map.bind_key(close_window, button::backspace);
//     while(!window.should_close()) {
//       state.update();
//       if (action_map.is_action_pressed(close_window, state)) {
//         window.request_close();
//         std::cout << "closing window";
//       } else
//         logger::info("Window running");
//     }
//   }
//
// }
