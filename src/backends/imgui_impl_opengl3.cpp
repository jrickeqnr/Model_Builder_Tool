#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <stdlib.h>

// Minimal implementation of OpenGL3 backend
// This is a stub implementation that provides just enough functionality
// for the ImGui implementation to compile and run

// Simple backend data structure
struct BackendData {
    unsigned int FontTexture;
};

static BackendData* g_BackendData = nullptr;

// ImGui_ImplOpenGL3_Init: Setup renderer backend
bool ImGui_ImplOpenGL3_Init(const char* glsl_version)
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_opengl3_minimal";

    // Create backend data
    g_BackendData = new BackendData();
    g_BackendData->FontTexture = 0;
    io.BackendRendererUserData = (void*)g_BackendData;

    return true;
}

// ImGui_ImplOpenGL3_Shutdown: Cleanup
void ImGui_ImplOpenGL3_Shutdown()
{
    if (g_BackendData)
    {
        ImGui::GetIO().BackendRendererUserData = nullptr;
        delete g_BackendData;
        g_BackendData = nullptr;
    }
}

// ImGui_ImplOpenGL3_NewFrame: Called at the start of a frame
void ImGui_ImplOpenGL3_NewFrame()
{
    // Nothing to do in the minimal implementation
}

// ImGui_ImplOpenGL3_RenderDrawData: Render ImGui draw data
void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data)
{
    // Simplified rendering - does nothing in this minimal implementation
    (void)draw_data;
}

// Optional: CreateFontsTexture
bool ImGui_ImplOpenGL3_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    
    // Just set a dummy texture ID
    g_BackendData->FontTexture = 1;
    io.Fonts->SetTexID((ImTextureID)(intptr_t)g_BackendData->FontTexture);
    
    return true;
}

// Optional: DestroyFontsTexture
void ImGui_ImplOpenGL3_DestroyFontsTexture()
{
    if (g_BackendData && g_BackendData->FontTexture)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->SetTexID(0);
        g_BackendData->FontTexture = 0;
    }
}

// Optional: CreateDeviceObjects
bool ImGui_ImplOpenGL3_CreateDeviceObjects()
{
    return ImGui_ImplOpenGL3_CreateFontsTexture();
}

// Optional: DestroyDeviceObjects
void ImGui_ImplOpenGL3_DestroyDeviceObjects()
{
    ImGui_ImplOpenGL3_DestroyFontsTexture();
} 