// Shared immutable input, paired residual arcs, counters, and result certificates.
// Total input capacity is bounded by INT64_MAX, conservatively protecting every
// residual capacity, accumulated flow, and excess update from signed overflow.
#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>
namespace mf {
using Cap = std::int64_t;
struct Edge {
    int u, v;
    Cap capacity;
};
struct Graph {
    int n, s, t;
    std::vector<Edge> edges;
};
struct Stats {
    std::uint64_t scans = 0, bfs = 0, augmentations = 0;
};
struct Arc {
    int to, rev;
    Cap cap;
};
struct Network {
    std::vector<std::vector<Arc>> a;
    std::vector<std::pair<int, int>> original;
    explicit Network(const Graph &g) : a(g.n >= 2 ? g.n : 0) {
        if (g.n < 2 || g.s < 0 || g.t < 0 || g.s >= g.n || g.t >= g.n || g.s == g.t)
            throw std::invalid_argument("invalid graph terminals");
        Cap sum = 0;
        for (auto e : g.edges) {
            if (e.u < 0 || e.v < 0 || e.u >= g.n || e.v >= g.n || e.u == e.v || e.capacity < 0)
                throw std::invalid_argument("invalid edge (self loops rejected)");
            if (e.capacity > std::numeric_limits<Cap>::max() - sum)
                throw std::overflow_error("sum of capacities exceeds int64");
            sum += e.capacity;
            int i = a[e.u].size(), j = a[e.v].size();
            original.emplace_back(e.u, i);
            a[e.u].push_back({e.v, j, e.capacity});
            a[e.v].push_back({e.u, i, 0});
        }
    }
    void send(int u, int i, Cap x) {
        auto &e = a[u][i];
        e.cap -= x;
        a[e.to][e.rev].cap += x;
    }
};
struct Solution {
    Cap value;
    std::vector<Cap> flows;
    std::vector<bool> reachable;
    Stats stats;
};
template <bool Count> inline void scan(Stats &s) {
    if constexpr (Count)
        ++s.scans;
}
Solution finish(const Graph &, const Network &, Cap, Stats);
} // namespace mf
#ifdef MF_ENABLE_DINIC
#include "dinic.hpp"
#endif
#include "edmonds_karp.hpp"
namespace mf {
template <bool Count> Cap run(Network &r, int s, int t, const std::string &algo, Stats &z) {
    if (algo == "ek")
        return edmonds_karp<Count>(r, s, t, z);
#ifdef MF_ENABLE_DINIC
    if (algo == "dinic")
        return dinic<Count>(r, s, t, z);
#endif
    throw std::invalid_argument("requested algorithm is unavailable in this build");
}
inline std::vector<std::string> available_algorithms() {
#ifdef MF_ENABLE_DINIC
    return {"ek", "dinic"};
#else
    return {"ek"};
#endif
}
inline Solution solve(const Graph &g, const std::string &algo) {
    Network r(g);
    Stats z;
    Cap v = run<true>(r, g.s, g.t, algo, z);
    return finish(g, r, v, z);
}
Graph generate(const std::string &, int, double, std::uint64_t);
} // namespace mf
