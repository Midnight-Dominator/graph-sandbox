#pragma once
#include <vector>
#include <string>
#include <set>

// Trạng thái từng bước để highlight khi animate
struct AnimStep
{
    int active_node = -1; // node đang xét
    int active_edge = -1; // cạnh đang đi qua
    int parent_node = -1; // node cha
    bool backtrack = false;
    std::string log;
};

class StepAnimator
{
public:
    std::vector<AnimStep> steps;
    int cur = -1;
    bool done = false;
    bool running = false;

    float timer = 0.f;
    float speed = 1.f; // bước/giây (1 = chậm, 5 = nhanh, 10 = rất nhanh)

    // Các node/cạnh đã được "thăm" để vẽ màu cây
    std::set<int> visited_nodes;
    std::set<int> tree_edges;
    std::set<int> back_edges;

    void reset()
    {
        steps.clear();
        cur = -1;
        done = false;
        running = false;
        timer = 0.f;
        visited_nodes.clear();
        tree_edges.clear();
        back_edges.clear();
    }

    void start()
    {
        if (steps.empty())
        {
            done = true;
            return;
        }
        cur = -1;
        done = false;
        running = true;
        timer = 0.f;
        visited_nodes.clear();
        tree_edges.clear();
        back_edges.clear();
    }

    // Gọi mỗi frame, trả về true nếu advance thêm bước
    bool update(float dt)
    {
        if (!running || done)
            return false;
        timer += dt * speed;
        if (timer >= 1.f)
        {
            timer -= 1.f;
            cur++;
            if (cur >= (int)steps.size())
            {
                done = true;
                running = false;
                return false;
            }
            auto &s = steps[cur];
            if (s.active_node >= 0)
                visited_nodes.insert(s.active_node);
            if (s.active_edge >= 0)
            {
                if (!s.backtrack)
                    tree_edges.insert(s.active_edge);
                else
                    back_edges.insert(s.active_edge);
            }
            return true;
        }
        return false;
    }

    const AnimStep *current() const
    {
        if (cur < 0 || cur >= (int)steps.size())
            return nullptr;
        return &steps[cur];
    }

    float progress() const
    {
        if (steps.empty())
            return 1.f;
        return (float)(cur + 1) / steps.size();
    }

    void skip_to_end()
    {
        for (int i = cur + 1; i < (int)steps.size(); i++)
        {
            auto &s = steps[i];
            if (s.active_node >= 0)
                visited_nodes.insert(s.active_node);
            if (s.active_edge >= 0)
                tree_edges.insert(s.active_edge);
        }
        cur = (int)steps.size() - 1;
        done = true;
        running = false;
    }
};
