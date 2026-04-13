#pragma once
#include <vector>
#include <set>
#include <stack>
#include <algorithm>
#include <string>
#include <functional>

struct Node {
    int id;
    float x, y;
    int disc=-1, low=-1;
    bool is_articulation=false;
    int  scc_id=-1;
    float pulse=0.f, spawn=0.f;
};

struct Edge {
    int u, v;           // directed: u → v
    bool is_bridge=false;
    float pulse=0.f;
};

class Graph {
public:
    bool directed=false;

    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::vector<std::vector<std::pair<int,int>>> adj;  // adj[u]={v,eidx}
    std::vector<std::vector<int>> radj;                // reverse (directed)

    int num_scc=0;

    int add_node(float x,float y){
        int id=nodes.size();
        Node nd{}; nd.id=id; nd.x=x; nd.y=y;
        nodes.push_back(nd); adj.push_back({}); radj.push_back({});
        return id;
    }

    int add_edge(int u,int v){
        if(u==v) return -1;
        int n=(int)nodes.size();
        if(u<0||v<0||u>=n||v>=n) return -1;
        for(auto [nv,_]:adj[u]) if(nv==v) return -1;
        int idx=edges.size();
        edges.push_back({u,v,false,0.f});
        adj[u].push_back({v,idx});
        radj[v].push_back(u);
        if(!directed){ adj[v].push_back({u,idx}); radj[u].push_back(v); }
        return idx;
    }

    void remove_node(int id){
        if(id<0||id>=(int)nodes.size()) return;
        std::vector<int> to_rm;
        for(int i=0;i<(int)edges.size();i++)
            if(edges[i].u==id||edges[i].v==id) to_rm.push_back(i);
        std::sort(to_rm.rbegin(),to_rm.rend());
        for(int ei:to_rm) _rm_edge(ei);
        nodes.erase(nodes.begin()+id);
        for(int i=id;i<(int)nodes.size();i++) nodes[i].id=i;
        adj.erase(adj.begin()+id);
        radj.erase(radj.begin()+id);
        for(auto& row:adj)  for(auto& [v,_]:row)  if(v>id) v--;
        for(auto& row:radj) for(auto& v:row)       if(v>id) v--;
        for(auto& e:edges){ if(e.u>id)e.u--; if(e.v>id)e.v--; }
    }

    void remove_edge(int idx){ _rm_edge(idx); }

    void clear_analysis(){
        for(auto& nd:nodes){nd.disc=-1;nd.low=-1;nd.is_articulation=false;nd.scc_id=-1;nd.pulse=0.f;}
        for(auto& e:edges){e.is_bridge=false;e.pulse=0.f;}
        num_scc=0;
    }

    // ── Tarjan undirected: bridge + articulation ──────────────────────────────
    void run_tarjan(){
        clear_analysis();
        int n=nodes.size(); if(!n) return;
        std::vector<bool> vis(n,false); int timer=0;
        std::function<void(int,int)> dfs=[&](int u,int pe){
            vis[u]=true; nodes[u].disc=nodes[u].low=timer++;
            int children=0;
            for(auto [v,eidx]:adj[u]){
                if(!vis[v]){
                    children++; dfs(v,eidx);
                    nodes[u].low=std::min(nodes[u].low,nodes[v].low);
                    if(nodes[v].low>nodes[u].disc)        edges[eidx].is_bridge=true;
                    if(pe==-1&&children>1)                nodes[u].is_articulation=true;
                    if(pe!=-1&&nodes[v].low>=nodes[u].disc) nodes[u].is_articulation=true;
                } else if(eidx!=pe){
                    nodes[u].low=std::min(nodes[u].low,nodes[v].disc);
                }
            }
        };
        for(int i=0;i<n;i++) if(!vis[i]) dfs(i,-1);
        for(auto& nd:nodes) if(nd.is_articulation) nd.pulse=1.f;
        for(auto& e:edges)  if(e.is_bridge)        e.pulse=1.f;
    }

