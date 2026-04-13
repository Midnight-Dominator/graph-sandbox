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

int SCREEN_W=1150, SCREEN_H=700;

enum class Mode { ADD_NODE, ADD_EDGE, DELETE };

struct LogEntry { std::string msg; Col color; };

// ─────────────────────────────────────────────────────────────────────────────
// MENU SCREEN
// ─────────────────────────────────────────────────────────────────────────────
class MenuScreen {
public:
    // Returns true khi người dùng đã chọn
    bool done=false;
    bool selected_directed=false;

    struct Card { int x,y,w,h; bool hovered=false; bool directed; };
    std::vector<Card> cards;

    void init(){
        cards.clear();
        int cw=340, ch=200, gap=60;
        int total=cw*2+gap;
        int ox=(SCREEN_W-total)/2, oy=SCREEN_H/2-80;
        cards.push_back({ox,        oy,cw,ch,false,false});
        cards.push_back({ox+cw+gap, oy,cw,ch,false,true });
    }

    void update_hover(int mx,int my){
        for(auto& c:cards)
            c.hovered=(mx>=c.x&&mx<c.x+c.w&&my>=c.y&&my<c.y+c.h);
    }

    bool on_click(int mx,int my){
        for(auto& c:cards){
            if(mx>=c.x&&mx<c.x+c.w&&my>=c.y&&my<c.y+c.h){
                selected_directed=c.directed;
                done=true;
                return true;
            }
        }
        return false;
    }

    void draw(SDL_Renderer* r,TTF_Font* f_lg,TTF_Font* f_md,TTF_Font* f_sm){
        // Background
        SDL_SetRenderDrawColor(r,8,10,18,255);
        SDL_RenderClear(r);

        // Dot grid
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r,18,24,40,255);
        for(int x=0;x<SCREEN_W;x+=36) for(int y=0;y<SCREEN_H;y+=36)
            SDL_RenderDrawPoint(r,x,y);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);

        // Title
        render_text(r,f_lg,"GRAPH SANDBOX",SCREEN_W/2,SCREEN_H/2-190,TXT_HI,true);
        render_text(r,f_md,"Bridge · Articulation Point · SCC",SCREEN_W/2,SCREEN_H/2-158,TXT_DIM,true);
        render_text(r,f_sm,"Chon che do do thi:",SCREEN_W/2,SCREEN_H/2-115,TXT_AMB,true);

        for(auto& c:cards){
            // Card background
            Col bg=c.hovered?Col{25,45,90,255}:Col{15,22,50,255};
            Col bd=c.hovered?Col{80,140,255,255}:Col{40,60,110,255};
            SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
            draw_rect(r,c.x,c.y,c.w,c.h,bg);
            draw_rect(r,c.x,c.y,c.w,c.h,bd,false);
            SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);

            int mx2=c.x+c.w/2, my2=c.y+c.h/2;

            if(!c.directed){
                // Draw undirected sample: 3 nodes, edges no arrow
                int nx[3]={mx2-60,mx2,mx2+60};
                int ny[3]={my2-20,my2+30,my2-20};
                Col ec=c.hovered?Col{80,160,255}:Col{50,90,160};
                SDL_SetRenderDrawColor(r,ec.r,ec.g,ec.b,ec.a);
                SDL_RenderDrawLine(r,nx[0],ny[0],nx[1],ny[1]);
                SDL_RenderDrawLine(r,nx[1],ny[1],nx[2],ny[2]);
                SDL_RenderDrawLine(r,nx[0],ny[0],nx[2],ny[2]);
                Col nc={45,110,210};
                for(int i=0;i<3;i++){sc(r,nc);fill_circle(r,nx[i],ny[i],10);}

                render_text(r,f_md,"Vo huong",mx2,c.y+c.h-50,TXT_HI,true);
                render_text(r,f_sm,"Bridge + Art.Point",mx2,c.y+c.h-30,TXT_DIM,true);
                render_text(r,f_sm,"(Tarjan)",mx2,c.y+c.h-14,TXT_DIM,true);
            } else {
                // Draw directed sample: 3 nodes, arrows
                int nx[3]={mx2-55,mx2+10,mx2+55};
                int ny[3]={my2-20,my2+30,my2-20};
                Col ec=c.hovered?Col{255,160,60}:Col{160,100,40};
                SDL_SetRenderDrawColor(r,ec.r,ec.g,ec.b,ec.a);
                // Draw lines with arrowheads
                auto arr=[&](int ax,int ay,int bx,int by){
                    float dx=bx-ax,dy=by-ay,len=sqrtf(dx*dx+dy*dy);
                    if(len<1)return;
                    float ux=dx/len,uy=dy/len;
                    int sx=(int)(ax+ux*12),sy=(int)(ay+uy*12);
                    int ex=(int)(bx-ux*14),ey=(int)(by-uy*14);
                    SDL_RenderDrawLine(r,sx,sy,ex,ey);
                    draw_arrowhead(r,(float)sx,(float)sy,(float)ex,(float)ey,8);
                };
                arr(nx[0],ny[0],nx[1],ny[1]);
                arr(nx[1],ny[1],nx[2],ny[2]);
                arr(nx[2],ny[2],nx[0],ny[0]);
                // Nodes with SCC colors
                Col scc_cols[]={{80,180,255},{255,140,60},{80,220,120}};
                for(int i=0;i<3;i++){sc(r,scc_cols[i]);fill_circle(r,nx[i],ny[i],10);}

                render_text(r,f_md,"Co huong",mx2,c.y+c.h-50,TXT_HI,true);
                render_text(r,f_sm,"Strongly Connected",mx2,c.y+c.h-30,TXT_DIM,true);
                render_text(r,f_sm,"Components (Kosaraju)",mx2,c.y+c.h-14,TXT_DIM,true);
            }
        }

        // Footer hint
        render_text(r,f_sm,"Click de chon che do",SCREEN_W/2,SCREEN_H-30,TXT_DIM,true);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// SANDBOX APP
