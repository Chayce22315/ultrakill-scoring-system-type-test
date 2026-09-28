#include <SDL3/SDL.h>
#include <cmath>
#include <cstdio>

struct vec3 { float x{}, y{}, z{}; };

static float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float length(vec3 v) { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return 1;

    SDL_Window* window = SDL_CreateWindow(
        "ultrakill scoring type system test i dunno",
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) return 2;

    SDL_GLContext gl = SDL_GL_CreateContext(window);
    if (!gl) return 3;
    SDL_GL_SetSwapInterval(1);

    bool running = true;
    bool mouse_locked = true;
    bool sliding = false;
    float yaw = 0.0f;
    float pitch = 0.0f;
    vec3 pos{0.0f, 1.0f, 0.0f};
    vec3 vel{};
    const float gravity = -22.0f;
    const float ground_y = 1.0f;
    Uint64 previous = SDL_GetPerformanceCounter();

    SDL_SetWindowRelativeMouseMode(window, true);

    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - previous) / static_cast<float>(SDL_GetPerformanceFrequency());
        previous = now;
        dt = clamp(dt, 0.0f, 0.05f);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    mouse_locked = !mouse_locked;
                    SDL_SetWindowRelativeMouseMode(window, mouse_locked);
                }
                if (event.key.scancode == SDL_SCANCODE_SPACE && pos.y <= ground_y + 0.01f) {
                    vel.y = 8.5f;
                }
                if (event.key.scancode == SDL_SCANCODE_LCTRL || event.key.scancode == SDL_SCANCODE_C) {
                    sliding = true;
                }
            }
            if (event.type == SDL_EVENT_KEY_UP) {
                if (event.key.scancode == SDL_SCANCODE_LCTRL || event.key.scancode == SDL_SCANCODE_C) sliding = false;
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION && mouse_locked) {
                yaw += event.motion.xrel * 0.0025f;
                pitch = clamp(pitch - event.motion.yrel * 0.0025f, -1.45f, 1.45f);
            }
        }

        const bool* keys = SDL_GetKeyboardState(nullptr);
        vec3 wish{};
        if (keys[SDL_SCANCODE_W]) wish.z -= 1.0f;
        if (keys[SDL_SCANCODE_S]) wish.z += 1.0f;
        if (keys[SDL_SCANCODE_A]) wish.x -= 1.0f;
        if (keys[SDL_SCANCODE_D]) wish.x += 1.0f;
        float wish_len = length(wish);
        if (wish_len > 0.0f) {
            wish.x /= wish_len; wish.z /= wish_len;
            float cy = std::cos(yaw), sy = std::sin(yaw);
            vec3 forward{-sy, 0.0f, -cy};
            vec3 right{cy, 0.0f, -sy};
            vec3 move{right.x * wish.x + forward.x * wish.z, 0.0f,
                      right.z * wish.x + forward.z * wish.z};
            float accel = sliding ? 34.0f : 24.0f;
            vel.x += move.x * accel * dt;
            vel.z += move.z * accel * dt;
        }

        float horizontal = std::sqrt(vel.x*vel.x + vel.z*vel.z);
        float max_speed = sliding ? 22.0f : 14.0f;
        if (horizontal > max_speed) {
            float scale = max_speed / horizontal;
            vel.x *= scale; vel.z *= scale;
        }

        vel.y += gravity * dt;
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        pos.z += vel.z * dt;

        if (pos.y < ground_y) {
            pos.y = ground_y;
            if (vel.y < 0.0f) vel.y = 0.0f;
        }

        float speed = length(vel);
        float mach = speed / 343.0f;

        char title[256];
        std::snprintf(title, sizeof(title),
            "ultrakill scoring type system test i dunno | mach %.2f | slide %s",
            mach, sliding ? "yes" : "no");
        SDL_SetWindowTitle(window, title);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
