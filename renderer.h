#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include "graph.h"
#include "animator.h"

// ─── Palette ──────────────────────────────────────────────────────────────────
struct Col { Uint8 r,g,b,a=255; };

static constexpr Col BG       = {8,  10, 18};
static constexpr Col GRID     = {18, 24, 40};
static constexpr Col PANEL    = {12, 16, 32, 230};
static constexpr Col PANEL_BD = {40, 55, 100};

static constexpr Col NODE_DEF = {45, 110, 210};
static constexpr Col NODE_VIS = {55, 190, 140};
static constexpr Col NODE_ART = {255,140,  30};
static constexpr Col NODE_HOV = {120,180, 255};
static constexpr Col NODE_SEL = {255,215,  50};

static constexpr Col EDGE_DEF = {50,  70, 130};
static constexpr Col EDGE_TRE = {60, 160, 255};
static constexpr Col EDGE_BRG = {255, 65,  80};
static constexpr Col EDGE_DRG = {100,180, 255, 160};

static constexpr Col TXT_HI   = {210,225,255};
static constexpr Col TXT_DIM  = { 90,105,140};
static constexpr Col TXT_AMB  = {255,185, 50};
static constexpr Col TXT_GRN  = { 60,215,110};
static constexpr Col TXT_RED  = {255, 70, 70};

static constexpr Col BTN_NORM = {20, 35, 75};
static constexpr Col BTN_HOV  = {35, 60,130};
static constexpr Col BTN_ACT  = {40, 90,180};
static constexpr Col BTN_BD   = {55, 80,160};

static constexpr int NODE_R   = 18;

// Bảng màu SCC (tối đa 10 màu, lặp vòng nếu nhiều hơn)
static const Col SCC_COLORS[] = {
    {80, 180, 255}, {255,140, 60}, {80, 220,120},
    {220, 80,200}, {255,220, 50}, {80, 220,210},
    {200,100,100}, {160,220, 80}, {180,100,255}, {255,160,100}
};
static constexpr int NUM_SCC_COLORS = 10;

// ─── Helpers ──────────────────────────────────────────────────────────────────
inline void sc(SDL_Renderer* r,Col c){SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);}

inline Col lerp(Col a,Col b,float t){
    t=std::clamp(t,0.f,1.f);
    return{(Uint8)(a.r+(b.r-a.r)*t),(Uint8)(a.g+(b.g-a.g)*t),
           (Uint8)(a.b+(b.b-a.b)*t),(Uint8)(a.a+(b.a-a.a)*t)};
}

inline void fill_circle(SDL_Renderer* r,int cx,int cy,int rad){
    for(int dy=-rad;dy<=rad;dy++){
        int dx=(int)sqrtf((float)(rad*rad-dy*dy));
        SDL_RenderDrawLine(r,cx-dx,cy+dy,cx+dx,cy+dy);
    }
}

inline void draw_circle(SDL_Renderer* r,int cx,int cy,int rad){
    int x=rad,y=0,err=0;
    while(x>=y){
        SDL_RenderDrawPoint(r,cx+x,cy+y);SDL_RenderDrawPoint(r,cx+y,cy+x);
        SDL_RenderDrawPoint(r,cx-y,cy+x);SDL_RenderDrawPoint(r,cx-x,cy+y);
        SDL_RenderDrawPoint(r,cx-x,cy-y);SDL_RenderDrawPoint(r,cx-y,cy-x);
        SDL_RenderDrawPoint(r,cx+y,cy-x);SDL_RenderDrawPoint(r,cx+x,cy-y);
        if(err<=0){y++;err+=2*y+1;}else{x--;err-=2*x+1;}
    }
}

inline void thick_line(SDL_Renderer* r,int x0,int y0,int x1,int y1,int w){
    float dx=x1-x0,dy=y1-y0,len=sqrtf(dx*dx+dy*dy);
    if(len<1)return;
    float nx=-dy/len,ny=dx/len;
    for(int i=-(w/2);i<=w/2;i++)
        SDL_RenderDrawLine(r,(int)(x0+nx*i),(int)(y0+ny*i),(int)(x1+nx*i),(int)(y1+ny*i));
}

