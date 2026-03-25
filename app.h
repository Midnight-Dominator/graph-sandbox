#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include "graph.h"
#include "animator.h"
#include "renderer.h"
#include "info_panel.h"

int SCREEN_W = 1150;
int SCREEN_H = 700;

// ── Mode enum ────────────────────────────────────────────────────────────────
enum class Mode {
    ADD_NODE,   // click để thêm đỉnh
    ADD_EDGE,   // drag để nối cạnh
    DELETE,     // click đỉnh/cạnh để xóa
    SELECT      // default — no action on canvas
};

// ── Log entry ────────────────────────────────────────────────────────────────
struct LogEntry { std::string msg; Col color; float alpha=1.f; };

class App {
public:
    SDL_Window*   win   = nullptr;
    SDL_Renderer* sdl_r = nullptr;
    Renderer      rdr;
    Graph         g;
    StepAnimator  anim;
    InfoPanel     info;

    Mode  mode       = Mode::ADD_NODE;
    bool  result_shown = false;   // Tarjan kết quả đang hiện
    bool  running    = true;

    // Animation type after submit
    enum class AnimType { DFS, BFS } anim_type = AnimType::DFS;

    // Drag state for ADD_EDGE
    bool  dragging   = false;
    int   drag_from  = -1;
    int   drag_mx    = 0, drag_my = 0;

    int   hover_node = -1;
    int   hover_edge = -1;

    // Log
    std::deque<LogEntry> logs;
    static constexpr int MAX_LOG = 8;

    // Speed slider
    Slider speed_slider;

    // Buttons (toolbar)
    std::vector<Button> btns;
    enum BtnId {
        BTN_ADD_NODE=0, BTN_ADD_EDGE, BTN_DELETE,
        BTN_SUBMIT, BTN_RUN_DFS, BTN_RUN_BFS,
        BTN_CLEAR_RESULT, BTN_RESET,
        BTN_TOGGLE_INFO,
        BTN_SKIP_ANIM,
        _BTN_COUNT
    };

    // ── Init ──────────────────────────────────────────────────────────────────
    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
        if (TTF_Init() < 0) return false;

        win = SDL_CreateWindow("Graph Sandbox — Bridge & Articulation",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (!win) return false;

        sdl_r = SDL_CreateRenderer(win, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!sdl_r) return false;

        SDL_SetRenderDrawBlendMode(sdl_r, SDL_BLENDMODE_BLEND);

        if (!rdr.init(sdl_r, "assets/font.ttf")) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Font Error", "Cannot open assets/font.ttf", win);
            return false;
        }

        build_toolbar();
        build_info_panel();

        // Speed slider
        speed_slider = {SCREEN_W - 230, SCREEN_H - 38, 150, 20, 0.3f, false, "Speed"};

