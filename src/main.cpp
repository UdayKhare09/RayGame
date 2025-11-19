#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "Renderer.h"
#include "Camera.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"

#include "Scene.h"

SDL_Window* create_window_sdl(const char* window_name) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    SDL_Window* window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 768,
                                          SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    return window;
}

void destroy_window_sdl(SDL_Window* window) {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    SDL_Window* window = create_window_sdl("Vulkan Ray Tracer");
    if (!window) return -1;

    Renderer renderer;
    if (!renderer.init(window)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }

    Camera camera;
    Scene scene;
    bool quit = false;
    bool resize_requested = false;
    bool minimized = false;
    SDL_Event e;

    // FPS Counter state
    auto lastTime = std::chrono::high_resolution_clock::now();
    auto startTime = lastTime;
    int frameCount = 0;

    // Input state
    bool keyW = false, keyA = false, keyS = false, keyD = false;
    bool mouseCaptured = false;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                if (!ImGui::GetIO().WantCaptureKeyboard) {
                    if (e.key.keysym.sym == SDLK_w) keyW = true;
                    if (e.key.keysym.sym == SDLK_a) keyA = true;
                    if (e.key.keysym.sym == SDLK_s) keyS = true;
                    if (e.key.keysym.sym == SDLK_d) keyD = true;
                    if (e.key.keysym.sym == SDLK_l) scene.sunEnabled = !scene.sunEnabled; // Toggle Sun
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    mouseCaptured = !mouseCaptured;
                    SDL_SetRelativeMouseMode(mouseCaptured ? SDL_TRUE : SDL_FALSE);
                }
            } else if (e.type == SDL_KEYUP) {
                if (e.key.keysym.sym == SDLK_w) keyW = false;
                if (e.key.keysym.sym == SDLK_a) keyA = false;
                if (e.key.keysym.sym == SDLK_s) keyS = false;
                if (e.key.keysym.sym == SDLK_d) keyD = false;
            } else if (e.type == SDL_MOUSEMOTION) {
                if (mouseCaptured && !ImGui::GetIO().WantCaptureMouse) {
                    // Sensitivity
                    camera.rotate(e.motion.xrel * 0.005f, -e.motion.yrel * 0.005f);
                }
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED || e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    resize_requested = true;
                } else if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    minimized = true;
                } else if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    minimized = false;
                }
            }
        }

        if (minimized) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (resize_requested) {
            renderer.resize();
            resize_requested = false;
        }

        // Update Camera
        float moveSpeed = 0.1f;
        float fwd = 0.0f, rgt = 0.0f;
        if (keyW) fwd += moveSpeed;
        if (keyS) fwd -= moveSpeed;
        if (keyD) rgt += moveSpeed;
        if (keyA) rgt -= moveSpeed;
        camera.move(fwd, rgt);

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        if (renderer.draw(camera, time, scene) != 0) {
            // Handle error
        }

        // FPS Calculation
        frameCount++;
        float delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        if (delta >= 1.0f) {
            std::string title = "Vulkan Ray Tracer - FPS: " + std::to_string(frameCount) + " | Sun: " + (scene.sunEnabled ? "ON" : "OFF") + " (Press L, ESC to capture mouse)";
            SDL_SetWindowTitle(window, title.c_str());
            frameCount = 0;
            lastTime = currentTime;
        }
    }

    renderer.cleanup();
    destroy_window_sdl(window);
    return 0;
}
