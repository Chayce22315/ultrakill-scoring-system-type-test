#include <SDL3/SDL.h>
#include <cmath>
#include <cstdio>

struct vec3 { float x{}, y{}, z{}; };

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float length(vec3 v) {
    return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return 1;

    SDL_Window* window = SDL_CreateWindow(
        "ultrakill scoring type system test i dunno",
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        SDL_Quit();
        return 2;
    }

    SDL_GLContext gl = SDL_GL_CreateContext(window);
    if (!gl) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 3;
    }

    SDL_GL_SetSwapInterval(1);
    SDL_SetWindowRelativeMouseMode(window, true);

    bool running = true;
    bool mouse_locked = true;
    bool sliding = false;

    float yaw = 0.0f;
    float pitch = 0.0f;
    vec3 position{0.0f, 1.0f, 0.0f};
    vec3 velocity{};

    constexpr float gravity = -22.0f;
    constexpr float ground_y = 1.0f;

    Uint64 previous = SDL_GetPerformanceCounter();

    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - previous) /
                   static_cast<float>(SDL_GetPerformanceFrequency());
        previous = now;
        dt = clampf(dt, 0.0f, 0.05f);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;

            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    mouse_locked = !mouse_locked;
                    SDL_SetWindowRelativeMouseMode(window, mouse_locked);
                }

                if (event.key.scancode == SDL_SCANCODE_SPACE &&
                    position.y <= ground_y + 0.01f) {
                    velocity.y = 8.5f;
                }

                if (event.key.scancode == SDL_SCANCODE_LCTRL ||
                    event.key.scancode == SDL_SCANCODE_C) {
                    sliding = true;
                }
            }

            if (event.type == SDL_EVENT_KEY_UP &&
                (event.key.scancode == SDL_SCANCODE_LCTRL ||
                 event.key.scancode == SDL_SCANCODE_C)) {
                sliding = false;
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION && mouse_locked) {
                yaw += event.motion.xrel * 0.0025f;
                pitch = clampf(
                    pitch - event.motion.yrel * 0.0025f,
                    -1.45f, 1.45f
                );
            }
        }

        const bool* keys = SDL_GetKeyboardState(nullptr);
        vec3 wish{};

        if (keys[SDL_SCANCODE_W]) wish.z -= 1.0f;
        if (keys[SDL_SCANCODE_S]) wish.z += 1.0f;
        if (keys[SDL_SCANCODE_A]) wish.x -= 1.0f;
        if (keys[SDL_SCANCODE_D]) wish.x += 1.0f;

        float wish_length = length(wish);
        if (wish_length > 0.0f) {
            wish.x /= wish_length;
            wish.z /= wish_length;

            float cy = std::cos(yaw);
            float sy = std::sin(yaw);

            vec3 forward{-sy, 0.0f, -cy};
            vec3 right{cy, 0.0f, -sy};

            vec3 movement{
                right.x * wish.x + forward.x * wish.z,
                0.0f,
                right.z * wish.x + forward.z * wish.z
            };

            float acceleration = sliding ? 34.0f : 24.0f;
            velocity.x += movement.x * acceleration * dt;
            velocity.z += movement.z * acceleration * dt;
        }

        float horizontal = std::sqrt(
            velocity.x * velocity.x + velocity.z * velocity.z
        );

        float max_speed = sliding ? 22.0f : 14.0f;
        if (horizontal > max_speed) {
            float scale = max_speed / horizontal;
            velocity.x *= scale;
            velocity.z *= scale;
        }

        velocity.y += gravity * dt;

        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        position.z += velocity.z * dt;

        if (position.y < ground_y) {
            position.y = ground_y;
            if (velocity.y < 0.0f) velocity.y = 0.0f;
        }

        float mach = length(velocity) / 343.0f;

        char title[256];
        std::snprintf(
            title, sizeof(title),
            "ultrakill scoring type system test i dunno | mach %.2f | slide %s",
            mach, sliding ? "yes" : "no"
        );
        SDL_SetWindowTitle(window, title);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
