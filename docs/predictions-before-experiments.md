# Predictions for the two-algorithm scope

Retained from the prediction record created before the 30 September 2026 collection. The two-person scope keeps P1, P2, P4 and P5; identifiers retain their original numbering.

P1. On growing layered networks with many parallel shortest paths, Dinic should require fewer complete BFS traversals than Edmonds–Karp, because one level graph can support multiple augmentations. Its speed advantage should increase with layer width, though constant costs may hide this on tiny inputs.

P2. Edge density should increase edge scanning for all algorithms. We predict Dinic beats Edmonds–Karp on larger dense random networks.

P4. Unit-capacity bipartite networks should favor Dinic as graph size increases because blocking flow can process multiple matches in one phase. Edmonds–Karp performs at least one successful BFS per unit of final flow. We expect this operation-count difference to be clearer than timing differences on small cases.

P5. Small instances may favor a simpler implementation despite weaker worst-case complexity. No particular crossover size is asserted before measurement because hardware and constant factors are unknown. A missing crossover will be reported rather than invented.

The paired method uses the same generated edge list, fixed seeds, separate counter and timing solves, and a case-level frozen input plan. Layered width changes depth and edge count together; results describe that combined structure.
