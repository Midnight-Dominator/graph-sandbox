#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>
#include "app.h"   // includes MenuScreen + App + all headers

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow(
        "Graph Sandbox — Bridge & Articulation & SCC",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!win) { fprintf(stderr, "Window: %s\n", SDL_GetError()); return 1; }

    SDL_Renderer* r = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!r) { fprintf(stderr, "Renderer: %s\n", SDL_GetError()); return 1; }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Load fonts cho MenuScreen
    TTF_Font* f_lg = TTF_OpenFont("assets/font.ttf", 28);
    TTF_Font* f_md = TTF_OpenFont("assets/font.ttf", 16);
    TTF_Font* f_sm = TTF_OpenFont("assets/font.ttf", 12);
    if (!f_lg || !f_md || !f_sm) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Font Error", "Cannot open assets/font.ttf", win);
        return 1;
    }

    MenuScreen menu;
    App app;
    bool app_initialized = false;
    bool in_menu = true;
    bool global_running = true;

    menu.init();

    Uint32 prev = SDL_GetTicks();

    while (global_running) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - prev) / 1000.f;
        prev = now;
        if (dt > 0.1f) dt = 0.1f;

        // ── Collect events ──
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { global_running = false; break; }

            if (ev.type == SDL_WINDOWEVENT &&
                ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                SCREEN_W = ev.window.data1;
                SCREEN_H = ev.window.data2;
                menu.init();
            }

            if (in_menu) {
                // Menu events
                if (ev.type == SDL_MOUSEMOTION)
                    menu.update_hover(ev.motion.x, ev.motion.y);
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    if (menu.on_click(ev.button.x, ev.button.y)) {
                        // Người dùng đã chọn — khởi tạo / reset sandbox
                        if (!app_initialized) {
                            if (!app.init(r, menu.selected_directed)) {
                                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                    "Init failed", "Cannot load assets/font.ttf", win);
                                global_running = false;
                                break;
                            }
                            app_initialized = true;
                        } else {
                            app.reset_for_new_session(menu.selected_directed);
                        }
                        in_menu = false;
                        menu.done = false;
                    }
                }
            } else {
                // Sandbox events
                app.handle_event(ev);
                if (app.back_to_menu) {
                    in_menu = true;
                    menu.init();  // refresh card positions
                    app.back_to_menu = false;
                }
            }
        }

        // ── Update & Render ──
        if (in_menu) {
            menu.draw(r, f_lg, f_md, f_sm);
            SDL_RenderPresent(r);
        } else {
            app.update(dt);
            app.render();
        }
    }

    TTF_CloseFont(f_lg);
    TTF_CloseFont(f_md);
    TTF_CloseFont(f_sm);
    if (app_initialized) app.rdr.quit();
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
