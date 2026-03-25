#pragma once
#include <vector>
#include <set>
#include <algorithm>
#include <string>
#include <functional>

struct Node
{
    int id;
    float x, y;
    int disc = -1, low = -1;
    bool is_articulation = false;
    // animation
    float pulse = 0.f; // 0..1 glow effect
    float spawn = 0.f; // 0..1 spawn animation
};

struct Edge
{
    int u, v;
    bool is_bridge = false;
    float pulse = 0.f;
};

class Graph
{
public:
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    // adj[u] = list of {v, edge_idx}
    std::vector<std::vector<std::pair<int, int>>> adj;

    int add_node(float x, float y)
    {
        int id = nodes.size();
        Node nd;
        nd.id = id;
        nd.x = x;
        nd.y = y;
        nd.spawn = 0.f;
        nodes.push_back(nd);
        adj.push_back({});
        return id;
    }

    // returns edge idx, or -1 if already exists / self-loop
    int add_edge(int u, int v)
    {
        if (u == v)
            return -1;
        if (u < 0 || v < 0 || u >= (int)nodes.size() || v >= (int)nodes.size())
            return -1;
        for (auto [nv, ei] : adj[u])
            if (nv == v)
                return -1; // already exists
        int idx = edges.size();
        edges.push_back({u, v, false, 0.f});
        adj[u].push_back({v, idx});
        adj[v].push_back({u, idx});
        return idx;
    }

    void remove_node(int id)
    {
        if (id < 0 || id >= (int)nodes.size())
            return;
        // Mark edges to remove
        std::vector<int> to_remove;
        for (int i = 0; i < (int)edges.size(); i++)
            if (edges[i].u == id || edges[i].v == id)
                to_remove.push_back(i);
        // Remove edges (reverse order)
        std::sort(to_remove.rbegin(), to_remove.rend());
        for (int ei : to_remove)
            _remove_edge_by_idx(ei);
        // Remap node ids
        nodes.erase(nodes.begin() + id);
        for (int i = id; i < (int)nodes.size(); i++)
            nodes[i].id = i;
        // Remap adj & edges
        adj.erase(adj.begin() + id);
        for (auto &row : adj)
            for (auto &[v, _] : row)
                if (v > id)
                    v--;
        for (auto &e : edges)
        {
            if (e.u > id)
                e.u--;
            if (e.v > id)
                e.v--;
        }
    }

    void remove_edge(int idx) { _remove_edge_by_idx(idx); }

    void clear_analysis()
    {
        for (auto &nd : nodes)
        {
            nd.disc = -1;
            nd.low = -1;
            nd.is_articulation = false;
            nd.pulse = 0.f;
        }
        for (auto &e : edges)
        {
            e.is_bridge = false;
            e.pulse = 0.f;
        }
    }

    // ---- Tarjan ----
    void run_tarjan()
    {
        clear_analysis();
        int n = nodes.size();
        if (n == 0)
            return;
        std::vector<bool> vis(n, false);
        int timer = 0;

        std::function<void(int, int)> dfs = [&](int u, int pe)
        {
            vis[u] = true;
            nodes[u].disc = nodes[u].low = timer++;
            int children = 0;
            for (auto [v, eidx] : adj[u])
            {
                if (!vis[v])
                {
                    children++;
                    dfs(v, eidx);
                    nodes[u].low = std::min(nodes[u].low, nodes[v].low);
                    // Bridge
                    if (nodes[v].low > nodes[u].disc)
                        edges[eidx].is_bridge = true;
                    // Articulation
                    if (pe == -1 && children > 1)
                        nodes[u].is_articulation = true;
                    if (pe != -1 && nodes[v].low >= nodes[u].disc)
                        nodes[u].is_articulation = true;
                }
                else if (eidx != pe)
                {
                    nodes[u].low = std::min(nodes[u].low, nodes[v].disc);
                }
            }
        };

        for (int i = 0; i < n; i++)
            if (!vis[i])
                dfs(i, -1);

        // Trigger pulse animation
        for (auto &nd : nodes)
            if (nd.is_articulation)
                nd.pulse = 1.f;
        for (auto &e : edges)
            if (e.is_bridge)
                e.pulse = 1.f;
    }

    // ---- BFS step generator ----
    struct BFSStep
    {
        int u, v, edge_idx;
        std::string desc;
    };
    std::vector<BFSStep> bfs_steps;

    void build_bfs_steps(int start = 0)
    {
        bfs_steps.clear();
        int n = nodes.size();
        if (n == 0)
            return;
        std::vector<bool> vis(n, false);
        std::vector<int> q = {start};
        vis[start] = true;
        bfs_steps.push_back({start, -1, -1, "BFS: start node " + std::to_string(start)});
        int qi = 0;
        while (qi < (int)q.size())
        {
            int u = q[qi++];
            for (auto [v, eidx] : adj[u])
            {
                if (!vis[v])
                {
                    vis[v] = true;
                    q.push_back(v);
                    bfs_steps.push_back({u, v, eidx,
                                         "BFS: " + std::to_string(u) + " -> " + std::to_string(v)});
                }
            }
        }
    }

    // ---- DFS step generator ----
    struct DFSStep
    {
        int u, v, edge_idx;
        std::string desc;
        bool backtrack;
    };
    std::vector<DFSStep> dfs_steps;

    void build_dfs_steps(int start = 0)
    {
        dfs_steps.clear();
        int n = nodes.size();
        if (n == 0)
            return;
        std::vector<bool> vis(n, false);
        std::function<void(int, int)> dfs = [&](int u, int pe)
        {
            vis[u] = true;
            dfs_steps.push_back({u, -1, pe,
                                 "DFS visit " + std::to_string(u) +
                                     "  disc=" + std::to_string(nodes[u].disc) +
                                     "  low=" + std::to_string(nodes[u].low),
                                 false});
            for (auto [v, eidx] : adj[u])
            {
                if (!vis[v])
                {
                    dfs(v, eidx);
                    dfs_steps.push_back({v, u, eidx,
                                         "Backtrack to " + std::to_string(u), true});
                }
            }
        };
        for (int i = 0; i < n; i++)
            if (!vis[i])
                dfs(i, -1);
    }

private:
    void _remove_edge_by_idx(int idx)
    {
        if (idx < 0 || idx >= (int)edges.size())
            return;
        int u = edges[idx].u, v = edges[idx].v;
        // Remove from adj
        auto rm = [&](int node, int eidx)
        {
            auto &row = adj[node];
            row.erase(std::remove_if(row.begin(), row.end(),
                                     [eidx](auto &p)
                                     { return p.second == eidx; }),
                      row.end());
        };
        rm(u, idx);
        rm(v, idx);
        edges.erase(edges.begin() + idx);
        // Re-index adj
        for (auto &row : adj)
            for (auto &[_, ei] : row)
                if (ei > idx)
                    ei--;
    }
};