// ─────────────────────────────────────────────────────────────────────────────
class App {
public:
    SDL_Window*   win=nullptr;
    SDL_Renderer* sdl_r=nullptr;
    Renderer      rdr;
    Graph         g;
    StepAnimator  anim;
    InfoPanel     info;

    bool directed=false;
    Mode mode=Mode::ADD_NODE;
    bool result_shown=false;
    bool running=true;
    bool back_to_menu=false;  // signal ra ngoài

    // Drag (ADD_EDGE)
    bool dragging=false;
    int drag_from=-1, drag_mx=0, drag_my=0;

    int hover_node=-1, hover_edge=-1;

    std::deque<LogEntry> logs;

    Slider speed_slider;

    std::vector<Button> btns;
    enum BtnId {
        BTN_ADD_NODE=0,BTN_ADD_EDGE,BTN_DELETE,
        BTN_SUBMIT,BTN_RUN_DFS,BTN_RUN_BFS,
        BTN_CLEAR_RESULT,BTN_RESET,
        BTN_TOGGLE_INFO,BTN_SKIP_ANIM,
        BTN_BACK,
        _BTN_COUNT
    };

    // ── Init ──────────────────────────────────────────────────────────────────
    bool init(SDL_Renderer* r,bool is_directed){
        sdl_r=r; directed=is_directed;
        if(!rdr.init(r,"assets/font.ttf")) return false;
        g.directed=is_directed;
        build_toolbar(); build_info_panel();
        speed_slider={SCREEN_W-185,SCREEN_H-42,160,20,0.3f,false,"Speed"};
        log("Graph Sandbox — "+(directed?std::string("DIRECTED (Kosaraju SCC)"):std::string("UNDIRECTED (Tarjan)")),TXT_GRN);
        log("N=Node  E=Edge  D=Del  ENTER=Run  I=Info  ESC=Back",TXT_DIM);
        return true;
    }

    void reset_for_new_session(bool is_directed){
        g.nodes.clear();g.edges.clear();g.adj.clear();g.radj.clear();
        g.directed=is_directed; directed=is_directed;
        anim.reset(); result_shown=false; back_to_menu=false;
        logs.clear();
        build_toolbar(); build_info_panel();
        log("Graph Sandbox — "+(directed?std::string("DIRECTED"):std::string("UNDIRECTED")),TXT_GRN);
        log("N=Node  E=Edge  D=Del  ENTER=Run  I=Info  ESC=Back",TXT_DIM);
    }