// ── Vẽ đầu mũi tên tại điểm (tip) hướng từ (fx,fy) → (tip) ─────────────────
inline void draw_arrowhead(SDL_Renderer* r,float fx,float fy,float tx,float ty,int size=10){
    float dx=tx-fx, dy=ty-fy, len=sqrtf(dx*dx+dy*dy);
    if(len<1)return;
    dx/=len; dy/=len;
    // Hai cánh của mũi tên
    float ax=tx-dx*size+dy*(size*0.5f);
    float ay=ty-dy*size-dx*(size*0.5f);
    float bx=tx-dx*size-dy*(size*0.5f);
    float by=ty-dy*size+dx*(size*0.5f);
    SDL_RenderDrawLine(r,(int)tx,(int)ty,(int)ax,(int)ay);
    SDL_RenderDrawLine(r,(int)tx,(int)ty,(int)bx,(int)by);
    SDL_RenderDrawLine(r,(int)ax,(int)ay,(int)bx,(int)by);
}

inline void draw_rect(SDL_Renderer* r,int x,int y,int w,int h,Col c,bool fill=true){
    sc(r,c);
    SDL_Rect rc={x,y,w,h};
    if(fill)SDL_RenderFillRect(r,&rc);
    else    SDL_RenderDrawRect(r,&rc);
}

inline void render_text(SDL_Renderer* r,TTF_Font* f,const std::string& t,
                        int x,int y,Col c,bool center=false,bool right=false){
    if(!f||t.empty())return;
    SDL_Color sc2={c.r,c.g,c.b,c.a};
    SDL_Surface* s=TTF_RenderUTF8_Blended(f,t.c_str(),sc2);
    if(!s)return;
    SDL_Texture* tx=SDL_CreateTextureFromSurface(r,s);
    int tw=s->w,th=s->h; SDL_FreeSurface(s);
    if(!tx)return;
    int rx=center?x-tw/2:(right?x-tw:x);
    SDL_Rect dst={rx,y-th/2,tw,th};
    SDL_RenderCopy(r,tx,nullptr,&dst);
    SDL_DestroyTexture(tx);
}

// ─── Button ───────────────────────────────────────────────────────────────────
struct Button {
    int x,y,w,h;
    std::string label;
    bool active=false, hovered=false, disabled=false;
    Col active_col=BTN_ACT;

    bool contains(int mx,int my)const{return mx>=x&&mx<x+w&&my>=y&&my<y+h;}

    void draw(SDL_Renderer* r,TTF_Font* f)const{
        Col bg  =disabled?Col{15,20,40}:(active?active_col:(hovered?BTN_HOV:BTN_NORM));
        Col bdc =disabled?Col{30,40,65}:(active?Col{80,140,255}:BTN_BD);
        Col tc  =disabled?TXT_DIM:(active?Col{255,255,255}:TXT_HI);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        draw_rect(r,x,y,w,h,bg);
        draw_rect(r,x,y,w,h,bdc,false);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
        render_text(r,f,label,x+w/2,y+h/2,tc,true);
    }
    void update_hover(int mx,int my){hovered=!disabled&&contains(mx,my);}
};

// ─── Slider ───────────────────────────────────────────────────────────────────
struct Slider {
    int x,y,w,h;
    float value=0.5f;
    bool dragging=false;
    std::string label;
    float mapped(float lo,float hi)const{return lo+value*(hi-lo);}
    void draw(SDL_Renderer* r,TTF_Font* f)const{
        draw_rect(r,x,y+h/2-3,w,6,{30,45,80});
        draw_rect(r,x,y+h/2-3,(int)(w*value),6,{55,110,220});
        int tx=x+(int)(w*value);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        sc(r,{80,140,255,80}); fill_circle(r,tx,y+h/2,9);
        sc(r,{120,180,255});   draw_circle(r,tx,y+h/2,9);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
        if(!label.empty()) render_text(r,f,label,x+w/2,y+h+8,TXT_DIM,true);
    }
    bool handle_down(int mx,int my){
        if(mx>=x-10&&mx<x+w+10&&my>=y&&my<y+h){dragging=true;_upd(mx);return true;}
        return false;
    }
    void handle_up(){dragging=false;}
    void handle_move(int mx){if(dragging)_upd(mx);}
    void _upd(int mx){value=std::clamp((float)(mx-x)/w,0.f,1.f);}
};

