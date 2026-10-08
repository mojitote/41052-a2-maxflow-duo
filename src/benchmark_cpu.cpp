#include "maxflow.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <time.h>
// Supplementary process CPU timing. This program remains single-threaded.
// CPU brackets are outer; steady-clock wall brackets are inner. Setup, warm-up,
// certificates, and counted solves remain outside both timing intervals.
static std::int64_t process_cpu_ns() {
    timespec t{};
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t) != 0)
        throw std::runtime_error("process CPU clock unavailable");
    return std::int64_t(t.tv_sec) * 1000000000LL + t.tv_nsec;
}
static void calibrate() {
    timespec resolution{};
    if (clock_getres(CLOCK_PROCESS_CPUTIME_ID, &resolution) != 0)
        throw std::runtime_error("process CPU clock resolution unavailable");
    std::vector<double> cpu, wall;
    cpu.reserve(10000);
    wall.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        const auto c0 = process_cpu_ns();
        const auto w0 = std::chrono::steady_clock::now();
        const auto w1 = std::chrono::steady_clock::now();
        const auto c1 = process_cpu_ns();
        cpu.push_back(double(c1 - c0));
        wall.push_back(std::chrono::duration<double, std::nano>(w1 - w0).count());
    }
    std::sort(cpu.begin(), cpu.end());
    std::sort(wall.begin(), wall.end());
    std::cout << std::setprecision(12) << "{\"samples\":10000,\"cpu_clock_resolution_ns\":"
              << std::int64_t(resolution.tv_sec) * 1000000000LL + resolution.tv_nsec
              << ",\"empty_cpu_bracket_median_ns\":" << (cpu[4999] + cpu[5000]) / 2
              << ",\"empty_wall_bracket_median_ns\":" << (wall[4999] + wall[5000]) / 2 << "}\n";
}
int main(int argc, char **argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--calibrate") {
            calibrate();
            return 0;
        }
        if (argc != 7) {
            std::cerr << "usage: benchmark_cpu FAMILY SIZE PARAM SEED REPEATS BATCH\n";
            return 2;
        }
        auto g =
            mf::generate(argv[1], std::stoi(argv[2]), std::stod(argv[3]), std::stoull(argv[4]));
        int repeats = std::stoi(argv[5]), batch = std::stoi(argv[6]);
        if (repeats < 1 || batch < 1 || batch > 10000)
            throw std::invalid_argument("repeats and batch positive; batch <= 10000");
        std::uint64_t hash = 14695981039346656037ULL;
        auto mix = [&](std::uint64_t x) {
            for (int i = 0; i < 8; ++i) {
                hash ^= (x >> (8 * i)) & 255;
                hash *= 1099511628211ULL;
            }
        };
        mix(g.n);
        mix(g.s);
        mix(g.t);
        for (auto e : g.edges) {
            mix(e.u);
            mix(e.v);
            mix(e.capacity);
        }
        std::cout << "family,size,param,seed,edges,algo,repeat,batch,flow,time_us,scans,bfs,"
                     "augmentations,graph_hash,cpu_time_us\n";
        mf::Cap expected = -1;
        const auto algorithms = mf::available_algorithms();
        const int count = static_cast<int>(algorithms.size());
        std::vector<mf::Solution> counted;
        for (auto algo : algorithms) {
            auto x = mf::solve(g, algo);
            if (expected < 0)
                expected = x.value;
            else if (expected != x.value)
                throw std::logic_error("algorithm disagreement");
            counted.push_back(x);
        }
        for (int rep = 0; rep < repeats; ++rep)
            for (int order = 0; order < count; ++order) {
                int index = (order + rep) % count;
                auto algo = algorithms[index];
                {
                    mf::Network warm(g);
                    mf::Stats dummy;
                    mf::run<false>(warm, g.s, g.t, algo, dummy);
                }
                {
                    std::vector<mf::Network> networks;
                    networks.reserve(batch);
                    for (int j = 0; j < batch; ++j)
                        networks.emplace_back(g);
                    mf::Cap value = 0;
                    mf::Stats dummy;
                    // All residual copies are prepared before timing. Count=false removes counters.
                    const auto cpu_start = process_cpu_ns();
                    auto start = std::chrono::steady_clock::now();
                    for (auto &r : networks)
                        value = mf::run<false>(r, g.s, g.t, algo, dummy);
                    auto stop = std::chrono::steady_clock::now();
                    const auto cpu_stop = process_cpu_ns();
                    if (value != expected)
                        throw std::logic_error("timed solve disagreement");
                    // Check every timed solution after the clock stops, using flow and cut
                    // certificates.
                    for (auto &r : networks) {
                        auto x = mf::finish(g, r, value, {});
                        std::vector<mf::Cap> balance(g.n);
                        mf::Cap cut = 0;
                        for (std::size_t i = 0; i < g.edges.size(); ++i) {
                            auto e = g.edges[i];
                            auto f = x.flows[i];
                            if (f < 0 || f > e.capacity)
                                throw std::logic_error("capacity certificate failed");
                            balance[e.u] -= f;
                            balance[e.v] += f;
                            if (x.reachable[e.u] && !x.reachable[e.v])
                                cut += e.capacity;
                        }
                        for (int u = 0; u < g.n; ++u)
                            if (balance[u] != (u == g.s ? -value : u == g.t ? value : 0))
                                throw std::logic_error("flow certificate failed");
                        if (x.reachable[g.t] || cut != value)
                            throw std::logic_error("cut certificate failed");
                    }
                    double us =
                        std::chrono::duration<double, std::micro>(stop - start).count() / batch;
                    const double cpu_us = double(cpu_stop - cpu_start) / (1000.0 * batch);
                    auto z = counted[index].stats;
                    std::cout << argv[1] << ',' << g.n << ',' << argv[3] << ',' << argv[4] << ','
                              << g.edges.size() << ',' << algo << ',' << rep << ',' << batch << ','
                              << value << ',' << std::setprecision(12) << us << ',' << z.scans
                              << ',' << z.bfs << ',' << z.augmentations << ',' << hash << ',' << cpu_us << '\n';
                }
            }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