    void build_toolbar(){
        btns.resize(_BTN_COUNT);
        int tx=8,ty=60,bw=120,bh=32,gap=5;
        auto mk=[&](int id,const std::string& lbl){
            btns[id]={tx,ty,bw,bh,lbl};
            ty+=bh+gap;
        };
        mk(BTN_ADD_NODE,"Add Node");
        mk(BTN_ADD_EDGE,"Add Edge");
        mk(BTN_DELETE,  "Delete");
        ty+=6;
        mk(BTN_SUBMIT,  directed?"Run Kosaraju":"Run Tarjan");
        mk(BTN_RUN_DFS, "Animate DFS");
        mk(BTN_RUN_BFS, "Animate BFS");
        mk(BTN_CLEAR_RESULT,"Clear Result");
        ty+=6;
        mk(BTN_RESET,   "Reset Graph");
        ty+=6;
        mk(BTN_TOGGLE_INFO,"Info Panel");
        mk(BTN_SKIP_ANIM,  "Skip Anim");
        ty+=6;
        // Back to menu — đặt gần cuối toolbar
        btns[BTN_BACK]={tx,SCREEN_H-50,bw,bh,"< Back to Menu"};

        btns[BTN_ADD_NODE].active=true;
        btns[BTN_SKIP_ANIM].disabled=true;
        for(auto& b:btns) b.active_col=BTN_ACT;
        btns[BTN_SUBMIT].active_col={50,130,60};
        btns[BTN_RESET].active_col ={120,40,40};
        btns[BTN_DELETE].active_col={130,45,45};
        btns[BTN_BACK].active_col  ={40,40,80};
    }

    void build_info_panel(){
        int pw=280,ph=SCREEN_H-120; // chừa 120px dưới cho speed + anim bar
        info.set_bounds(SCREEN_W-pw-8,58,pw,ph);
        info.init_tabs(directed);
    }

    void set_mode(Mode m){
        mode=m;
        btns[BTN_ADD_NODE].active=(m==Mode::ADD_NODE);
        btns[BTN_ADD_EDGE].active=(m==Mode::ADD_EDGE);
        btns[BTN_DELETE].active  =(m==Mode::DELETE);
        dragging=false; drag_from=-1;
    }

    void log(const std::string& msg,Col c=TXT_HI){
        logs.push_front({msg,c});
        if((int)logs.size()>8) logs.pop_back();
    }

    // ── Run ───────────────────────────────────────────────────────────────────
    void run_frame(float dt,SDL_Event* events,int n_events){
        for(int i=0;i<n_events;i++) handle_event(events[i]);
        update(dt);
        render();
    }

    // ── Update ────────────────────────────────────────────────────────────────
    void update(float dt){
        if(anim.running){
            anim.speed=speed_slider.mapped(0.3f,12.f);
            if(anim.update(dt)){
                auto* s=anim.current();
                if(s) log(s->log,TXT_HI);
            }
            btns[BTN_SKIP_ANIM].disabled=!anim.running;
        }
        for(auto& nd:g.nodes) if(nd.pulse>0) nd.pulse=fmodf(nd.pulse+dt*2.f,1.f);
        for(auto& e :g.edges) if(e.pulse>0)  e.pulse=fmodf(e.pulse+dt*2.f,1.f);
    }

    // ── Events ────────────────────────────────────────────────────────────────
    void handle_event(SDL_Event& ev){
        int mx,my; SDL_GetMouseState(&mx,&my);

        if(ev.type==SDL_WINDOWEVENT&&ev.window.event==SDL_WINDOWEVENT_RESIZED){
            SCREEN_W=ev.window.data1; SCREEN_H=ev.window.data2;
            build_toolbar(); build_info_panel();
            speed_slider.x=SCREEN_W-185; speed_slider.y=SCREEN_H-42;
        }
        if(ev.type==SDL_MOUSEMOTION){
            for(auto& b:btns) b.update_hover(ev.motion.x,ev.motion.y);
            speed_slider.handle_move(ev.motion.x);
            update_hover(ev.motion.x,ev.motion.y);
            if(dragging){drag_mx=ev.motion.x;drag_my=ev.motion.y;}
        }
        if(ev.type==SDL_MOUSEBUTTONDOWN&&ev.button.button==SDL_BUTTON_LEFT){
            int ex=ev.button.x,ey=ev.button.y;
            if(speed_slider.handle_down(ex,ey)) return;
            for(int i=0;i<_BTN_COUNT;i++)
                if(!btns[i].disabled&&btns[i].contains(ex,ey)){on_btn(i);return;}
            if(info.in_panel(ex,ey)){info.on_click(ex,ey);return;}
            on_canvas_down(ex,ey);
        }
        if(ev.type==SDL_MOUSEBUTTONUP&&ev.button.button==SDL_BUTTON_LEFT){
            speed_slider.handle_up();
            if(dragging) on_canvas_up(ev.button.x,ev.button.y);
        }
        if(ev.type==SDL_KEYDOWN) on_key(ev.key.keysym.sym);
    }

