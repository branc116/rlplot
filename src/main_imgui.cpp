#include <iostream>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_extensions.h"
#include "raylib.h"
#include "GLFW/glfw3.h"
#include "plotter.h"
#include <cstdio>

extern context_t context;

// ../imgui/examples/example_glfw_opengl3/main.cpp
// ../imgui/examples/example_glfw_opengl3/Makefile.emscripten

int main() {
  int display_w = 1280, display_h = 720;
#ifdef RELEASE
  SetTraceLogLevel(LOG_TRACE);
#endif
  SetWindowState(FLAG_MSAA_4X_HINT);
  InitWindow(display_w, display_h, "brplot");
  SetWindowState(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  SetExitKey(KEY_NULL);

  graph_values_t* gv = (graph_values_t*)BR_MALLOC(sizeof(graph_values_t));
  graph_init(gv, 1, 1);
#ifndef RELEASE
  start_refreshing_shaders(gv);
#endif
  read_input_main(gv);

  GLFWwindow* ctx = glfwGetCurrentContext();
  ImGui::SetAllocatorFunctions(BR_IMGUI_MALLOC, BR_IMGUI_FREE, nullptr);
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
  ImGui::StyleColorsDark();
#ifdef PLATFORM_WEB
  const char* glsl_version = "#version 100";
  io.IniFilename = nullptr;
#elif PLATFORM_DESKTOP 
  const char* glsl_version = "#version 330";
#endif
  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(ctx, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  bool show_demo_window = true;
  ImVec4 clear_color = ImVec4(.0f, .0f, .0f, 1.00f);
  ImGuiStyle& s = ImGui::GetStyle();
  s.Colors[ImGuiCol_WindowBg].w = 0.f;
  float padding = 50.f;

  while (false == WindowShouldClose()) {
    BeginDrawing();
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
    if (ImGui::Begin("Test") && false == ImGui::IsWindowHidden()) {
      ImVec2 p = ImGui::GetWindowPos();
      ImVec2 size = ImGui::GetWindowSize();
      graph_draw_min(gv, p.x, p.y, size.x, size.y, padding);
    }
    ImGui::End();
#ifndef RELEASE
    if (show_demo_window) {
      ImGui::SetNextWindowBgAlpha(0.7f);
      ImGui::ShowDemoWindow(&show_demo_window);
    }
#endif
    ImGui::SetNextWindowBgAlpha(0.7f);
    if (ImGui::Begin("Settings")) {
      ImGui::SliderFloat("Padding", &padding, 0.f, 100.f);
      ImGui::SliderFloat("Recoil", &context.recoil, 0.f, 1.1f);
      ImGui::SliderFloat("Font scale", &context.font_scale, 0.f, 100.f);

      ImGui::Checkbox("Follow Arround", &gv->follow);
      ImGui::Checkbox("Debug Lines", &context.debug_bounds);
      ImGui::Checkbox("Jump Arround", &gv->jump_around);
      for (size_t i = 0; i < gv->groups.len; ++i) {
        sprintf(context.buff, "Plot %lu", i);
        ImGui::Checkbox(context.buff, &gv->groups.arr[i].is_selected);
        auto& c = gv->groups.arr[i].color;
        float colors[4] = { ((float)c.r)/255.f, ((float)c.g)/255.f, ((float)c.b)/255.f, ((float)c.a)/255.f};
        sprintf(context.buff, "PlotColor_%lu", i);
        ImGui::ColorPicker4(context.buff, colors);
        gv->groups.arr[i].color = { (unsigned char)(colors[0] * 256.f), (unsigned char)(colors[1] * 256.f), (unsigned char)(colors[2] * 256.f), (unsigned char)(colors[3] * 256.f) };
      }
    }
    ImGui::End();

    ImGui::SetNextWindowBgAlpha(0.7f);
    if (ImGui::Begin("Info") && false == ImGui::IsWindowHidden()) {
      //alloc_size, alloc_count, alloc_total_size, alloc_total_count, alloc_max_size, alloc_max_count, free_of_unknown_memory;
      if (ImGui::CollapsingHeader("Allocations")) {
        int s = sprintf(context.buff, "Allocations: %lu (%lu KB)", context.alloc_count, context.alloc_size >> 10); ImGui::TextUnformatted(context.buff, context.buff + s);
        s = sprintf(context.buff, "Total Allocations: %lu (%lu KB)", context.alloc_total_count, context.alloc_total_size >> 10); ImGui::TextUnformatted(context.buff, context.buff + s);
        s = sprintf(context.buff, "Max Allocations: %lu (%lu KB)", context.alloc_max_count, context.alloc_max_size >> 10); ImGui::TextUnformatted(context.buff, context.buff + s);
        s = sprintf(context.buff, "Unaccounted Allocations: >%lu", context.free_of_unknown_memory); ImGui::TextUnformatted(context.buff, context.buff + s);
      }
      if (ImGui::CollapsingHeader("Allocations")) {
      }
    }
    ImGui::End();

    ImGui::Render();
    glfwGetFramebufferSize(ctx, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    EndDrawing();
#ifndef PLATFORM_WEB
    glClear(GL_COLOR_BUFFER_BIT);
#endif
  }

  //Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  CloseWindow();
  read_input_stop();
  graph_free(gv);
  BR_FREE(gv);
  return 0;
}
