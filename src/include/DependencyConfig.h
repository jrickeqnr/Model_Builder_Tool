// DependencyConfig.h
#pragma once

// Version definitions
#define REQUIRED_IMGUI_VERSION "1.89.9"
#define REQUIRED_IMPLOT_VERSION "0.16"
#define REQUIRED_FLTK_VERSION "1.4.0"

// Platform-specific includes
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

// FLTK configuration
#include <FL/Fl.H>
#include <FL/Fl_Window.H>

// ImGui configuration
#include "imgui.h"
#include "implot.h"

// Eigen configuration
#include <Eigen/Dense>

// Standard library configuration
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <filesystem>

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