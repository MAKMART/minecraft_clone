set_project("my_engine")

set_version("1.0.1")

set_xmakever("3.0.9")

set_toolchains("clang")
add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")

add_requires("rmlui 6.2", "glm 1.0.3", "glfw 3.4")
add_requires("gtest 1.17.0")


if is_mode("debug") then
    set_symbols("debug")
    set_optimize("none")
elseif is_mode("release") then
    set_symbols("hidden")
    set_optimize("fastest")
end

target("glad")
  set_kind("static")
  -- set_showmenu(false)
  add_files("engine/vendor/glad/src/gl.c")
  add_includedirs("engine/vendor/glad/include")


-- add_cxxflags("-freflection", "-fexpansion-statements", "-freflection-latest")
target("engine")
  set_kind("static")
  set_languages("c++26")
  add_files("engine/**.cpp")
  add_files("engine/**.cppm", { public = true })
  remove_files("engine/vendor/**", "engine/renderer/ui/**")
  -- remove_files("engine/renderer/ui/**")
  remove_files("engine/renderer/gl/framebuffer_manager.cppm")
  remove_files("engine/renderer/debug_drawer.cppm")
  remove_files("engine/renderer/debug_drawer.cpp")
  remove_files("engine/tests/**")
  remove_files("engine/test_app/**")
  add_deps("glad")
  add_includedirs("engine/vendor/glad/include")
  add_includedirs("engine/vendor/stb")
  add_packages("rmlui")
  add_packages("glm")
  add_packages("glfw")
  if is_mode("debug") then
    add_cxxflags("-fsanitize=address", "-fsanitize=undefined")
    add_ldflags("-fsanitize=address", "-fsanitize=undefined")
  end

-- Engine tests
for _, file in ipairs(os.files("engine/tests/test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_group("tests")
        set_default(false)
        set_languages("c++26")
        add_deps("engine")
        add_files(file)
        add_files("engine/tests/main.cpp")
        add_packages("gtest")
        -- add_tests("default")
        add_tests("default", {runargs = "--gtest_color=yes"})
end

target("application_test")
  set_kind("binary")
  set_group("apps")
  set_languages("c++26")
  add_deps("engine")
  add_files("engine/test_app/test.cpp")
  -- add_files("engine/test_app/**.cppm")


-- target("game")
--   set_kind("binary")
--   set_languages("c++26")
--   add_deps("engine")
--   add_files("game/**.cpp")
--   add_files("game/**.cppm")
