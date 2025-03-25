#include "imgui.h"
#include "imgui_impl_fltk.h"
#include "imgui_impl_opengl3.h"
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>
#include <FL/Enumerations.H>  // For FLTK constants
#include <ctime>              // For time handling

// FLTK data
struct ImGui_ImplFLTK_Data
{
    Fl_Window* Window;
    Fl_Gl_Window* GlWindow;
    bool MousePressed[3];
    bool MouseWheel;
    int MouseX, MouseY;
    float MouseWheelX, MouseWheelY;
    bool HasFocus;
    int Width, Height;
    double Time;
};

static ImGui_ImplFLTK_Data* g_Data = nullptr;

// Forward Declarations
static void ImGui_ImplFLTK_UpdateMousePosAndButtons();
static void ImGui_ImplFLTK_UpdateMouseCursor();
static void ImGui_ImplFLTK_UpdateGamepads();

// FLTK callbacks
static void ImGui_ImplFLTK_MouseCallback(Fl_Widget*, void*);
static void ImGui_ImplFLTK_KeyboardCallback(Fl_Widget*, void*);
static void ImGui_ImplFLTK_ResizeCallback(Fl_Widget*, void*);

bool ImGui_ImplFLTK_Init(Fl_Window* window, Fl_Gl_Window* gl_window)
{
    // Check and create ImGui context if not already created
    if (!ImGui::GetCurrentContext())
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    }

    g_Data = new ImGui_ImplFLTK_Data();
    g_Data->Window = window;
    g_Data->GlWindow = gl_window;
    g_Data->Time = 0.0;
    g_Data->MousePressed[0] = g_Data->MousePressed[1] = g_Data->MousePressed[2] = false;
    g_Data->MouseWheel = false;
    g_Data->MouseX = g_Data->MouseY = 0;
    g_Data->MouseWheelX = g_Data->MouseWheelY = 0.0f;
    g_Data->HasFocus = false;
    g_Data->Width = window->w();
    g_Data->Height = window->h();

    // Setup FLTK callbacks
    window->callback(ImGui_ImplFLTK_MouseCallback);
    window->callback(ImGui_ImplFLTK_KeyboardCallback);
    window->callback(ImGui_ImplFLTK_ResizeCallback);

    // Setup Platform/Renderer backends
    ImGui_ImplOpenGL3_Init("#version 150");

    return true;
}

void ImGui_ImplFLTK_Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    delete g_Data;
    g_Data = nullptr;
}

void ImGui_ImplFLTK_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2((float)g_Data->Width, (float)g_Data->Height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Setup time step - FLTK doesn't have a direct event_time() function
    // Use standard C++ time facilities instead
    double current_time = static_cast<double>(clock()) / CLOCKS_PER_SEC;
    io.DeltaTime = g_Data->Time > 0.0 ? (float)(current_time - g_Data->Time) : (float)(1.0f / 60.0f);
    g_Data->Time = current_time;

    // Update mouse position and buttons
    ImGui_ImplFLTK_UpdateMousePosAndButtons();

    // Update mouse cursor
    ImGui_ImplFLTK_UpdateMouseCursor();

    // Update gamepads
    ImGui_ImplFLTK_UpdateGamepads();
}

void ImGui_ImplFLTK_RenderDrawData(ImDrawData* draw_data)
{
    // Set up the OpenGL state for rendering
    if (g_Data && g_Data->GlWindow) {
        g_Data->GlWindow->make_current();
        
        // Simple version that doesn't try to save/restore OpenGL state
        // Just render directly
        ImGui_ImplOpenGL3_RenderDrawData(draw_data);
    }
}

