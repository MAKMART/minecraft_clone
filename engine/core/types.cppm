export module engine.core:types;
import std;
export namespace engine::core {
  using i8 = std::int8_t;
  using i16 = std::int16_t;
  using i32 = std::int32_t;
  using i64 = std::int64_t;

  using u8  = std::uint8_t;
  using u16 = std::uint16_t;
  using u32 = std::uint32_t;
  using u64 = std::uint64_t;

  using f32 = float;
  using f64 = double;

  // Input
  enum class button : u16 {
    unknown = 0,

    // letters
    a,
    b,
    c,
    d,
    e,
    f,
    g,

    w,
    s,

    // numbers
    num_0,
    num_1,

    // control
    escape,
    enter,
    tab,
    backspace,
    space,

    // modifiers
    left_shift,
    right_shift,
    left_ctrl,
    right_ctrl,
    left_alt,
    right_alt,

    // arrows
    up,
    down,
    left,
    right,

    // function keys
    f1,
    f2,

    // mouse
    mouse_left,
    mouse_right,
    mouse_middle,

    count
  };

  enum class cursor_mode : u8 {
    visible,
    hidden,
    locked
  };
}
export {
  using engine::core::i8;
  using engine::core::i16;
  using engine::core::i32;
  using engine::core::i64;

  using engine::core::u8;
  using engine::core::u16;
  using engine::core::u32;
  using engine::core::u64;

  using engine::core::f32;
  using engine::core::f64;
}
