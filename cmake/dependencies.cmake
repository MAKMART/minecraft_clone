include(FetchContent)

# ==== GLFW ====
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(glfw)

# ==== GLM ====
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(glm)

# ==== IMGUI ====
file(GLOB IMGUI_SOURCES 
    include/external/imgui/imgui.cpp
    include/external/imgui/imgui_draw.cpp
    include/external/imgui/imgui_tables.cpp
    include/external/imgui/imgui_widgets.cpp
    include/external/imgui/backends/imgui_impl_glfw.cpp
    include/external/imgui/backends/imgui_impl_opengl3.cpp
)
set(IMGUI_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/include/external/imgui
    ${PROJECT_SOURCE_DIR}/include/external/imgui/backends
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_link_libraries(imgui
    PUBLIC glfw
)
target_include_directories(imgui PUBLIC
    ${IMGUI_INCLUDE_DIRS}
    ${glfw_SOURCE_DIR}/include
)

# ==== FreeType ====
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-14-1  # pinned
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(freetype)
add_library(Freetype::Freetype ALIAS freetype)

# ==== RmlUi ====
FetchContent_Declare(
    RmlUi
    GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(RmlUi)

# ==== Tracy (optional) ====
if(ENABLE_TRACY)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG        master
	GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(tracy)
endif()