static void ImGui_ImplFLTK_UpdateMousePosAndButtons()
{
    ImGuiIO& io = ImGui::GetIO();
    const bool is_app_focused = g_Data->HasFocus;
    io.AddFocusEvent(is_app_focused);

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos)
    {
        // FLTK doesn't have warp_mouse(), use cursor position directly
        g_Data->MouseX = (int)io.MousePos.x;
        g_Data->MouseY = (int)io.MousePos.y;
    }

    // Set mouse position
    io.AddMousePosEvent((float)g_Data->MouseX, (float)g_Data->MouseY);

    // Mouse buttons
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        io.AddMouseButtonEvent(i, g_Data->MousePressed[i]);
    }

    // Mouse wheel
    if (g_Data->MouseWheel)
    {
        io.AddMouseWheelEvent(g_Data->MouseWheelX, g_Data->MouseWheelY);
        g_Data->MouseWheel = false;
    }
}

static void ImGui_ImplFLTK_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    // FLTK cursor handling - use basic cursor types
    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Use default cursor
        g_Data->Window->cursor(FL_CURSOR_DEFAULT);
    }
    else
    {
        // Show OS mouse cursor
        int fl_cursor = FL_CURSOR_DEFAULT;
        switch (imgui_cursor)
        {
            case ImGuiMouseCursor_Arrow:        fl_cursor = FL_CURSOR_ARROW; break;
            case ImGuiMouseCursor_TextInput:    fl_cursor = FL_CURSOR_INSERT; break;
            case ImGuiMouseCursor_ResizeAll:    fl_cursor = FL_CURSOR_MOVE; break;
            case ImGuiMouseCursor_ResizeNS:     fl_cursor = FL_CURSOR_NS; break;
            case ImGuiMouseCursor_ResizeEW:     fl_cursor = FL_CURSOR_WE; break;
            case ImGuiMouseCursor_ResizeNESW:   fl_cursor = FL_CURSOR_NESW; break;
            case ImGuiMouseCursor_ResizeNWSE:   fl_cursor = FL_CURSOR_NWSE; break;
            case ImGuiMouseCursor_Hand:         fl_cursor = FL_CURSOR_HAND; break;
            case ImGuiMouseCursor_NotAllowed:   fl_cursor = FL_CURSOR_DEFAULT; break; // Not available in FLTK
        }
        g_Data->Window->cursor((Fl_Cursor)fl_cursor);
    }
}

static void ImGui_ImplFLTK_UpdateGamepads()
{
    // Simplified version that doesn't use NavInputs
    // ImGui modern versions use io.AddKeyEvent
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    // The gamepad mapping has been removed as it's not compatible with
    // newer versions of ImGui that use AddKeyEvent instead of direct NavInputs
}

static void ImGui_ImplFLTK_MouseCallback(Fl_Widget* widget, void*)
{
    if (!g_Data) return;

    switch (Fl::event())
    {
        case FL_PUSH:
            g_Data->MousePressed[Fl::event_button() - 1] = true;
            break;
        case FL_RELEASE:
            g_Data->MousePressed[Fl::event_button() - 1] = false;
            break;
        case FL_MOVE:
            g_Data->MouseX = Fl::event_x();
            g_Data->MouseY = Fl::event_y();
            break;
        case FL_MOUSEWHEEL:
            g_Data->MouseWheel = true;
            g_Data->MouseWheelX = Fl::event_dx();
            g_Data->MouseWheelY = Fl::event_dy();
            break;
    }
}