// ─── Main Renderer ────────────────────────────────────────────────────────────
class Renderer {
public:
    SDL_Renderer* r=nullptr;
    TTF_Font* f_lg=nullptr, *f_md=nullptr, *f_sm=nullptr, *f_mono=nullptr;

    bool init(SDL_Renderer* rr,const std::string& fp){
        r=rr;
        f_lg  =TTF_OpenFont(fp.c_str(),20);
        f_md  =TTF_OpenFont(fp.c_str(),15);
        f_sm  =TTF_OpenFont(fp.c_str(),11);
        f_mono=TTF_OpenFont(fp.c_str(),13);
        return f_lg&&f_md&&f_sm&&f_mono;
    }
    void quit(){
        if(f_lg)  TTF_CloseFont(f_lg);
        if(f_md)  TTF_CloseFont(f_md);
        if(f_sm)  TTF_CloseFont(f_sm);
        if(f_mono)TTF_CloseFont(f_mono);
    }

    void panel(int x,int y,int w,int h,Col bg=PANEL,bool border=true){
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        draw_rect(r,x,y,w,h,bg);
        if(border)draw_rect(r,x,y,w,h,PANEL_BD,false);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    }

    void progress_bar(int x,int y,int w2,int h,float t){
        draw_rect(r,x,y,w2,h,{20,30,60});
        draw_rect(r,x,y,(int)(w2*t),h,{55,110,220});
        draw_rect(r,x,y,w2,h,PANEL_BD,false);
    }

    // ── Draw edge (supports directed + SCC color) ─────────────────────────────
    void draw_edge(const Graph& g,int ei,
                   const StepAnimator* anim=nullptr,
                   bool result_shown=false){
        auto& e=g.edges[ei];
        float fx=g.nodes[e.u].x, fy=g.nodes[e.u].y;
        float tx=g.nodes[e.v].x, ty=g.nodes[e.v].y;

        // Rút ngắn điểm đầu/cuối để không chui vào node
        float dx=tx-fx,dy=ty-fy,len=sqrtf(dx*dx+dy*dy);
        if(len<1)return;
        float ux=dx/len,uy=dy/len;
        float r_start=(float)NODE_R+2;
        float r_end  =(float)NODE_R+4; // thêm chỗ cho mũi tên
        float ax=fx+ux*r_start, ay=fy+uy*r_start;
        float bx=tx-ux*r_end,   by=ty-uy*r_end;

        Col c=EDGE_DEF;
        int  w2=2;

        if(result_shown){
            if(!g.directed && e.is_bridge){
                float p=0.5f+0.5f*sinf(e.pulse*6.28f);
                c=lerp(EDGE_BRG,Col{255,120,100},p); w2=4;
            } else if(g.directed){
                // Tô màu cạnh theo SCC của đỉnh nguồn
                int scc=g.nodes[e.u].scc_id;
                if(scc>=0) c=SCC_COLORS[scc%NUM_SCC_COLORS];
                w2=2;
            }
        }
        if(anim && anim->tree_edges.count(ei)){ c=EDGE_TRE; w2=3; }

        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        sc(r,c);
        thick_line(r,(int)ax,(int)ay,(int)bx,(int)by,w2);

        // Mũi tên cho directed
        if(g.directed){
            sc(r,c);
            draw_arrowhead(r,ax,ay,bx,by,10);
        }

        // Bridge label (undirected)
        if(result_shown && !g.directed && e.is_bridge){
            int mx2=(int)(fx+tx)/2, my2=(int)(fy+ty)/2;
            render_text(r,f_sm,"BRIDGE",mx2,my2-12,EDGE_BRG,true);
        }
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    }

    // ── Draw drag preview (with arrowhead if directed) ────────────────────────
    void draw_drag_edge(int x0,int y0,int x1,int y1,bool directed=false){
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        sc(r,EDGE_DRG);
        thick_line(r,x0,y0,x1,y1,2);
        if(directed) draw_arrowhead(r,(float)x0,(float)y0,(float)x1,(float)y1,10);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    }

