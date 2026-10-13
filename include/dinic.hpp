// Primary author: Jinying Li (14543526).
// A complete BFS builds each level graph. Recursive current-arc search then
// sends paths until that graph is blocked, updating paired reverse arcs.
#pragma once
namespace mf {
namespace detail {
template <bool Count>
inline Cap dinic_send(Network &r, int u, int t, Cap limit,
                      const std::vector<int> &level, std::vector<int> &current,
                      Stats &z) {
    if (u == t)
        return limit;

    while (current[u] < static_cast<int>(r.a[u].size())) {
        int i = current[u];
        scan<Count>(z);
        const Arc &e = r.a[u][i];
        if (e.cap > 0 && level[e.to] == level[u] + 1) {
            Cap sent = dinic_send<Count>(r, e.to, t, std::min(limit, e.cap), level,
                                         current, z);
            if (sent > 0) {
                r.send(u, i, sent);
                return sent;
            }
        }
        ++current[u];
    }
    return 0;
}
} // namespace detail

template <bool Count> Cap dinic(Network &r, int s, int t, Stats &z) {
    const int n = static_cast<int>(r.a.size());
    std::vector<int> level(n);
    std::vector<int> current(n);
    Cap total = 0;

    for (;;) {
        if constexpr (Count)
            ++z.bfs;
        std::fill(level.begin(), level.end(), -1);
        level[s] = 0;
        std::queue<int> q;
        q.push(s);
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (const Arc &e : r.a[u]) {
                scan<Count>(z);
                if (e.cap > 0 && level[e.to] < 0) {
                    level[e.to] = level[u] + 1;
                    q.push(e.to);
                }
            }
        }
        if (level[t] < 0)
            return total;

        std::fill(current.begin(), current.end(), 0);
        for (;;) {
            Cap sent = detail::dinic_send<Count>(
                r, s, t, std::numeric_limits<Cap>::max(), level, current, z);
            if (sent == 0)
                break;
            total += sent;
            if constexpr (Count)
                ++z.augmentations;
        }
    }
}
} // namespace mf