        log("Graph Sandbox ready. Mode: ADD NODE", TXT_GRN);
        log("Click to place nodes. Drag between nodes to add edges.", TXT_DIM);
        return true;
    }

    void build_toolbar() {
        btns.resize(_BTN_COUNT);
        // Toolbar: left strip, vertical
        int tx=8, ty=60, bw=120, bh=34, gap=6;
        auto mk=[&](int id, const std::string& lbl, bool tog=false){
            btns[id]={tx,ty,bw,bh,lbl,false,false,false};
            ty+=bh+gap;
        };
        mk(BTN_ADD_NODE, "Add Node");
        mk(BTN_ADD_EDGE, "Add Edge");
        mk(BTN_DELETE,   "Delete");
        ty+=8; // separator
        mk(BTN_SUBMIT,   "Run Tarjan");
        mk(BTN_RUN_DFS,  "Animate DFS");
        mk(BTN_RUN_BFS,  "Animate BFS");
        mk(BTN_CLEAR_RESULT,"Clear Result");
        ty+=8;
        mk(BTN_RESET,    "Reset Graph");
        ty+=8;
        mk(BTN_TOGGLE_INFO,"Info Panel");
        mk(BTN_SKIP_ANIM, "Skip Anim");

        // Initial active state
        btns[BTN_ADD_NODE].active = true;
        btns[BTN_SKIP_ANIM].disabled = true;

        // active color
        for (auto& b : btns) b.active_col = BTN_ACT;
        btns[BTN_SUBMIT].active_col = {50,130,60};
        btns[BTN_RESET].active_col  = {120,40,40};
        btns[BTN_DELETE].active_col = {130,45,45};
    }

    void build_info_panel() {
        int pw=280, ph=SCREEN_H-70;
        info.set_bounds(SCREEN_W-pw-8, 58, pw, ph);
        info.init_tabs();
    }

    void set_mode(Mode m) {
        mode = m;
        btns[BTN_ADD_NODE].active = (m==Mode::ADD_NODE);
        btns[BTN_ADD_EDGE].active = (m==Mode::ADD_EDGE);
        btns[BTN_DELETE].active   = (m==Mode::DELETE);
        dragging = false; drag_from = -1;
    }

    void log(const std::string& msg, Col c=TXT_HI) {
        logs.push_front({msg, c, 1.f});
        if ((int)logs.size() > MAX_LOG) logs.pop_back();
    }

    // ── Main loop ─────────────────────────────────────────────────────────────
    void run() {
        Uint32 prev = SDL_GetTicks();
        while (running) {
            Uint32 now = SDL_GetTicks();
            float dt = (now-prev)/1000.f; prev=now;
            if (dt>0.1f) dt=0.1f;

            SDL_Event e;
            while (SDL_PollEvent(&e)) handle_event(e);

            update(dt);
            render();
        }
    }

    void quit() {
        rdr.quit();
        if(sdl_r) SDL_DestroyRenderer(sdl_r);
        if(win)   SDL_DestroyWindow(win);
        TTF_Quit(); SDL_Quit();
    }

    // ── Update ────────────────────────────────────────────────────────────────
    void update(float dt) {
        // Animate
        if (anim.running) {
            float spd = speed_slider.mapped(0.3f, 12.f);
            anim.speed = spd;
            if (anim.update(dt)) {
                auto* s = anim.current();
                if (s) log(s->log, TXT_HI);
            }
            btns[BTN_SKIP_ANIM].disabled = !anim.running;
        }
        // Pulse decay on bridges/art points
        for (auto& nd : g.nodes) if (nd.pulse>0) nd.pulse=fmodf(nd.pulse+dt*2.f,1.f);
        for (auto& ed : g.edges) if (ed.pulse>0) ed.pulse=fmodf(ed.pulse+dt*2.f,1.f);
        // Log fade
        for (auto& l:logs) l.alpha=1.f; // keep solid
    }

    // ── Events ────────────────────────────────────────────────────────────────
    void handle_event(SDL_Event& ev) {
        int mx,my; SDL_GetMouseState(&mx,&my);

        if (ev.type==SDL_QUIT) { running=false; return; }

        if (ev.type==SDL_WINDOWEVENT && ev.window.event==SDL_WINDOWEVENT_RESIZED) {
            SCREEN_W=ev.window.data1; SCREEN_H=ev.window.data2;
            build_toolbar(); build_info_panel();
            speed_slider.x=SCREEN_W-230; speed_slider.y=SCREEN_H-38;
        }

        if (ev.type==SDL_MOUSEMOTION) {
            for (auto& b:btns) b.update_hover(ev.motion.x,ev.motion.y);
            speed_slider.handle_move(ev.motion.x);
            update_hover(ev.motion.x, ev.motion.y);
            if (dragging) { drag_mx=ev.motion.x; drag_my=ev.motion.y; }
        }

        if (ev.type==SDL_MOUSEBUTTONDOWN && ev.button.button==SDL_BUTTON_LEFT) {
            int ex=ev.button.x, ey=ev.button.y;

            // Slider
            if (speed_slider.handle_down(ex,ey)) return;

            // Buttons
            for (int i=0;i<_BTN_COUNT;i++) {
                if (!btns[i].disabled && btns[i].contains(ex,ey)) {
                    on_btn(i); return;
                }
            }

            // Info panel tabs / click
            if (info.in_panel(ex,ey)) { info.on_click(ex,ey); return; }

            // Canvas actions
            on_canvas_down(ex,ey);
        }

        if (ev.type==SDL_MOUSEBUTTONUP && ev.button.button==SDL_BUTTON_LEFT) {
            speed_slider.handle_up();
            if (dragging) on_canvas_up(ev.button.x, ev.button.y);
        }

        if (ev.type==SDL_KEYDOWN) on_key(ev.key.keysym.sym);
    }

    void update_hover(int mx, int my) {
        hover_node=-1; hover_edge=-1;
        // Nodes first
        for (int i=0;i<(int)g.nodes.size();i++) {
            float dx=mx-g.nodes[i].x, dy=my-g.nodes[i].y;
            if (dx*dx+dy*dy <= (NODE_R+4)*(NODE_R+4)) { hover_node=i; return; }
        }
        // Edges
        for (int i=0;i<(int)g.edges.size();i++) {
            auto& e=g.edges[i];
            float ax=g.nodes[e.u].x,ay=g.nodes[e.u].y;
            float bx=g.nodes[e.v].x,by=g.nodes[e.v].y;
            float px=mx-ax,py=my-ay,ex2=bx-ax,ey2=by-ay;
            float len2=ex2*ex2+ey2*ey2;
            if(len2<1)continue;
            float t=std::clamp((px*ex2+py*ey2)/len2,0.f,1.f);
            float cx2=ax+t*ex2-mx,cy2=ay+t*ey2-my;
            if(cx2*cx2+cy2*cy2<=64){hover_edge=i;return;}
        }
    }

    // ── Canvas mouse down ─────────────────────────────────────────────────────
    void on_canvas_down(int mx, int my) {
        // Ignore toolbar area
        if (mx < 140) return;
        // Ignore info panel
        if (info.in_panel(mx,my)) return;

        switch (mode) {
        case Mode::ADD_NODE:
            if (hover_node < 0) { // only if not clicking existing node
                int id = g.add_node((float)mx,(float)my);
                g.nodes[id].spawn=0.f;
                log("Added node " + std::to_string(id), TXT_GRN);
                result_shown=false;
            }
            break;

        case Mode::ADD_EDGE:
            if (hover_node >= 0) {
                drag_from = hover_node;
                dragging  = true;
                drag_mx   = mx; drag_my = my;
            }
            break;

        case Mode::DELETE:
            if (hover_node >= 0) {
                log("Deleted node " + std::to_string(hover_node), TXT_RED);
                g.remove_node(hover_node);
                hover_node=-1; result_shown=false;
            } else if (hover_edge >= 0) {
                auto& e=g.edges[hover_edge];
                log("Deleted edge ("+std::to_string(e.u)+"-"+std::to_string(e.v)+")", TXT_RED);
                g.remove_edge(hover_edge);
                hover_edge=-1; result_shown=false;
            }
            break;

        default: break;
        }
    }

    void on_canvas_up(int mx, int my) {
        if (!dragging) return;
        dragging=false;
        if (mode != Mode::ADD_EDGE) { drag_from=-1; return; }

        update_hover(mx,my);
        if (hover_node >= 0 && hover_node != drag_from) {
            int ei = g.add_edge(drag_from, hover_node);
            if (ei >= 0) {
                log("Added edge ("+std::to_string(drag_from)+"-"+std::to_string(hover_node)+")", TXT_GRN);
                result_shown=false;
            } else {
                log("Edge already exists or invalid.", TXT_DIM);
            }
        }
        drag_from=-1;
    }

    // ── Button actions ────────────────────────────────────────────────────────
    void on_btn(int id) {
        switch(id) {
        case BTN_ADD_NODE: set_mode(Mode::ADD_NODE); log("Mode: Add Node", TXT_DIM); break;
        case BTN_ADD_EDGE: set_mode(Mode::ADD_EDGE); log("Mode: Add Edge  (drag between nodes)", TXT_DIM); break;
        case BTN_DELETE:   set_mode(Mode::DELETE);   log("Mode: Delete  (click node or edge)", TXT_RED); break;

        case BTN_SUBMIT:
            do_tarjan();
            break;

        case BTN_RUN_DFS:
            if (g.nodes.empty()) { log("Graph is empty.", TXT_RED); break; }
            do_tarjan(); // ensure result
            g.build_dfs_steps(0);
            anim.reset();
            for (auto& s : g.dfs_steps)
                anim.steps.push_back({s.u, s.v>=0?s.edge_idx:-1, s.v, s.backtrack, s.desc});
            anim.start();
            btns[BTN_SKIP_ANIM].disabled=false;
            log("DFS animation started.", TXT_GRN);
            break;

        case BTN_RUN_BFS:
            if (g.nodes.empty()) { log("Graph is empty.", TXT_RED); break; }
            do_tarjan();
            g.build_bfs_steps(0);
            anim.reset();
            for (auto& s : g.bfs_steps)
                anim.steps.push_back({s.u, s.edge_idx, s.v, false, s.desc});
            anim.start();
            btns[BTN_SKIP_ANIM].disabled=false;
            log("BFS animation started.", TXT_GRN);
            break;

        case BTN_CLEAR_RESULT:
            g.clear_analysis();
            anim.reset();
            result_shown=false;
            btns[BTN_SKIP_ANIM].disabled=true;
            log("Cleared analysis.", TXT_DIM);
            break;

        case BTN_RESET:
            g.nodes.clear(); g.edges.clear(); g.adj.clear();
            anim.reset(); result_shown=false;
            btns[BTN_SKIP_ANIM].disabled=true;
            log("Graph reset.", TXT_RED);
            break;

        case BTN_TOGGLE_INFO:
            info.visible=!info.visible;
            btns[BTN_TOGGLE_INFO].active=info.visible;
            break;

        case BTN_SKIP_ANIM:
            anim.skip_to_end();
            btns[BTN_SKIP_ANIM].disabled=true;
            log("Animation skipped.", TXT_DIM);
            break;
        }
    }

    void do_tarjan() {
        if (g.nodes.empty()) { log("Graph is empty.", TXT_RED); return; }
        g.run_tarjan();
        result_shown=true;
        // Count results
        int na=0, nb=0;
        for (auto& nd:g.nodes) if(nd.is_articulation) na++;
        for (auto& e :g.edges)  if(e.is_bridge) nb++;
        log("Tarjan done: "+std::to_string(na)+" art.points, "+std::to_string(nb)+" bridges",TXT_AMB);
        if (na==0 && nb==0) log("No bridges or articulation points found.",TXT_DIM);
    }

    void on_key(SDL_Keycode k) {
        if (k==SDLK_n) { set_mode(Mode::ADD_NODE); }
        if (k==SDLK_e) { set_mode(Mode::ADD_EDGE); }
        if (k==SDLK_d) { set_mode(Mode::DELETE); }
        if (k==SDLK_RETURN || k==SDLK_SPACE) {
            if (anim.running) anim.skip_to_end();
            else do_tarjan();
        }
        if (k==SDLK_i) { info.visible=!info.visible; btns[BTN_TOGGLE_INFO].active=info.visible; }
        if (k==SDLK_r) { on_btn(BTN_RESET); }
        if (k==SDLK_ESCAPE) running=false;
    }

    // ── Render ────────────────────────────────────────────────────────────────
    void render() {
        // Background
        sc(sdl_r, BG); SDL_RenderClear(sdl_r);

        // Dot grid
        SDL_SetRenderDrawBlendMode(sdl_r, SDL_BLENDMODE_BLEND);
        sc(sdl_r, GRID);
        for(int x=0;x<SCREEN_W;x+=36) for(int y=50;y<SCREEN_H-40;y+=36)
            SDL_RenderDrawPoint(sdl_r,x,y);
        SDL_SetRenderDrawBlendMode(sdl_r, SDL_BLENDMODE_NONE);

        // ── Draw edges ──
        for (int i=0;i<(int)g.edges.size();i++)
            rdr.draw_edge(g, i, result_shown||anim.running ? &anim : nullptr, result_shown);

        // Drag preview
        if (dragging && drag_from>=0) {
            int x0=(int)g.nodes[drag_from].x, y0=(int)g.nodes[drag_from].y;
            rdr.draw_drag_edge(x0,y0,drag_mx,drag_my);
        }

        // ── Draw nodes ──
        for (int i=0;i<(int)g.nodes.size();i++)
            rdr.draw_node(g, i, hover_node, -1,
                anim.running||anim.done ? &anim : nullptr, result_shown);

        // ── Toolbar ──
        rdr.panel(0, 0, 138, SCREEN_H, {10,14,30,245});
        render_text(sdl_r, rdr.f_sm, "GRAPH", 69, 16, TXT_AMB, true);
        render_text(sdl_r, rdr.f_sm, "SANDBOX", 69, 32, TXT_DIM, true);
        for (auto& b:btns) b.draw(sdl_r, rdr.f_md);

        // ── Top status bar ──
        rdr.panel(140, 0, SCREEN_W-140, 46, {8,12,28,240});
        // mode indicator
        std::string mode_str = mode==Mode::ADD_NODE?"ADD NODE":
                               mode==Mode::ADD_EDGE?"ADD EDGE (drag)":
                               mode==Mode::DELETE  ?"DELETE":"SELECT";
        render_text(sdl_r, rdr.f_md,
            "Mode: " + mode_str, 155, 15, TXT_AMB);

        // node/edge count
        render_text(sdl_r, rdr.f_sm,
            std::to_string(g.nodes.size())+" nodes  "+std::to_string(g.edges.size())+" edges",
            155, 33, TXT_DIM);

        // result summary top-right
        if (result_shown) {
            int na=0,nb=0;
            for(auto& nd:g.nodes)if(nd.is_articulation)na++;
            for(auto& e:g.edges) if(e.is_bridge)nb++;
            std::string rs="Art.Points: "+std::to_string(na)+"   Bridges: "+std::to_string(nb);
            render_text(sdl_r, rdr.f_md, rs, SCREEN_W-(info.visible?290:20), 23, TXT_AMB, false, true);
        }

        // Shortcut hint
        render_text(sdl_r, rdr.f_sm, "N=Node  E=Edge  D=Del  I=Info  ENTER=Run  ESC=Quit",
            SCREEN_W/2, 35, TXT_DIM, true);

        // ── Animation status ──
        if (anim.running || (anim.done && !anim.steps.empty())) {
            int bx=140, by=SCREEN_H-44, bw=SCREEN_W-(info.visible?300:20)-bx;
            rdr.panel(bx, by, bw, 38, {8,14,35,230});
            auto* s=anim.current();
            std::string logstr = s ? s->log : "Animation complete";
            render_text(sdl_r, rdr.f_md, logstr, bx+10, by+12, TXT_HI);
            rdr.progress_bar(bx+10, by+26, bw-20, 6, anim.progress());
        }

        // ── Speed slider label ──
        render_text(sdl_r, rdr.f_sm, "Anim Speed", SCREEN_W-240, SCREEN_H-30, TXT_DIM);
        speed_slider.x=SCREEN_W-185; speed_slider.y=SCREEN_H-42;
        speed_slider.w=160;
        speed_slider.draw(sdl_r, rdr.f_sm);

        // ── Log ──
        {
            int lx=145, ly=SCREEN_H-50;
            if (anim.running||(!anim.steps.empty()&&anim.done)) ly-=42;
            for (int i=0;i<(int)logs.size()&&i<4;i++) {
                Uint8 a=(Uint8)(255*(1.f-i*0.22f));
                Col c=logs[i].color; c.a=a;
                render_text(sdl_r, rdr.f_sm, logs[i].msg, lx, ly-i*15, c);
            }
        }

        // ── Legend ──
        if (result_shown) {
            int lx=SCREEN_W-(info.visible?295:10)-180, ly=54;
            rdr.panel(lx,ly,172,68,{10,14,30,220});
            // Art point
            SDL_SetRenderDrawBlendMode(sdl_r,SDL_BLENDMODE_BLEND);
            sc(sdl_r,NODE_ART); fill_circle(sdl_r,lx+14,ly+18,8);
            SDL_SetRenderDrawBlendMode(sdl_r,SDL_BLENDMODE_NONE);
            render_text(sdl_r,rdr.f_sm,"Articulation Point",lx+26,ly+18,TXT_AMB);
            // Bridge
            sc(sdl_r,EDGE_BRG); thick_line(sdl_r,lx+6,ly+44,lx+22,ly+44,3);
            render_text(sdl_r,rdr.f_sm,"Bridge",lx+26,ly+44,EDGE_BRG);
        }

        // ── Info panel ──
        info.draw(sdl_r, rdr.f_md, rdr.f_sm, rdr.f_mono, g);

        SDL_RenderPresent(sdl_r);
    }
};