    void update_hover(int mx,int my){
        hover_node=-1; hover_edge=-1;
        for(int i=0;i<(int)g.nodes.size();i++){
            float dx=mx-g.nodes[i].x,dy=my-g.nodes[i].y;
            if(dx*dx+dy*dy<=(NODE_R+4)*(NODE_R+4)){hover_node=i;return;}
        }
        for(int i=0;i<(int)g.edges.size();i++){
            auto& e=g.edges[i];
            float ax=g.nodes[e.u].x,ay=g.nodes[e.u].y;
            float bx=g.nodes[e.v].x,by=g.nodes[e.v].y;
            float px=mx-ax,py=my-ay,ex2=bx-ax,ey2=by-ay;
            float len2=ex2*ex2+ey2*ey2; if(len2<1)continue;
            float t=std::clamp((px*ex2+py*ey2)/len2,0.f,1.f);
            float cx2=ax+t*ex2-mx,cy2=ay+t*ey2-my;
            if(cx2*cx2+cy2*cy2<=64){hover_edge=i;return;}
        }
    }

    void on_canvas_down(int mx,int my){
        if(mx<140) return;
        if(info.in_panel(mx,my)) return;
        switch(mode){
        case Mode::ADD_NODE:
            if(hover_node<0){
                int id=g.add_node((float)mx,(float)my);
                log("Added node "+std::to_string(id),TXT_GRN);
                result_shown=false;
            }
            break;
        case Mode::ADD_EDGE:
            if(hover_node>=0){drag_from=hover_node;dragging=true;drag_mx=mx;drag_my=my;}
            break;
        case Mode::DELETE:
            if(hover_node>=0){
                log("Deleted node "+std::to_string(hover_node),TXT_RED);
                g.remove_node(hover_node); hover_node=-1; result_shown=false;
            } else if(hover_edge>=0){
                auto& e=g.edges[hover_edge];
                log("Deleted edge ("+std::to_string(e.u)+"->"+std::to_string(e.v)+")",TXT_RED);
                g.remove_edge(hover_edge); hover_edge=-1; result_shown=false;
            }
            break;
        }
    }

    void on_canvas_up(int mx,int my){
        if(!dragging){return;}
        dragging=false;
        if(mode!=Mode::ADD_EDGE){drag_from=-1;return;}
        update_hover(mx,my);
        if(hover_node>=0&&hover_node!=drag_from){
            int ei=g.add_edge(drag_from,hover_node);
            if(ei>=0) log("Added edge ("+std::to_string(drag_from)+"->"+std::to_string(hover_node)+")",TXT_GRN);
            else      log("Edge already exists.",TXT_DIM);
        }
        drag_from=-1;
    }

    void on_btn(int id){
        switch(id){
        case BTN_ADD_NODE: set_mode(Mode::ADD_NODE); log("Mode: Add Node",TXT_DIM); break;
        case BTN_ADD_EDGE: set_mode(Mode::ADD_EDGE); log("Mode: Add Edge (drag)",TXT_DIM); break;
        case BTN_DELETE:   set_mode(Mode::DELETE);   log("Mode: Delete",TXT_RED);  break;

        case BTN_SUBMIT:
            if(g.nodes.empty()){log("Graph is empty.",TXT_RED);break;}
            if(directed){ g.run_kosaraju(); log("Kosaraju: "+std::to_string(g.num_scc)+" SCC found",TXT_AMB); }
            else         { g.run_tarjan();
                int na=0,nb=0;
                for(auto& nd:g.nodes) if(nd.is_articulation) na++;
                for(auto& e:g.edges)  if(e.is_bridge) nb++;
                log("Tarjan: "+std::to_string(na)+" art.points, "+std::to_string(nb)+" bridges",TXT_AMB);
            }
            result_shown=true;
            break;

        case BTN_RUN_DFS:
            if(g.nodes.empty()){log("Graph is empty.",TXT_RED);break;}
            if(result_shown==false) on_btn(BTN_SUBMIT);
            g.build_dfs_steps(0);
            anim.reset();
            for(auto& s:g.dfs_steps) anim.steps.push_back({s.u,s.edge_idx,s.v,s.backtrack,s.desc});
            anim.start(); btns[BTN_SKIP_ANIM].disabled=false;
            log("DFS animation started.",TXT_GRN); break;

        case BTN_RUN_BFS:
            if(g.nodes.empty()){log("Graph is empty.",TXT_RED);break;}
            if(!result_shown) on_btn(BTN_SUBMIT);
            g.build_bfs_steps(0);
            anim.reset();
            for(auto& s:g.bfs_steps) anim.steps.push_back({s.u,s.edge_idx,s.v,false,s.desc});
            anim.start(); btns[BTN_SKIP_ANIM].disabled=false;
            log("BFS animation started.",TXT_GRN); break;

        case BTN_CLEAR_RESULT:
            g.clear_analysis(); anim.reset(); result_shown=false;
            btns[BTN_SKIP_ANIM].disabled=true;
            log("Cleared.",TXT_DIM); break;

        case BTN_RESET:
            g.nodes.clear();g.edges.clear();g.adj.clear();g.radj.clear();
            g.adj.resize(0);g.radj.resize(0);
            anim.reset(); result_shown=false;
            btns[BTN_SKIP_ANIM].disabled=true;
            log("Graph reset.",TXT_RED); break;

        case BTN_TOGGLE_INFO:
            info.visible=!info.visible;
            btns[BTN_TOGGLE_INFO].active=info.visible; break;

        case BTN_SKIP_ANIM:
            anim.skip_to_end(); btns[BTN_SKIP_ANIM].disabled=true;
            log("Animation skipped.",TXT_DIM); break;

        case BTN_BACK:
            back_to_menu=true; break;
        }
    }

