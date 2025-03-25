// DependencyConfig.h
#pragma once

// Version requirements
#define REQUIRED_OPENGL_VERSION 120
#define REQUIRED_IMGUI_VERSION "1.89.9"  // Update with actual version
#define REQUIRED_IMPLOT_VERSION "0.16"   // Update with actual version

// ImGui/ImPlot configuration
#ifdef _WIN32
    #ifndef IMGUI_IMPL_API
        #define IMGUI_IMPL_API __declspec(dllimport)
    #endif
    #ifndef IMPLOT_IMPL_API
        #define IMPLOT_IMPL_API __declspec(dllimport)
    #endif
#else
    #ifndef IMGUI_IMPL_API
        #define IMGUI_IMPL_API
    #endif
    #ifndef IMPLOT_IMPL_API
        #define IMPLOT_IMPL_API
    #endif
#endif

// OpenGL configuration
#define GL_SILENCE_DEPRECATION
#if defined(_WIN32)
    #include <windows.h>
    #include <GL/gl.h>
#elif defined(__APPLE__)
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

// FLTK configuration
#define FL_INTERNALS // Required for some FLTK functionality 