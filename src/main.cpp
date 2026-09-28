#include <SDL3/SDL.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <string>

struct vec3 { float x{}, y{}, z{}; };
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
        1280, 720, SDL_WINDOW_RESIZABLE
    );
    if (!window) { SDL_Quit(); return 2; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) { SDL_DestroyWindow(window); SDL_Quit(); return 3; }

    bool running = true, sliding = false, flashlight_spinning = false;
    bool blackout = false, floor_complete = false, cashout = false;
    int floor = 1, fragments = 0, banked = 0;
    int fragment_target = 12;
    float coolness = 0, score = 0, yaw = 0, pitch = 0;
    float player_x = 0, player_y = 0, player_z = 1.7f;
    float vel_x = 0, vel_y = 0, vel_z = 0;
    bool grounded = true;

    struct Fragment { float x, y, z; bool taken; };
    std::vector<Fragment> fs;

    auto spawn = [&]() {
        fs.clear();
        for (int i = 0; i < fragment_target; ++i) {
            float a = i * 2.399963f + floor * .71f;
            float r = 5.0f + ((i * 73 + floor * 41) % 12);
            fs.push_back({std::cos(a) * r, std::sin(a) * r, .8f, false});
        }
        fragments = 0; floor_complete = false; cashout = false; blackout = false;
    };
    spawn();

    Uint64 previous = SDL_GetPerformanceCounter();
    bool mouse_captured = false;

    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = clampf(float(now - previous) / float(SDL_GetPerformanceFrequency()), 0, .05f);
        previous = now;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
                auto k = e.key.scancode;
                if (k == SDL_SCANCODE_ESCAPE) running = false;
                if (k == SDL_SCANCODE_TAB) { mouse_captured = !mouse_captured; SDL_SetWindowRelativeMouseMode(window, mouse_captured); }
                if (k == SDL_SCANCODE_LCTRL || k == SDL_SCANCODE_C) sliding = true;
                if (k == SDL_SCANCODE_SPACE && grounded && !floor_complete) { vel_z = 7.0f; grounded = false; }
                if (k == SDL_SCANCODE_F) flashlight_spinning = !flashlight_spinning;
                if (k == SDL_SCANCODE_B && !blackout && !floor_complete) blackout = true;
                if (floor_complete && k == SDL_SCANCODE_Y) { ++floor; fragment_target = std::min(20, 10 + floor % 11); spawn(); }
                if (floor_complete && k == SDL_SCANCODE_N) { banked += int(score); score = 0; cashout = true; }
            }
            if (e.type == SDL_EVENT_KEY_UP &&
                (e.key.scancode == SDL_SCANCODE_LCTRL || e.key.scancode == SDL_SCANCODE_C)) sliding = false;

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                mouse_captured = true;
                SDL_SetWindowRelativeMouseMode(window, true);
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION && mouse_captured && !floor_complete) {
                yaw += e.motion.xrel * .0025f;
                pitch = clampf(pitch - e.motion.yrel * .0025f, -1.35f, 1.35f);
            }
        }

        const bool* keys = SDL_GetKeyboardState(nullptr);
        float fx = 0, fy = 0;
        if (keys[SDL_SCANCODE_W]) fy += 1;
        if (keys[SDL_SCANCODE_S]) fy -= 1;
        if (keys[SDL_SCANCODE_A]) fx -= 1;
        if (keys[SDL_SCANCODE_D]) fx += 1;
        float fl = std::sqrt(fx*fx + fy*fy);
        if (fl > 0) { fx /= fl; fy /= fl; }

        float forward_x = std::sin(yaw), forward_y = std::cos(yaw);
        float right_x = std::cos(yaw), right_y = -std::sin(yaw);
        float wish_x = forward_x * fy + right_x * fx;
        float wish_y = forward_y * fy + right_y * fx;

        float accel = sliding ? 18.0f : 11.0f;
        vel_x += wish_x * accel * dt;
        vel_y += wish_y * accel * dt;
        float max_speed = sliding ? 18.0f : 10.0f;
        float sp = std::sqrt(vel_x*vel_x + vel_y*vel_y);
        if (sp > max_speed) { vel_x *= max_speed/sp; vel_y *= max_speed/sp; }
        if (fl == 0) { float drag = std::pow(.0008f, dt); vel_x *= drag; vel_y *= drag; }

        player_x += vel_x * dt; player_y += vel_y * dt;
        vel_z -= 20.0f * dt; player_z += vel_z * dt;
        if (player_z <= 1.7f) { player_z = 1.7f; vel_z = 0; grounded = true; }

        sp = std::sqrt(vel_x*vel_x + vel_y*vel_y);
        float mach = sp * 3.6f / 343.0f;
        score += mach * dt * 10.0f;
        if (sliding) score += sp * dt * 2.0f;
        if (sliding && sp > 8 && flashlight_spinning) coolness = std::min(100.0f, coolness + 35*dt);
        else coolness = std::max(0.0f, coolness - 8*dt);

        for (auto& f : fs) {
            float dx=f.x-player_x, dy=f.y-player_y, dz=f.z-player_z;
            if (!f.taken && std::sqrt(dx*dx+dy*dy+dz*dz) < .8f) {
                f.taken=true; ++fragments; score += 100;
            }
        }
        if (fragments >= fragment_target) floor_complete = true;

        float genx=10, geny=-8;
        if (blackout && std::sqrt((player_x-genx)*(player_x-genx)+(player_y-geny)*(player_y-geny)) < 1.3f) {
            blackout=false; score+=500;
        }

        int w,h; SDL_GetRenderOutputSize(renderer,&w,&h);
        SDL_SetRenderDrawColor(renderer, blackout ? 1:8, blackout ? 1:9, blackout ? 3:14,255);
        SDL_RenderClear(renderer);

        auto project = [&](float x,float y,float z, float& sx,float& sy,float& depth) -> bool {
            float dx=x-player_x, dy=y-player_y;
            float cy=std::cos(yaw), syaw=std::sin(yaw);
            float cx=dx*cy-dy*syaw, cz=dx*syaw+dy*cy;
            if (cz < .25f) return false;
            float relz=z-player_z;
            float cp=std::cos(pitch), spitch=std::sin(pitch);
            float vz=relz*cp-cz*spitch;
            float vd=relz*spitch+cz*cp;
            if (vd <= .1f) return false;
            float focal=700.0f;
            sx=w*.5f + cx*focal/vd;
            sy=h*.5f - vz*focal/vd;
            depth=vd; return true;
        };

        // 3d-ish world: floor, walls, fragments, generator, and first-person body.
        if (!blackout) {
            SDL_SetRenderDrawColor(renderer, 24,24,31,255);
            SDL_FRect floor_rect{0, h*.55f, float(w), h*.45f}; SDL_RenderFillRect(renderer,&floor_rect);
            for (int i=-12;i<=12;i++) {
                float sx,sy,d; if(project(float(i),-20,0,sx,sy,d)) SDL_RenderLine(renderer,sx,sy,sx,h*.98f);
            }
            for (int i=-20;i<=20;i+=2) {
                float sx,sy,d; if(project(-12,float(i),0,sx,sy,d)) SDL_RenderLine(renderer,sx,sy,w*.75f,h*.98f);
            }
        }

        for (const auto& f:fs) if(!f.taken) {
            float sx,sy,d; if(project(f.x,f.y,f.z,sx,sy,d) && d < 45) {
                float size=clampf(260/d,5,42);
                SDL_SetRenderDrawColor(renderer,230,220,90,255);
                SDL_FRect r{sx-size*.5f,sy-size*.5f,size,size}; SDL_RenderFillRect(renderer,&r);
            }
        }

        if (blackout) {
            float sx,sy,d; if(project(genx,geny,1.3f,sx,sy,d)) {
                float size=clampf(400/d,12,80);
                SDL_SetRenderDrawColor(renderer,80,170,255,255);
                SDL_FRect g{sx-size*.5f,sy-size*.5f,size,size}; SDL_RenderRect(renderer,&g);
            }
        }

        // simple first-person hands/flashlight silhouette
        SDL_SetRenderDrawColor(renderer,18,18,22,255);
        SDL_FRect hand{w*.52f,h*.76f,120,180}; SDL_RenderFillRect(renderer,&hand);
        if (flashlight_spinning) {
            float a=float(now%6283)/1000.0f;
            SDL_SetRenderDrawColor(renderer,255,245,180,255);
            SDL_RenderLine(renderer,w*.57f,h*.78f,w*.57f+std::cos(a)*90,h*.78f+std::sin(a)*90);
        }

        char hud[512];
        std::snprintf(hud,sizeof(hud),"floor %d   fragments %d/%d   score %.0f   banked %d   mach %.2f   coolness %.0f",
            floor,fragments,fragment_target,score,banked,mach,coolness);
        SDL_SetRenderDrawColor(renderer,235,235,240,255);
        SDL_RenderDebugText(renderer,30,30,hud);
        SDL_RenderDebugText(renderer,30,52,"wasd move | mouse look | tab unlock/relock mouse | space jump | ctrl/c slide | f flashlight | b blackout");

        if (floor_complete || cashout) {
            SDL_FRect p{w*.30f,h*.35f,w*.40f,150}; SDL_SetRenderDrawColor(renderer,8,8,12,240); SDL_RenderFillRect(renderer,&p);
            SDL_SetRenderDrawColor(renderer,220,220,230,255); SDL_RenderRect(renderer,&p);
            if (cashout) SDL_RenderDebugText(renderer,w*.34f,h*.43f,"CASHED OUT. RUN SECURED.");
            else { SDL_RenderDebugText(renderer,w*.34f,h*.41f,"FLOOR COMPLETE"); SDL_RenderDebugText(renderer,w*.34f,h*.46f,"Y = KEEP GOING    N = CASH OUT"); }
        }

        SDL_RenderPresent(renderer);
    }
    SDL_SetWindowRelativeMouseMode(window,false);
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
}
