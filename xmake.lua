set_project("minecraft_clone") -- Or maybe test_game, I'm still not sure what I want to build

set_version("1.0.1")

set_xmakever("3.0.9")

set_toolchains("clang")
add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")

add_requires("git::https://github.com/MAKMART/engine.git", {
  configs = {
-- Add configs that you want
  }
})
-- Maybe add tests later
-- add_requires("gtest 1.17.0")


if is_mode("debug") then
    set_symbols("debug")
    set_optimize("none")
elseif is_mode("release") then
    set_symbols("hidden")
    set_optimize("fastest")
end


-- add_cxxflags("-freflection", "-fexpansion-statements", "-freflection-latest")
-- target("game")
--   set_kind("binary")
--   set_languages("c++26")
--   add_deps("engine")
--   add_files("game/**.cpp")
--   add_files("game/**.cppm")
