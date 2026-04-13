#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "graph.h"
#include "renderer.h"

class InfoPanel {
public:
    bool visible=false;
    int tab=0; // 0=AdjList  1=AdjMatrix  2=disc/low or SCC

    int px,py,pw,ph;

    struct TabBtn{int x,y,w,h;std::string label;};
    std::vector<TabBtn> tabs;

    void set_bounds(int x,int y,int w,int h){px=x;py=y;pw=w;ph=h;}

    void init_tabs(bool directed){
        tabs.clear();
        int bw=80,bh=26,gap=4,tx=px+8,ty=py+8;
        tabs.push_back({tx,           ty,bw,bh,"Adj.List"});
        tabs.push_back({tx+bw+gap,    ty,bw,bh,"Matrix"});
        tabs.push_back({tx+2*(bw+gap),ty,bw+20,bh, directed?"SCC Info":"disc/low"});
    }

    void draw(SDL_Renderer* r,TTF_Font* f_md,TTF_Font* f_sm,
              TTF_Font* f_mono,const Graph& g){
        if(!visible)return;
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        draw_rect(r,px,py,pw,ph,PANEL);
        draw_rect(r,px,py,pw,ph,PANEL_BD,false);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);

        // Tabs
        for(int i=0;i<(int)tabs.size();i++){
            auto& tb=tabs[i]; bool act=(i==tab);
            draw_rect(r,tb.x,tb.y,tb.w,tb.h,act?BTN_ACT:BTN_NORM);
            draw_rect(r,tb.x,tb.y,tb.w,tb.h,act?Col{80,140,255}:BTN_BD,false);
            render_text(r,f_sm,tb.label,tb.x+tb.w/2,tb.y+tb.h/2,TXT_HI,true);
        }

        int cx2=px+10, cy2=py+44, lh=16;
        int n=g.nodes.size();

