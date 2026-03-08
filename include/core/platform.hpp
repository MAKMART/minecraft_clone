#pragma once

// --- Platform Detection ---
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define KPLATFORM_WINDOWS 1
    #ifndef _WIN64
        #error "64-bit is required on Windows!"
    #endif
#elif defined(__linux__) || defined(__gnu_linux__)
    #define KPLATFORM_LINUX 1
    #if defined(__ANDROID__)
        #define KPLATFORM_ANDROID 1
    #endif
#elif defined(__APPLE__)
    #define KPLATFORM_APPLE 1
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define KPLATFORM_IOS 1
    #elif TARGET_OS_MAC
        // MacOS
    #else
        #error "Unknown Apple platform"
    #endif
#else
    #error "Unknown platform!"
#endif

// Properly define static assertions
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// --- ANSI Escape Codes for CLI Colors ---
// (Kept as macros because they are often used in format strings)
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RESET   "\033[0m"

// --- Global Utility Macros ---
// Macros cannot be exported by modules, so keep helper macros here
#define UNUSED(x) (void)(x)

// --- Primitive Logic Macros ---
// These are classic; though 'constexpr bool' in a module is usually better.
#ifndef TRUE
    #define TRUE 1
#endif
#ifndef FALSE
    #define FALSE 0
#endif

// --- Build Configuration ---
#ifdef DEBUG
    #define KDEBUG 1
#else
    #define KDEBUG 0
#endif



#define SIZE_OF(x) pretty_size(sizeof(x))
