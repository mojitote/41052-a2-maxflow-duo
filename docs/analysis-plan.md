# Joint analysis method

The completed two-algorithm study uses the fixed input matrix and the complete 19 October 2026 elapsed-time and process-CPU collections. The separately supplied report and evidence archive contain the observations and final interpretation.

| Input family | Main question | Explanatory evidence |
| --- | --- | --- |
| Sparse random | When does simple path search compete with level-graph setup? | Flow, BFS calls, inspections and early sink discovery |
| Dense random | Does phase reuse offset repeated traversal of larger adjacency lists? | Edges, augmentations and inspections |
| Layered | How do useful parallel paths and the combined width/depth structure affect reuse? | Width/depth, terminal capacities, BFS phases and scans |
| Bipartite unit capacity | How many matching paths can one level graph process? | Matching size, successful sends, BFS calls and reassignment |

Reduce seven timing repeats to a median for each algorithm and graph. Form EK/Dinic ratios on the same graph, then summarize the five seed ratios using their geometric mean. The exploratory paired bootstrap uses the five independent graph seeds, not timing repeats. Keep intervals crossing one distinct from consistently ordered cells.

Compare elapsed time with process CPU time and deterministic work counts. Five-seed cell statistics and one-seed diagnostic examples answer different questions; label each explicitly. Use the diagnostic examples to explain a mechanism, not to replace the full collection.

Write the analysis around observed mechanisms, a specific departure from the retained predictions and a targeted follow-up. Layered width changes depth and edge count together, so that generator supports a combined structural interpretation. Independently vary those factors before assigning a causal effect to width alone.

Numerical outputs, figures, the report and the evidence archive stay outside Git.