    // ── Kosaraju SCC (directed) ───────────────────────────────────────────────
    void run_kosaraju(){
        clear_analysis();
        int n=nodes.size(); if(!n) return;

        // Pass 1: finish order trên adj gốc
        std::vector<bool> vis(n,false);
        std::stack<int> order;
        std::function<void(int)> dfs1=[&](int u){
            vis[u]=true;
            for(auto [v,_]:adj[u]) if(!vis[v]) dfs1(v);
            order.push(u);
        };
        for(int i=0;i<n;i++) if(!vis[i]) dfs1(i);

        // Pass 2: DFS trên radj theo finish order
        std::fill(vis.begin(),vis.end(),false);
        int cid=0;
        std::function<void(int,int)> dfs2=[&](int u,int c){
            vis[u]=true; nodes[u].scc_id=c;
            for(int v:radj[u]) if(!vis[v]) dfs2(v,c);
        };
        while(!order.empty()){
            int u=order.top(); order.pop();
            if(!vis[u]){ dfs2(u,cid++); }
        }
        num_scc=cid;
        for(auto& nd:nodes) nd.pulse=1.f;
    }

    // ── BFS / DFS steps ───────────────────────────────────────────────────────
    struct BFSStep { int u,v,edge_idx; std::string desc; };
    struct DFSStep { int u,v,edge_idx; std::string desc; bool backtrack; };
    std::vector<BFSStep> bfs_steps;
    std::vector<DFSStep> dfs_steps;

    void build_bfs_steps(int start=0){
        bfs_steps.clear(); int n=nodes.size(); if(!n) return;
        std::vector<bool> vis(n,false);
        std::vector<int> q={start}; vis[start]=true;
        bfs_steps.push_back({start,-1,-1,"BFS: start "+std::to_string(start)});
        int qi=0;
        while(qi<(int)q.size()){
            int u=q[qi++];
            for(auto [v,eidx]:adj[u]) if(!vis[v]){
                vis[v]=true; q.push_back(v);
                bfs_steps.push_back({u,v,eidx,"BFS: "+std::to_string(u)+" -> "+std::to_string(v)});
            }
        }
    }

    void build_dfs_steps(int start=0){
        dfs_steps.clear(); int n=nodes.size(); if(!n) return;
        std::vector<bool> vis(n,false);
        std::function<void(int,int)> dfs=[&](int u,int pe){
            vis[u]=true;
            std::string info=(nodes[u].disc>=0)?"  d="+std::to_string(nodes[u].disc)+" l="+std::to_string(nodes[u].low):"";
            dfs_steps.push_back({u,-1,pe,"DFS visit "+std::to_string(u)+info,false});
            for(auto [v,eidx]:adj[u]) if(!vis[v]){
                dfs(v,eidx);
                dfs_steps.push_back({v,u,eidx,"Backtrack to "+std::to_string(u),true});
            }
        };
        for(int i=0;i<n;i++) if(!vis[i]) dfs(i,-1);
    }

private:
    void _rm_edge(int idx){
        if(idx<0||idx>=(int)edges.size()) return;
        int u=edges[idx].u, v=edges[idx].v;
        auto rm_a=[&](int node,int eidx){
            auto& row=adj[node];
            row.erase(std::remove_if(row.begin(),row.end(),
                [eidx](auto& p){return p.second==eidx;}),row.end());
        };
        auto rm_r=[&](int node,int tgt){
            auto& row=radj[node];
            row.erase(std::remove(row.begin(),row.end(),tgt),row.end());
        };
        rm_a(u,idx); rm_r(v,u);
        if(!directed){ rm_a(v,idx); rm_r(u,v); }
        edges.erase(edges.begin()+idx);
        for(auto& row:adj)
            for(auto& [_,ei]:row) if(ei>idx) ei--;
    }
};
