# Joint analysis plan

After Jinying Li integrates Dinic, collect both algorithms on the fixed input matrix. XinghaoYuan and Jinying Li then interpret the paired measurements together and update the separately supplied report.

| Input family | Main question | Explanatory evidence |
| --- | --- | --- |
| Sparse random | When does simpler path search compete with level-graph setup? | Flow value, BFS calls, inspections, early sink discovery |
| Dense random | Does phase reuse offset repeated traversal of larger adjacency lists? | Edges, augmentations, inspections per solve |
| Layered | How do parallel paths and the combined width/depth structure affect phase reuse? | Group width/depth, terminal capacities, BFS phases, scans |
| Bipartite unit capacity | How many matching paths can one level graph process? | Matching size, successful sends, BFS calls, residual reassignment |

Use seven timing repeats to obtain a median for each algorithm and graph. Form EK/Dinic ratios on the same graph, then summarize the five seed ratios with a geometric mean. Inspect exploratory paired bootstrap intervals and keep cells crossing one distinct from consistently ordered cells. Examine CPU time alongside elapsed time and operation counts to identify sensitivity to scheduling and short timer brackets.

Write the final analysis around observed mechanisms, a specific surprise relative to the retained predictions, and a targeted follow-up experiment. Layered width changes depth and edge count together, so that generator supports a combined structural comparison. Separate those factors in an extension before making a causal claim about width alone.

Numerical tables, figures, the final interpretation and the report are produced from the joint collection and stored locally. The repository contains the tools and plan for that work.
