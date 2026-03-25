#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "graph.h"
#include "renderer.h"

class InfoPanel {
public:
    bool visible = false;
    int tab = 0; // 0=AdjList  1=AdjMatrix  2=disc/low

    int px, py, pw, ph;

    void set_bounds(int x,int y,int w,int h){ px=x;py=y;pw=w;ph=h; }

    // Tab buttons
    struct TabBtn { int x,y,w,h; std::string label; };
    std::vector<TabBtn> tabs;

    void init_tabs() {
        tabs.clear();
        int bw=80, bh=26, gap=4;
        int tx=px+8, ty=py+8;
        tabs.push_back({tx,          ty,bw,bh,"Adj.List"});
        tabs.push_back({tx+bw+gap,   ty,bw,bh,"Matrix"});
        tabs.push_back({tx+2*(bw+gap),ty,bw+10,bh,"disc/low"});
    }

    void draw(SDL_Renderer* r, TTF_Font* f_md, TTF_Font* f_sm,
              TTF_Font* f_mono, const Graph& g) {
        if (!visible) return;

        // Panel background
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        draw_rect(r, px, py, pw, ph, PANEL);
        draw_rect(r, px, py, pw, ph, PANEL_BD, false);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

        // Tabs
        for (int i = 0; i < (int)tabs.size(); i++) {
            auto& tb = tabs[i];
            bool act = (i == tab);
            Col bg  = act ? BTN_ACT : BTN_NORM;
            Col bdc = act ? Col{80,140,255} : BTN_BD;
            draw_rect(r, tb.x, tb.y, tb.w, tb.h, bg);
            draw_rect(r, tb.x, tb.y, tb.w, tb.h, bdc, false);
            render_text(r, f_sm, tb.label, tb.x+tb.w/2, tb.y+tb.h/2, TXT_HI, true);
        }

        int content_y = py + 44;
        int content_x = px + 10;
        int line_h    = 16;

        int n = g.nodes.size();

        if (tab == 0) {
            // ── Adjacency List ──
            render_text(r, f_md, "Adjacency List", content_x, content_y, TXT_AMB);
            content_y += 22;
            for (int i = 0; i < n && content_y < py+ph-10; i++) {
                std::string line = std::to_string(i) + ": [";
                bool first = true;
                for (auto [v,_] : g.adj[i]) {
                    if (!first) line += ", ";
                    line += std::to_string(v);
                    first = false;
                }
                line += "]";
                Col c = g.nodes[i].is_articulation ? TXT_AMB : TXT_HI;
                render_text(r, f_mono, line, content_x, content_y, c);
                content_y += line_h;
            }
        }
        else if (tab == 1) {
            // ── Adjacency Matrix ──
            render_text(r, f_md, "Adjacency Matrix", content_x, content_y, TXT_AMB);
            content_y += 22;
            if (n == 0) {
                render_text(r, f_sm, "(empty)", content_x, content_y, TXT_DIM);
                return;
            }
            // Build matrix
            std::vector<std::vector<int>> mat(n, std::vector<int>(n,0));
            for (auto& e : g.edges) mat[e.u][e.v] = mat[e.v][e.u] = 1;

            // Cell size adaptive
            int cs = std::min(18, (pw-20) / (n+1));
            int fs = cs - 4;
            if (fs < 6) fs = 6;

            // Header row
            int ox = content_x + cs + 4;
            for (int j = 0; j < n; j++)
                render_text(r, f_sm, std::to_string(j), ox + j*cs + cs/2, content_y, TXT_DIM, true);
            content_y += cs;

            for (int i = 0; i < n && content_y < py+ph-10; i++) {
                render_text(r, f_sm, std::to_string(i), content_x+cs/2, content_y, TXT_DIM, true);
                for (int j = 0; j < n; j++) {
                    int cx2 = ox + j*cs + cs/2;
                    Col c = mat[i][j] ? (g.edges[0].is_bridge && /* rough check */false
                                ? EDGE_BRG : TXT_GRN)
                            : TXT_DIM;
                    // Check if this edge is a bridge
                    if (mat[i][j]) {
                        for (auto& e : g.edges)
                            if ((e.u==i&&e.v==j)||(e.u==j&&e.v==i))
                                c = e.is_bridge ? EDGE_BRG : TXT_GRN;
                    }
                    render_text(r, f_sm, mat[i][j]?"1":"·", cx2, content_y, c, true);
                }
                content_y += cs;
            }
        }
        else if (tab == 2) {
            // ── disc / low table ──
            render_text(r, f_md, "disc[] and low[]", content_x, content_y, TXT_AMB);
            content_y += 22;

            // Header
            int c1=content_x, c2=content_x+38, c3=content_x+80, c4=content_x+128;
            render_text(r, f_sm, "Node", c1, content_y, TXT_DIM);
            render_text(r, f_sm, "disc", c2, content_y, TXT_DIM);
            render_text(r, f_sm, "low",  c3, content_y, TXT_DIM);
            render_text(r, f_sm, "Type", c4, content_y, TXT_DIM);
            content_y += 18;

            for (int i = 0; i < n && content_y < py+ph-10; i++) {
                auto& nd = g.nodes[i];
                Col rc = nd.is_articulation ? TXT_AMB : TXT_HI;

                render_text(r, f_mono, std::to_string(i), c1, content_y, rc);
                if (nd.disc >= 0) {
                    render_text(r, f_mono, std::to_string(nd.disc), c2, content_y, TXT_HI);
                    render_text(r, f_mono, std::to_string(nd.low),  c3, content_y, TXT_HI);
                } else {
                    render_text(r, f_sm, "-", c2, content_y, TXT_DIM);
                    render_text(r, f_sm, "-", c3, content_y, TXT_DIM);
                }
                std::string type = nd.is_articulation ? "ART" : "";
                // Check if any edge is bridge from this node
                for (auto [v,ei]:g.adj[i]) if(g.edges[ei].is_bridge){ type+=(type.empty()?"":"+")+std::string("BRG"); break; }
                render_text(r, f_sm, type, c4, content_y, nd.is_articulation?TXT_AMB:TXT_DIM);
                content_y += line_h;
            }
        }
    }

    void on_click(int mx, int my) {
        if (!visible) return;
        for (int i=0;i<(int)tabs.size();i++)
            if (mx>=tabs[i].x&&mx<tabs[i].x+tabs[i].w&&my>=tabs[i].y&&my<tabs[i].y+tabs[i].h)
                tab=i;
    }

    bool in_panel(int mx, int my) const {
        return visible && mx>=px&&mx<px+pw&&my>=py&&my<py+ph;
    }
};