        if(tab==0){
            // ── Adjacency List ──
            render_text(r,f_md,g.directed?"Adjacency List (directed)":"Adjacency List",cx2,cy2,TXT_AMB);
            cy2+=22;
            for(int i=0;i<n&&cy2<py+ph-10;i++){
                std::string line=std::to_string(i)+": [";
                bool first=true;
                for(auto [v,_]:g.adj[i]){
                    if(!first)line+=", ";
                    line+=std::to_string(v); first=false;
                }
                line+="]";
                Col c=(g.nodes[i].is_articulation&&!g.directed)?TXT_AMB:
                      (g.directed&&g.nodes[i].scc_id>=0)?SCC_COLORS[g.nodes[i].scc_id%NUM_SCC_COLORS]:TXT_HI;
                render_text(r,f_mono,line,cx2,cy2,c);
                cy2+=lh;
            }
        }
        else if(tab==1){
            // ── Adjacency Matrix ──
            render_text(r,f_md,"Adjacency Matrix",cx2,cy2,TXT_AMB); cy2+=22;
            if(!n){render_text(r,f_sm,"(empty)",cx2,cy2,TXT_DIM);return;}
            std::vector<std::vector<int>> mat(n,std::vector<int>(n,0));
            // For directed: mat[u][v]=1 if edge u->v exists
            for(auto& e:g.edges) mat[e.u][e.v]=1;
            if(!g.directed) for(auto& e:g.edges) mat[e.v][e.u]=1;

            int cs=std::min(18,(pw-20)/(n+1));
            int ox=cx2+cs+4;
            // Header
            for(int j=0;j<n;j++)
                render_text(r,f_sm,std::to_string(j),ox+j*cs+cs/2,cy2,TXT_DIM,true);
            cy2+=cs;
            for(int i=0;i<n&&cy2<py+ph-10;i++){
                render_text(r,f_sm,std::to_string(i),cx2+cs/2,cy2,TXT_DIM,true);
                for(int j=0;j<n;j++){
                    Col c=TXT_DIM;
                    if(mat[i][j]){
                        c=TXT_GRN;
                        // Bridge check (undirected)
                        if(!g.directed)
                            for(auto& e:g.edges)
                                if((e.u==i&&e.v==j)||(e.u==j&&e.v==i))
                                    if(e.is_bridge){c=EDGE_BRG;break;}
                        // SCC same component (directed)
                        if(g.directed&&g.nodes[i].scc_id>=0&&g.nodes[i].scc_id==g.nodes[j].scc_id)
                            c=SCC_COLORS[g.nodes[i].scc_id%NUM_SCC_COLORS];
                    }
                    render_text(r,f_sm,mat[i][j]?"1":"·",ox+j*cs+cs/2,cy2,c,true);
                }
                cy2+=cs;
            }
        }
        else if(tab==2){
            if(!g.directed){
                // ── disc/low table ──
                render_text(r,f_md,"disc[] and low[]",cx2,cy2,TXT_AMB); cy2+=22;
                int c1=cx2,c2=cx2+38,c3=cx2+80,c4=cx2+128;
                render_text(r,f_sm,"Node",c1,cy2,TXT_DIM);
                render_text(r,f_sm,"disc",c2,cy2,TXT_DIM);
                render_text(r,f_sm,"low", c3,cy2,TXT_DIM);
                render_text(r,f_sm,"Type",c4,cy2,TXT_DIM); cy2+=18;
                for(int i=0;i<n&&cy2<py+ph-10;i++){
                    auto& nd=g.nodes[i];
                    Col rc=nd.is_articulation?TXT_AMB:TXT_HI;
                    render_text(r,f_mono,std::to_string(i),c1,cy2,rc);
                    if(nd.disc>=0){
                        render_text(r,f_mono,std::to_string(nd.disc),c2,cy2,TXT_HI);
                        render_text(r,f_mono,std::to_string(nd.low), c3,cy2,TXT_HI);
                    } else {
                        render_text(r,f_sm,"-",c2,cy2,TXT_DIM);
                        render_text(r,f_sm,"-",c3,cy2,TXT_DIM);
                    }
                    std::string type=nd.is_articulation?"ART":"";
                    for(auto [v,ei]:g.adj[i])
                        if(g.edges[ei].is_bridge){type+=(type.empty()?"":"+")+std::string("BRG");break;}
                    render_text(r,f_sm,type,c4,cy2,nd.is_articulation?TXT_AMB:TXT_DIM);
                    cy2+=lh;
                }
            } else {
                // ── SCC Info (directed) ──
                render_text(r,f_md,"SCC Components (Kosaraju)",cx2,cy2,TXT_AMB); cy2+=22;
                if(g.num_scc==0){
                    render_text(r,f_sm,"Run Kosaraju first",cx2,cy2,TXT_DIM);
                    return;
                }
                render_text(r,f_sm,"Total SCC: "+std::to_string(g.num_scc),cx2,cy2,TXT_GRN);
                cy2+=20;
                // Group nodes by SCC
                std::vector<std::vector<int>> groups(g.num_scc);
                for(int i=0;i<n;i++) if(g.nodes[i].scc_id>=0) groups[g.nodes[i].scc_id].push_back(i);
                for(int s=0;s<g.num_scc&&cy2<py+ph-10;s++){
                    Col sc2=SCC_COLORS[s%NUM_SCC_COLORS];
                    std::string line="S"+std::to_string(s)+": {";
                    for(int k=0;k<(int)groups[s].size();k++){
                        if(k>0)line+=",";
                        line+=std::to_string(groups[s][k]);
                    }
                    line+="}";
                    render_text(r,f_mono,line,cx2,cy2,sc2);
                    cy2+=lh+2;
                }
            }
        }
    }

    void on_click(int mx,int my){
        if(!visible)return;
        for(int i=0;i<(int)tabs.size();i++)
            if(mx>=tabs[i].x&&mx<tabs[i].x+tabs[i].w&&my>=tabs[i].y&&my<tabs[i].y+tabs[i].h)
                tab=i;
    }

    bool in_panel(int mx,int my)const{
        return visible&&mx>=px&&mx<px+pw&&my>=py&&my<py+ph;
    }
};
