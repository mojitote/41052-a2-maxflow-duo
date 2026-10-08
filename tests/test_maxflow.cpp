// Independent exhaustive-cut oracle does not call a maximum-flow algorithm.
// Certificates additionally verify edge capacities, conservation, and the final
// residual cut. The uninstrumented execution must match the counted execution.
#include "maxflow.hpp"
#include <iostream>
#include <random>
void require(bool ok, const char *msg) {
    if (!ok)
        throw std::runtime_error(msg);
}
mf::Cap oracle(const mf::Graph &g) {
    mf::Cap best = std::numeric_limits<mf::Cap>::max();
    for (std::uint64_t mask = 0; mask < (1ULL << g.n); ++mask) {
        if (!(mask & (1ULL << g.s)) || (mask & (1ULL << g.t)))
            continue;
        mf::Cap cut = 0;
        for (auto e : g.edges)
            if ((mask & (1ULL << e.u)) && !(mask & (1ULL << e.v)))
                cut += e.capacity;
        best = std::min(best, cut);
    }
    return best;
}
void check_expected(const mf::Graph &g, mf::Cap correct) {
    for (auto algo : mf::available_algorithms()) {
        auto x = mf::solve(g, algo);
        require(x.value == correct, "independent expected-value disagreement");
        require(x.flows.size() == g.edges.size(), "flow size");
        std::vector<mf::Cap> b(g.n);
        mf::Cap cut = 0;
        for (std::size_t i = 0; i < g.edges.size(); ++i) {
            auto e = g.edges[i];
            auto f = x.flows[i];
            require(f >= 0 && f <= e.capacity, "capacity violation");
            b[e.u] -= f;
            b[e.v] += f;
            if (x.reachable[e.u] && !x.reachable[e.v])
                cut += e.capacity;
        }
        for (int v = 0; v < g.n; ++v)
            require(b[v] == (v == g.s   ? -x.value
                             : v == g.t ? x.value
                                        : 0),
                    "conservation violation");
        require(x.reachable[g.s] && !x.reachable[g.t], "residual reachability");
        require(cut == x.value, "certificate cut disagreement");
        mf::Network r(g);
        mf::Stats z;
        require(mf::run<false>(r, g.s, g.t, algo, z) == correct, "uncounted disagreement");
        require(z.scans == 0 && z.bfs == 0 && z.augmentations == 0,
                "instrumentation leaked");
    }
}
// Only small graphs use exponential cut enumeration.
void check(const mf::Graph &g) {
    require(g.n < 63, "exhaustive oracle restricted to small graphs");
    check_expected(g, oracle(g));
}
int main() {
    try {
        check({2, 0, 1, {}});
        check({2, 0, 1, {{0, 1, 0}, {0, 1, 7}, {0, 1, 9}, {1, 0, 20}}});
        check({4, 0, 3, {{0, 1, 9}, {1, 2, 9}, {2, 3, 9}, {0, 2, 2}, {1, 3, 2}}});
        check({3, 0, 2, {{0, 1, std::numeric_limits<mf::Cap>::max()}}});
        check({2, 0, 1, {{0, 1, std::numeric_limits<mf::Cap>::max()}}});
        std::mt19937_64 rng(931);
        for (int trial = 0; trial < 1200; ++trial) {
            int n = 2 + rng() % 7;
            mf::Graph g{n, int(rng() % n), 0, {}};
            g.t = (g.s + 1 + rng() % (n - 1)) % n;
            for (int i = 0; i < n * n; ++i) {
                int u = rng() % n, v = rng() % n;
                if (u != v && rng() % 3 == 0)
                    g.edges.push_back({u, v, mf::Cap(rng() % 30)});
            }
            check(g);
        }
        for (auto f :
             {"sparse", "dense", "layered", "bipartite"})
            for (int n = 2; n <= 9; ++n)
                check(mf::generate(f, n, .4, 74));
        // Adverse edge order first sends s-a-c-t. The second unit must use the
        // residual reverse c-a arc to replace that choice with a-d-t and b-c-t.
        mf::Graph reroute{
            6, 0, 5, {{0, 1, 1}, {0, 2, 1}, {1, 3, 1}, {1, 4, 1}, {2, 3, 1}, {3, 5, 1}, {4, 5, 1}}};
        check(reroute);
        require(oracle(reroute) == 2, "reroute known minimum cut");
        for (auto algo : mf::available_algorithms()) {
            auto x = mf::solve(reroute, algo);
            require(x.flows[2] == 0 && x.flows[3] == 1 && x.flows[4] == 1,
                    "reverse-arc rerouting failed");
        }
        // A 2048-node chain has capacity 7 on every edge. A feasible flow of 7
        // and any single-edge cut of 7 independently prove its optimum. This
        // case deliberately uses no exhaustive oracle (2^2048 cuts).
        mf::Graph deep{2048, 0, 2047, {}};
        for (int u = 0; u < 2047; ++u)
            deep.edges.push_back({u, u + 1, 7});
        check_expected(deep, 7);
        int rejected = 0;
        const std::vector<mf::Graph> invalid = {
            {-1, 0, 1, {}},
            {0, 0, 1, {}},
            {1, 0, 0, {}},
            {2, -1, 1, {}},
            {2, 0, -1, {}},
            {2, 2, 1, {}},
            {2, 0, 2, {}},
            {2, 0, 0, {}},
            {2, 0, 1, {{-1, 1, 1}}},
            {2, 0, 1, {{0, -1, 1}}},
            {2, 0, 1, {{2, 1, 1}}},
            {2, 0, 1, {{0, 2, 1}}},
            {2, 0, 1, {{0, 0, 1}}},
            {2, 0, 1, {{0, 1, -1}}},
            {2, 0, 1, {{0, 1, std::numeric_limits<mf::Cap>::max()}, {0, 1, 1}}}};
        for (const auto &g : invalid) {
            try {
                mf::Network r(g);
            } catch (const std::invalid_argument &) {
                ++rejected;
            } catch (const std::overflow_error &) {
                ++rejected;
            }
        }
        require(rejected == 15, "validation rejection missing");
        std::cout << "PASS: 1238 small graphs with exhaustive-cut oracle; "
                     "one 2048-node chain with optimum 7; counted/uncounted parity; "
                     "reverse-arc rerouting; 15 invalid inputs rejected; algorithms: ";
        for (const auto &name : mf::available_algorithms()) std::cout << name << ' ';
        std::cout << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
