// Primary author: XinghaoYuan (25145766).
// BFS finds a shortest residual path. Each send updates a paired reverse arc,
// permitting a later augmenting path to cancel and redirect earlier flow.
#pragma once
namespace mf {
template <bool Count> Cap edmonds_karp(Network &r, int s, int t, Stats &z) {
    Cap total = 0;
    int n = r.a.size();
    std::vector<int> p(n), pe(n);
    for (;;) {
        if constexpr (Count)
            ++z.bfs;
        std::fill(p.begin(), p.end(), -1);
        p[s] = s;
        std::queue<int> q;
        q.push(s);
        while (!q.empty() && p[t] < 0) {
            int u = q.front();
            q.pop();
            for (int i = 0; i < (int)r.a[u].size(); ++i) {
                scan<Count>(z);
                auto e = r.a[u][i];
                if (e.cap > 0 && p[e.to] < 0) {
                    p[e.to] = u;
                    pe[e.to] = i;
                    q.push(e.to);
                }
            }
        }
        if (p[t] < 0)
            return total;
        Cap x = std::numeric_limits<Cap>::max();
        for (int v = t; v != s; v = p[v])
            x = std::min(x, r.a[p[v]][pe[v]].cap);
        for (int v = t; v != s; v = p[v])
            r.send(p[v], pe[v], x);
        total += x;
        if constexpr (Count)
            ++z.augmentations;
    }
}
} // namespace mf