static void ImGui_ImplFLTK_KeyboardCallback(Fl_Widget* widget, void*)
{
    if (!g_Data) return;

    ImGuiIO& io = ImGui::GetIO();
    switch (Fl::event())
    {
        case FL_FOCUS:
            g_Data->HasFocus = true;
            break;
        case FL_UNFOCUS:
            g_Data->HasFocus = false;
            break;
        case FL_KEYDOWN:
            io.AddKeyEvent(ImGuiKey_ModCtrl, (Fl::event_state() & FL_CTRL) != 0);
            io.AddKeyEvent(ImGuiKey_ModShift, (Fl::event_state() & FL_SHIFT) != 0);
            io.AddKeyEvent(ImGuiKey_ModAlt, (Fl::event_state() & FL_ALT) != 0);
            io.AddKeyEvent(ImGuiKey_ModSuper, (Fl::event_state() & FL_META) != 0);
            
            // Map FLTK key events to ImGui key events
            // Only use keys that are guaranteed to exist in both FLTK and ImGui
            io.AddKeyEvent(ImGuiKey_Enter, Fl::event_key() == FL_Enter);
            io.AddKeyEvent(ImGuiKey_LeftArrow, Fl::event_key() == FL_Left);
            io.AddKeyEvent(ImGuiKey_RightArrow, Fl::event_key() == FL_Right);
            io.AddKeyEvent(ImGuiKey_UpArrow, Fl::event_key() == FL_Up);
            io.AddKeyEvent(ImGuiKey_DownArrow, Fl::event_key() == FL_Down);
            io.AddKeyEvent(ImGuiKey_PageUp, Fl::event_key() == FL_Page_Up);
            io.AddKeyEvent(ImGuiKey_PageDown, Fl::event_key() == FL_Page_Down);
            io.AddKeyEvent(ImGuiKey_Home, Fl::event_key() == FL_Home);
            io.AddKeyEvent(ImGuiKey_End, Fl::event_key() == FL_End);
            io.AddKeyEvent(ImGuiKey_Insert, Fl::event_key() == FL_Insert);
            io.AddKeyEvent(ImGuiKey_Delete, Fl::event_key() == FL_Delete);
            io.AddKeyEvent(ImGuiKey_Backspace, Fl::event_key() == FL_BackSpace);
            io.AddKeyEvent(ImGuiKey_Space, Fl::event_key() == ' ');
            io.AddKeyEvent(ImGuiKey_Escape, Fl::event_key() == FL_Escape);
            io.AddKeyEvent(ImGuiKey_Apostrophe, Fl::event_key() == '\'');
            io.AddKeyEvent(ImGuiKey_Comma, Fl::event_key() == ',');
            io.AddKeyEvent(ImGuiKey_Minus, Fl::event_key() == '-');
            io.AddKeyEvent(ImGuiKey_Period, Fl::event_key() == '.');
            io.AddKeyEvent(ImGuiKey_Slash, Fl::event_key() == '/');
            io.AddKeyEvent(ImGuiKey_Semicolon, Fl::event_key() == ';');
            io.AddKeyEvent(ImGuiKey_Equal, Fl::event_key() == '=');
            io.AddKeyEvent(ImGuiKey_LeftBracket, Fl::event_key() == '[');
            io.AddKeyEvent(ImGuiKey_Backslash, Fl::event_key() == '\\');
            io.AddKeyEvent(ImGuiKey_RightBracket, Fl::event_key() == ']');
            io.AddKeyEvent(ImGuiKey_GraveAccent, Fl::event_key() == '`');
            
            // Basic alphanumeric keys
            io.AddKeyEvent(ImGuiKey_0, Fl::event_key() == '0');
            io.AddKeyEvent(ImGuiKey_1, Fl::event_key() == '1');
            io.AddKeyEvent(ImGuiKey_2, Fl::event_key() == '2');
            io.AddKeyEvent(ImGuiKey_3, Fl::event_key() == '3');
            io.AddKeyEvent(ImGuiKey_4, Fl::event_key() == '4');
            io.AddKeyEvent(ImGuiKey_5, Fl::event_key() == '5');
            io.AddKeyEvent(ImGuiKey_6, Fl::event_key() == '6');
            io.AddKeyEvent(ImGuiKey_7, Fl::event_key() == '7');
            io.AddKeyEvent(ImGuiKey_8, Fl::event_key() == '8');
            io.AddKeyEvent(ImGuiKey_9, Fl::event_key() == '9');
            io.AddKeyEvent(ImGuiKey_A, Fl::event_key() == 'a');
            io.AddKeyEvent(ImGuiKey_B, Fl::event_key() == 'b');
            io.AddKeyEvent(ImGuiKey_C, Fl::event_key() == 'c');
            io.AddKeyEvent(ImGuiKey_D, Fl::event_key() == 'd');
            io.AddKeyEvent(ImGuiKey_E, Fl::event_key() == 'e');
            io.AddKeyEvent(ImGuiKey_F, Fl::event_key() == 'f');
            io.AddKeyEvent(ImGuiKey_G, Fl::event_key() == 'g');
            io.AddKeyEvent(ImGuiKey_H, Fl::event_key() == 'h');
            io.AddKeyEvent(ImGuiKey_I, Fl::event_key() == 'i');
            io.AddKeyEvent(ImGuiKey_J, Fl::event_key() == 'j');
            io.AddKeyEvent(ImGuiKey_K, Fl::event_key() == 'k');
            io.AddKeyEvent(ImGuiKey_L, Fl::event_key() == 'l');
            io.AddKeyEvent(ImGuiKey_M, Fl::event_key() == 'm');
            io.AddKeyEvent(ImGuiKey_N, Fl::event_key() == 'n');
            io.AddKeyEvent(ImGuiKey_O, Fl::event_key() == 'o');
            io.AddKeyEvent(ImGuiKey_P, Fl::event_key() == 'p');
            io.AddKeyEvent(ImGuiKey_Q, Fl::event_key() == 'q');
            io.AddKeyEvent(ImGuiKey_R, Fl::event_key() == 'r');
            io.AddKeyEvent(ImGuiKey_S, Fl::event_key() == 's');
            io.AddKeyEvent(ImGuiKey_T, Fl::event_key() == 't');
            io.AddKeyEvent(ImGuiKey_U, Fl::event_key() == 'u');
            io.AddKeyEvent(ImGuiKey_V, Fl::event_key() == 'v');
            io.AddKeyEvent(ImGuiKey_W, Fl::event_key() == 'w');
            io.AddKeyEvent(ImGuiKey_X, Fl::event_key() == 'x');
            io.AddKeyEvent(ImGuiKey_Y, Fl::event_key() == 'y');
            io.AddKeyEvent(ImGuiKey_Z, Fl::event_key() == 'z');
            
            // Function keys
            io.AddKeyEvent(ImGuiKey_F1, Fl::event_key() == FL_F + 1);
            io.AddKeyEvent(ImGuiKey_F2, Fl::event_key() == FL_F + 2);
            io.AddKeyEvent(ImGuiKey_F3, Fl::event_key() == FL_F + 3);
            io.AddKeyEvent(ImGuiKey_F4, Fl::event_key() == FL_F + 4);
            io.AddKeyEvent(ImGuiKey_F5, Fl::event_key() == FL_F + 5);
            io.AddKeyEvent(ImGuiKey_F6, Fl::event_key() == FL_F + 6);
            io.AddKeyEvent(ImGuiKey_F7, Fl::event_key() == FL_F + 7);
            io.AddKeyEvent(ImGuiKey_F8, Fl::event_key() == FL_F + 8);
            io.AddKeyEvent(ImGuiKey_F9, Fl::event_key() == FL_F + 9);
            io.AddKeyEvent(ImGuiKey_F10, Fl::event_key() == FL_F + 10);
            io.AddKeyEvent(ImGuiKey_F11, Fl::event_key() == FL_F + 11);
            io.AddKeyEvent(ImGuiKey_F12, Fl::event_key() == FL_F + 12);
            break;
        case FL_KEYUP:
            // Reset key states
            io.AddKeyEvent(ImGuiKey_ModCtrl, false);
            io.AddKeyEvent(ImGuiKey_ModShift, false);
            io.AddKeyEvent(ImGuiKey_ModAlt, false);
            io.AddKeyEvent(ImGuiKey_ModSuper, false);
            break;
    }
}

static void ImGui_ImplFLTK_ResizeCallback(Fl_Widget* widget, void*)
{
    if (!g_Data) return;
    g_Data->Width = widget->w();
    g_Data->Height = widget->h();
} 