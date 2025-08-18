# ==== Compiler Flags ====
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    GLEW_STATIC
    PROJECT_NAME="${PROJECT_NAME}"
    PROJECT_VERSION="v${PROJECT_FULL_VERSION}"
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PROJECT_VERSION_SUFFIX="${PROJECT_VERSION_SUFFIX}"
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:
        -D_GLIBCXX_ASSERTIONS
        -g
        -fexceptions
        -Wall
        -Wextra
        -pedantic
        -Wuninitialized
        -ggdb
        -flto
    >
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:
        -O3
        -s
        -flto
        -fomit-frame-pointer
        -funsafe-math-optimizations
        -fno-math-errno
    >
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:
        /W4
        /Zi
        /EHsc
        /MDd
        /GL
    >
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:
        /O2
        /GL
        /MD
        /fp:fast
    >
)

target_link_options(${PROJECT_NAME} PRIVATE
    -static-libgcc -static-libstdc++
)