    // ── Draw node (supports SCC color for directed) ───────────────────────────
    void draw_node(const Graph& g,int ni,
                   int hover_node,int sel_node,
                   const StepAnimator* anim=nullptr,
                   bool result_shown=false){
        auto& nd=g.nodes[ni];
        int cx=(int)nd.x,cy=(int)nd.y,rad=NODE_R;

        bool is_hover =(ni==hover_node);
        bool is_sel   =(ni==sel_node);
        bool is_vis   =anim&&anim->visited_nodes.count(ni);
        bool is_active=anim&&anim->current()&&anim->current()->active_node==ni;
        bool is_art   =result_shown&&!g.directed&&nd.is_articulation;
        bool has_scc  =result_shown&&g.directed&&nd.scc_id>=0;

        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        // Glow: articulation point
        if(is_art){
            for(int g2=rad+16;g2>rad;g2--){
                Uint8 a=(Uint8)(70.f*(rad+16-g2)/16.f);
                SDL_SetRenderDrawColor(r,NODE_ART.r,NODE_ART.g,NODE_ART.b,a);
                draw_circle(r,cx,cy,g2);
            }
        }
        // Glow: SCC color ring
        if(has_scc){
            Col sc2=SCC_COLORS[nd.scc_id%NUM_SCC_COLORS];
            for(int g2=rad+14;g2>rad;g2--){
                Uint8 a=(Uint8)(80.f*(rad+14-g2)/14.f);
                SDL_SetRenderDrawColor(r,sc2.r,sc2.g,sc2.b,a);
                draw_circle(r,cx,cy,g2);
            }
        }
        // Glow: active (animation)
        if(is_active){
            for(int g2=rad+12;g2>rad;g2--){
                Uint8 a=(Uint8)(100.f*(rad+12-g2)/12.f);
                SDL_SetRenderDrawColor(r,120,200,255,a);
                draw_circle(r,cx,cy,g2);
            }
        }
        // Glow: hover/select
        if(is_hover||is_sel){
            Col gc=is_sel?NODE_SEL:NODE_HOV;
            for(int g2=rad+8;g2>rad;g2--){
                Uint8 a=(Uint8)(80.f*(rad+8-g2)/8.f);
                SDL_SetRenderDrawColor(r,gc.r,gc.g,gc.b,a);
                draw_circle(r,cx,cy,g2);
            }
        }
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);

        // Fill color
        Col fill=NODE_DEF;
        if(is_art)       fill=NODE_ART;
        else if(has_scc) fill=lerp(NODE_DEF,SCC_COLORS[nd.scc_id%NUM_SCC_COLORS],0.55f);
        else if(is_vis)  fill=NODE_VIS;
        if(is_sel)       fill=lerp(fill,NODE_SEL,0.4f);
        if(is_hover)     fill=lerp(fill,NODE_HOV,0.3f);

        sc(r,fill); fill_circle(r,cx,cy,rad);

        // Border
        Col bdc=is_sel?NODE_SEL:(is_hover?NODE_HOV:(is_art?NODE_ART:Col{80,120,200}));
        if(has_scc) bdc=SCC_COLORS[nd.scc_id%NUM_SCC_COLORS];
        sc(r,bdc); draw_circle(r,cx,cy,rad);
        if(is_sel||is_hover) draw_circle(r,cx,cy,rad+2);

        // ID
        render_text(r,f_md,std::to_string(ni),cx,cy,BG,true);

        // SCC badge (directed)
        if(has_scc){
            Col sc2=SCC_COLORS[nd.scc_id%NUM_SCC_COLORS];
            render_text(r,f_sm,"S"+std::to_string(nd.scc_id),cx,cy+rad+9,sc2,true);
        }
        // disc/low (undirected)
        else if(nd.disc>=0&&!g.directed){
            std::string dl="d"+std::to_string(nd.disc)+" l"+std::to_string(nd.low);
            render_text(r,f_sm,dl,cx,cy+rad+9,is_art?TXT_AMB:TXT_DIM,true);
        }
    }
};

extern int SCREEN_W, SCREEN_H;