    void on_key(SDL_Keycode k){
        if(k==SDLK_n) set_mode(Mode::ADD_NODE);
        if(k==SDLK_e) set_mode(Mode::ADD_EDGE);
        if(k==SDLK_d) set_mode(Mode::DELETE);
        if(k==SDLK_RETURN||k==SDLK_SPACE){
            if(anim.running) anim.skip_to_end();
            else on_btn(BTN_SUBMIT);
        }
        if(k==SDLK_i){info.visible=!info.visible;btns[BTN_TOGGLE_INFO].active=info.visible;}
        if(k==SDLK_r) on_btn(BTN_RESET);
        if(k==SDLK_ESCAPE) back_to_menu=true;
    }

    // ── Render ────────────────────────────────────────────────────────────────
    void render(){
        // BG + dot grid
        sc(sdl_r,BG); SDL_RenderClear(sdl_r);
        SDL_SetRenderDrawBlendMode(sdl_r,SDL_BLENDMODE_BLEND);
        sc(sdl_r,GRID);
        for(int x=0;x<SCREEN_W;x+=36) for(int y=50;y<SCREEN_H-40;y+=36)
            SDL_RenderDrawPoint(sdl_r,x,y);
        SDL_SetRenderDrawBlendMode(sdl_r,SDL_BLENDMODE_NONE);

        // Edges
        for(int i=0;i<(int)g.edges.size();i++)
            rdr.draw_edge(g,i,(result_shown||anim.running)?&anim:nullptr,result_shown);

        // Drag preview
        if(dragging&&drag_from>=0)
            rdr.draw_drag_edge((int)g.nodes[drag_from].x,(int)g.nodes[drag_from].y,
                               drag_mx,drag_my,directed);

        // Nodes
        for(int i=0;i<(int)g.nodes.size();i++)
            rdr.draw_node(g,i,hover_node,-1,
                (anim.running||anim.done)?&anim:nullptr,result_shown);

        // ── Toolbar ──
        rdr.panel(0,0,138,SCREEN_H,{10,14,30,245});
        render_text(sdl_r,rdr.f_sm,"GRAPH",69,16,TXT_AMB,true);
        std::string mode_badge=directed?"DIRECTED":"UNDIRECTED";
        Col mbc=directed?TXT_AMB:TXT_GRN;
        render_text(sdl_r,rdr.f_sm,mode_badge,69,32,mbc,true);
        for(auto& b:btns) b.draw(sdl_r,rdr.f_md);

        // ── Top bar ──
        rdr.panel(140,0,SCREEN_W-140,46,{8,12,28,240});
        std::string mode_str=mode==Mode::ADD_NODE?"ADD NODE":
                             mode==Mode::ADD_EDGE?"ADD EDGE (drag)":"DELETE";
        render_text(sdl_r,rdr.f_md,"Mode: "+mode_str,155,15,TXT_AMB);
        render_text(sdl_r,rdr.f_sm,
            std::to_string(g.nodes.size())+" nodes  "+std::to_string(g.edges.size())+" edges",
            155,33,TXT_DIM);

        // Result summary — cố định góc phải, không phụ thuộc info panel
        if(result_shown){
            std::string rs;
            if(directed) rs="SCC: "+std::to_string(g.num_scc)+" components";
            else{
                int na=0,nb=0;
                for(auto& nd:g.nodes)if(nd.is_articulation)na++;
                for(auto& e:g.edges) if(e.is_bridge)nb++;
                rs="Art.Points: "+std::to_string(na)+"   Bridges: "+std::to_string(nb);
            }
            // Luôn cố định ở góc phải topbar, không dịch theo info panel
            render_text(sdl_r,rdr.f_md,rs,SCREEN_W-20,23,TXT_AMB,false,true);
        }

        // Shortcut hint
        render_text(sdl_r,rdr.f_sm,"N=Node  E=Edge  D=Del  I=Info  ENTER=Run  ESC=Menu",
            SCREEN_W/2,35,TXT_DIM,true);

        // ── Anim bar — ngắn lại, chỉ chiếm nửa trái màn hình ──
        if(anim.running||(anim.done&&!anim.steps.empty())){
            int bx=140, by=SCREEN_H-44;
            int bw=(SCREEN_W-140)/2 - 20; // trừ thêm margin để progress không tràn
            rdr.panel(bx,by,bw,38,{8,14,35,230});
            auto* s=anim.current();
            render_text(sdl_r,rdr.f_md,s?s->log:"Animation complete",bx+10,by+12,TXT_HI);
            rdr.progress_bar(bx+8,by+26,bw-16,6,anim.progress()); // padding đều 2 bên
        }

        // Speed slider — cố định góc phải dưới
        speed_slider.x=SCREEN_W-185; speed_slider.y=SCREEN_H-42; speed_slider.w=160;
        render_text(sdl_r,rdr.f_sm,"Anim Speed",SCREEN_W-240,SCREEN_H-30,TXT_DIM);
        speed_slider.draw(sdl_r,rdr.f_sm);

        // Log — đẩy lên cao hơn để không bị legend đè
        {
            int lx=145, ly=SCREEN_H-130; // lên cao hơn trước (trước là -50)
            if(anim.running||(!anim.steps.empty()&&anim.done)) ly-=42;
            for(int i=0;i<(int)logs.size()&&i<4;i++){
                Uint8 a=(Uint8)(255*(1.f-i*0.22f));
                Col c=logs[i].color; c.a=a;
                render_text(sdl_r,rdr.f_sm,logs[i].msg,lx,ly-i*15,c);
            }
        }

        // ── Legend — ngay trên speed slider, góc dưới trái ──
        if(result_shown){
            int lh_box=70, lx=145, ly=SCREEN_H-lh_box-52;
            if(!directed){
                rdr.panel(lx,ly,185,lh_box,{10,14,30,220});
                SDL_SetRenderDrawBlendMode(sdl_r,SDL_BLENDMODE_BLEND);
                sc(sdl_r,NODE_ART); fill_circle(sdl_r,lx+14,ly+18,8);
                SDL_SetRenderDrawBlendMode(sdl_r,SDL_BLENDMODE_NONE);
                render_text(sdl_r,rdr.f_sm,"Articulation Point",lx+28,ly+18,TXT_AMB);
                sc(sdl_r,EDGE_BRG); thick_line(sdl_r,lx+6,ly+46,lx+22,ly+46,3);
                render_text(sdl_r,rdr.f_sm,"Bridge",lx+28,ly+46,EDGE_BRG);
            } else {
                int n_scc=std::min(g.num_scc,5);
                int box_h=14+n_scc*20+6;
                ly=SCREEN_H-box_h-52;
                rdr.panel(lx,ly,185,box_h,{10,14,30,220});
                for(int i=0;i<n_scc;i++){
                    Col sc2=SCC_COLORS[i%NUM_SCC_COLORS];
                    sc(sdl_r,sc2); fill_circle(sdl_r,lx+12,ly+14+i*20,7);
                    render_text(sdl_r,rdr.f_sm,"SCC "+std::to_string(i),lx+26,ly+14+i*20,sc2);
                }
                if(g.num_scc>5) render_text(sdl_r,rdr.f_sm,"...",lx+12,ly+14+5*20,TXT_DIM);
            }
        }

        // ── Info panel ──
        info.draw(sdl_r,rdr.f_md,rdr.f_sm,rdr.f_mono,g);

        SDL_RenderPresent(sdl_r);
    }
};