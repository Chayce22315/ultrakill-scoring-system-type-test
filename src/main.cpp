#include <SDL3/SDL.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>

struct vec2 { float x{}, y{}; };

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float length(vec2 v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

static vec2 normalize(vec2 v) {
    float n = length(v);
    return n > 0.0001f ? vec2{v.x / n, v.y / n} : vec2{};
}

static void text_title(SDL_Window* window, int floor, int fragments, int banked,
                       float mach, bool blackout, float coolness) {
    char title[256];
    std::snprintf(title, sizeof(title),
        "ultrakill scoring type system test i dunno | floor %d | fragments %d/12 | banked %d | mach %.2f | coolness %.0f%s",
        floor, fragments, banked, mach, coolness, blackout ? " | BLACKOUT" : "");
    SDL_SetWindowTitle(window, title);
}

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return 1;

    SDL_Window* window = SDL_CreateWindow(
        "ultrakill scoring type system test i dunno",
        1280, 720,
        SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        SDL_Quit();
        return 2;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 3;
    }

    bool running = true;
    bool sliding = false;
    bool flashlight_spinning = false;
    bool blackout = false;
    bool generator_powered = false;
    bool floor_complete = false;
    bool cashout = false;

    int floor = 1;
    int fragments = 0;
    int banked = 0;
    int fragment_target = 12;
    float coolness = 0.0f;
    float score = 0.0f;

    vec2 player{640.0f, 360.0f};
    vec2 velocity{};
    vec2 generator{1050.0f, 550.0f};

    std::vector<vec2> fragment_positions;
    auto spawn_fragments = [&]() {
        fragment_positions.clear();
        for (int i = 0; i < fragment_target; ++i) {
            float a = static_cast<float>(i) * 2.399963f + floor * 0.71f;
            float radius = 90.0f + static_cast<float>((i * 73 + floor * 41) % 230);
            fragment_positions.push_back({
                640.0f + std::cos(a) * radius,
                360.0f + std::sin(a) * radius
            });
        }
        fragments = 0;
        floor_complete = false;
        cashout = false;
        blackout = false;
        generator_powered = false;
        generator = {1050.0f, 550.0f};
    };
    spawn_fragments();

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
                const auto key = event.key.scancode;

                if (key == SDL_SCANCODE_ESCAPE) running = false;

                if (key == SDL_SCANCODE_LCTRL || key == SDL_SCANCODE_C)
                    sliding = true;

                if (key == SDL_SCANCODE_F)
                    flashlight_spinning = !flashlight_spinning;

                if (key == SDL_SCANCODE_B && !blackout && !floor_complete)
                    blackout = true;

                if (floor_complete) {
                    if (key == SDL_SCANCODE_Y) {
                        floor++;
                        fragment_target = std::min(20, 10 + floor % 11);
                        spawn_fragments();
                    }
                    if (key == SDL_SCANCODE_N) {
                        banked += static_cast<int>(score);
                        score = 0.0f;
                        cashout = true;
                    }
                }
            }

            if (event.type == SDL_EVENT_KEY_UP &&
                (event.key.scancode == SDL_SCANCODE_LCTRL ||
                 event.key.scancode == SDL_SCANCODE_C)) {
                sliding = false;
            }
        }

        const bool* keys = SDL_GetKeyboardState(nullptr);
        vec2 wish{};
        if (keys[SDL_SCANCODE_W]) wish.y -= 1.0f;
        if (keys[SDL_SCANCODE_S]) wish.y += 1.0f;
        if (keys[SDL_SCANCODE_A]) wish.x -= 1.0f;
        if (keys[SDL_SCANCODE_D]) wish.x += 1.0f;
        wish = normalize(wish);

        const float acceleration = sliding ? 1500.0f : 900.0f;
        velocity.x += wish.x * acceleration * dt;
        velocity.y += wish.y * acceleration * dt;

        const float max_speed = sliding ? 620.0f : 360.0f;
        float speed = length(velocity);
        if (speed > max_speed) {
            velocity.x *= max_speed / speed;
            velocity.y *= max_speed / speed;
        }

        if (length(wish) < 0.1f) {
            const float drag = std::pow(0.0008f, dt);
            velocity.x *= drag;
            velocity.y *= drag;
        }

        player.x += velocity.x * dt;
        player.y += velocity.y * dt;
        player.x = clampf(player.x, 32.0f, 1248.0f);
        player.y = clampf(player.y, 32.0f, 688.0f);

        speed = length(velocity);
        float mach = speed / 343.0f;

        if (sliding && speed > 300.0f && flashlight_spinning)
            coolness = std::min(100.0f, coolness + 35.0f * dt);
        else
            coolness = std::max(0.0f, coolness - 8.0f * dt);

        score += mach * dt * 3.0f;
        if (sliding) score += speed * dt * 0.008f;

        if (!floor_complete && !cashout) {
            for (auto it = fragment_positions.begin(); it != fragment_positions.end();) {
                if (length({player.x - it->x, player.y - it->y}) < 24.0f) {
                    ++fragments;
                    score += 100.0f;
                    it = fragment_positions.erase(it);
                } else {
                    ++it;
                }
            }

            if (fragments >= fragment_target) {
                fragments = fragment_target;
                floor_complete = true;
                blackout = false;
            }

            if (blackout && !generator_powered &&
                length({player.x - generator.x, player.y - generator.y}) < 55.0f) {
                generator_powered = true;
                blackout = false;
                score += 500.0f;
            }
        }

        text_title(window, floor, fragments, banked, mach, blackout, coolness);

        SDL_SetRenderDrawColor(renderer, 10, 10, 14, 255);
        SDL_RenderClear(renderer);

        SDL_FRect arena{24.0f, 24.0f, 1232.0f, 672.0f};
        SDL_SetRenderDrawColor(renderer, 35, 35, 45, 255);
        SDL_RenderFillRect(renderer, &arena);

        if (!blackout) {
            SDL_SetRenderDrawColor(renderer, 18, 18, 24, 255);
            for (int x = 48; x < 1248; x += 48)
                SDL_RenderLine(renderer, static_cast<float>(x), 24.0f,
                               static_cast<float>(x), 696.0f);
            for (int y = 48; y < 696; y += 48)
                SDL_RenderLine(renderer, 24.0f, static_cast<float>(y),
                               1256.0f, static_cast<float>(y));
        }

        for (const auto& f : fragment_positions) {
            SDL_FRect r{f.x - 7.0f, f.y - 7.0f, 14.0f, 14.0f};
            SDL_SetRenderDrawColor(renderer, 230, 220, 90, 255);
            SDL_RenderFillRect(renderer, &r);
        }

        if (blackout && !generator_powered) {
            SDL_FRect g{generator.x - 22.0f, generator.y - 22.0f, 44.0f, 44.0f};
            SDL_SetRenderDrawColor(renderer, 80, 170, 255, 255);
            SDL_RenderRect(renderer, &g);
        }

        SDL_FRect p{player.x - 10.0f, player.y - 10.0f, 20.0f, 20.0f};
        SDL_SetRenderDrawColor(renderer, 240, 240, 245, 255);
        SDL_RenderFillRect(renderer, &p);

        if (flashlight_spinning) {
            const float a = static_cast<float>(now % 6283) / 1000.0f;
            SDL_SetRenderDrawColor(renderer, 255, 245, 180, 255);
            SDL_RenderLine(renderer, player.x, player.y,
                           player.x + std::cos(a) * 42.0f,
                           player.y + std::sin(a) * 42.0f);
        }

        if (floor_complete || cashout) {
            SDL_FRect panel{365.0f, 275.0f, 550.0f, 170.0f};
            SDL_SetRenderDrawColor(renderer, 8, 8, 12, 245);
            SDL_RenderFillRect(renderer, &panel);
            SDL_SetRenderDrawColor(renderer, 220, 220, 230, 255);
            SDL_RenderRect(renderer, &panel);

            if (cashout) {
                SDL_SetRenderDrawColor(renderer, 120, 220, 140, 255);
                SDL_RenderDebugText(renderer, 430.0f, 325.0f, "CASHED OUT. RUN SECURED.");
            } else {
                SDL_SetRenderDrawColor(renderer, 230, 220, 100, 255);
                SDL_RenderDebugText(renderer, 430.0f, 310.0f, "FLOOR COMPLETE");
                SDL_RenderDebugText(renderer, 430.0f, 340.0f, "Y = KEEP GOING    N = CASH OUT");
            }
            char line[128];
            std::snprintf(line, sizeof(line), "UNBANKED SCORE: %.0f", score);
            SDL_RenderDebugText(renderer, 430.0f, 370.0f, line);
        } else {
            char hud[256];
            std::snprintf(hud, sizeof(hud),
                "floor %d   fragments %d/%d   score %.0f   mach %.2f   coolness %.0f",
                floor, fragments, fragment_target, score, mach, coolness);
            SDL_SetRenderDrawColor(renderer, 235, 235, 240, 255);
            SDL_RenderDebugText(renderer, 42.0f, 42.0f, hud);
            SDL_RenderDebugText(renderer,
                42.0f, 64.0f,
                "wasd move | ctrl/c slide | f flashlight spin | b blackout | esc quit");
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
