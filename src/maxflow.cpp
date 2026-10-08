#include "maxflow.hpp"
#include <cmath>
#include <random>
namespace mf {
Solution finish(const Graph &g, const Network &r, Cap v, Stats z) {
    Solution x{v, {}, std::vector<bool>(g.n), z};
    for (std::size_t i = 0; i < g.edges.size(); ++i) {
        auto [u, j] = r.original[i];
        x.flows.push_back(g.edges[i].capacity - r.a[u][j].cap);
    }
    std::queue<int> q;
    q.push(g.s);
    x.reachable[g.s] = true;
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (auto e : r.a[u])
            if (e.cap && !x.reachable[e.to]) {
                x.reachable[e.to] = true;
                q.push(e.to);
            }
    }
    return x;
}
Graph generate(const std::string &family, int size, double param, std::uint64_t seed) {
    if (size < 2 || !std::isfinite(param) || param < 0 || param > 1)
        throw std::invalid_argument("size >= 2 and param in [0,1] required");
    std::mt19937_64 rng(seed);
    Graph g{size, 0, size - 1, {}};
    auto add = [&](int u, int v, Cap c) { g.edges.push_back({u, v, c}); };
    auto cap = [&]() { return Cap(1 + rng() % 100); };
    if (family == "sparse" || family == "dense") {
        double p = family == "sparse" ? std::min(1.0, param * 8 / size) : param;
        for (int u = 0; u < size; ++u)
            for (int v = 0; v < size; ++v)
                if (u != v && std::generate_canonical<double, 53>(rng) < p)
                    add(u, v, cap());
    } else if (family == "layered") {
        int width = std::max(1, int(param * std::max(1, size - 2)));
        for (int v = 1; v < size - 1 && v <= width; ++v)
            add(0, v, cap());
        for (int u = 1; u < size - 1; ++u) {
            int layer = (u - 1) / width;
            for (int v = 1; v < size - 1; ++v)
                if ((v - 1) / width == layer + 1)
                    add(u, v, cap());
            if ((u - 1) / width == (size - 3) / width)
                add(u, size - 1, cap());
        }
    } else if (family == "bipartite") {
        int split = (size - 2) / 2;
        for (int u = 1; u <= split; ++u) {
            add(0, u, 1);
            for (int v = split + 1; v < size - 1; ++v)
                if (std::generate_canonical<double, 53>(rng) < param)
                    add(u, v, 1);
        }
        for (int v = split + 1; v < size - 1; ++v)
            add(v, size - 1, 1);
    } else
        throw std::invalid_argument("unknown family");
    std::shuffle(g.edges.begin(), g.edges.end(), rng);
    return g;
}
} // namespace mf
