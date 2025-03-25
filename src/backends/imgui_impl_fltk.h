#pragma once

#include "imgui.h"
#include <FL/Fl_Window.H>

// Backend API
IMGUI_IMPL_API bool     ImGui_ImplFLTK_Init(Fl_Window* window);
IMGUI_IMPL_API void     ImGui_ImplFLTK_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplFLTK_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplFLTK_RenderDrawData(ImDrawData* draw_data);

// Optional: Platform interface
#if defined(_WIN32)
IMGUI_IMPL_API bool     ImGui_ImplFLTK_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplFLTK_InvalidateDeviceObjects();
#endif 