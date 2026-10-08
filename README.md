# Maximum flow comparison

Two-person C++17 project for UTS 41052 Advanced Algorithms A2.

| Member | Student ID | Primary algorithm | Shared responsibilities |
| --- | --- | --- | --- |
| XinghaoYuan | 25145766 | Edmonds–Karp | Residual graph, input generators, tests, benchmarking framework |
| Jinying Li | 14543526 | Dinic | Dinic integration, paired experiment collection, structured-input explanation |

The current build runs **Edmonds–Karp** with the shared correctness and benchmark framework. Jinying Li’s implementation slot is `include/dinic.hpp`. The two-algorithm targets use the `MF_ENABLE_DINIC` build switch. Final interpretation of the paired measurements is joint work.

The report and Jinying Li’s detailed implementation prompt are supplied separately. Generated measurements, figures, reports and handoff documents are kept locally through `.gitignore`.

## Run XinghaoYuan’s module

Requirements: a C++17 compiler and Make. Python 3 is used for experiment orchestration. Process CPU timing targets macOS and Linux.

```bash
make all
make test
make sanitize
./build/benchmark sparse 514 0.5 11 7 2
```

The tests check 1238 small graphs against an independent exhaustive-cut oracle, a 2048-node chain with optimum seven, reverse-arc rerouting, counted/uncounted parity, and 15 invalid inputs. Each timed benchmark solution is checked after timing against capacity, conservation and minimum-cut certificates.

An EK-only smoke collection covers all four input families:

```bash
python3 scripts/run_experiments.py --algorithms ek --smoke --output build/ek-smoke-01
python3 scripts/run_experiments.py --algorithms ek --smoke --clock cpu --output build/ek-smoke-01
```

The CSV output displays graph size, edge count, algorithm, maximum flow, elapsed time, operation counts and the graph fingerprint. The CPU benchmark also displays process CPU time. Choose a fresh directory for another collection.

## Jinying Li’s implementation stage

Complete `include/dinic.hpp` with this entry point:

```cpp
template <bool Count>
Cap dinic(Network &r, int s, int t, Stats &z);
```

Use BFS levels, recursive blocking-flow search and per-phase current-arc indices. Residual updates go through `Network::send`. Count each BFS invocation including the final failed one, each BFS/DFS arc inspection, and each successful top-level source-to-sink send. Counter updates are compile-time gated, so `Count=false` leaves all counters zero.

After the module is supplied:

```bash
make compare test-pair sanitize-pair
./build/benchmark_pair layered 514 0.125 11 2 1
python3 scripts/run_experiments.py --algorithms pair --smoke --output build/pair-smoke-01
```

The same oracle, deep-chain, rerouting and certificate checks then exercise both algorithms. The separately supplied prompt contains the exact traversal policy and acceptance steps.

## Paired experiments and joint analysis

The input plan defines **235 graphs in 47 parameter cells**, with five seeds per cell and seven repeats per algorithm. Families are sparse random, dense random, layered, and unit-capacity bipartite networks. See [the input matrix](results/experiment-plan.json), [predictions](docs/predictions-before-experiments.md), and [the analysis plan](docs/analysis-plan.md).

Once both modules pass their tests, collect fresh paired data and generate local summaries:

```bash
python3 -m pip install -r requirements.txt
python3 scripts/run_experiments.py --algorithms pair --output results/pair-run-01
python3 scripts/run_experiments.py --algorithms pair --clock cpu --output results/pair-run-01
python3 scripts/analyze.py --data-dir results/pair-run-01
```

A complete pair collection contains 3290 rows per clock collection. The runner preserves existing outputs and records source, binary, plan and environment hashes. The analysis tools check input and repeat coverage, aggregate seven repeats per graph, compute paired seed ratios, and produce tables and figures in the chosen local run directory. The group then connects the observed rankings to BFS calls, arc inspections, augmentations and graph structure, and writes the final conclusions.

Generation, residual copying, warm-up, counted solves, certificates and I/O are outside the solve clock. Search-array initialization is inside it. Plot bands describe the range of five seed medians; exploratory bootstrap intervals use the five paired graph seeds. Repetitions characterize timing on a graph and are separate from independent graph sampling.

## CMake

```bash
cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake
ctest --test-dir build/cmake --output-on-failure
```

Use `-DENABLE_DINIC=ON` for the pair build after implementing Dinic, and `-DSANITIZE=ON` for sanitizer checks. For a custom compiler, use a clean build with `make CXX=g++` and pass `--compiler g++` to the runner.

## Repository layout

- `include/edmonds_karp.hpp`: XinghaoYuan’s algorithm module.
- `include/dinic.hpp`: Jinying Li’s implementation slot.
- `include/maxflow.hpp` and `src/maxflow.cpp`: common representation, generators and solution certificates.
- `src/benchmark.cpp` and `src/benchmark_cpu.cpp`: solve timing and separately counted operations.
- `tests/test_maxflow.cpp`: independent correctness checks.
- `scripts/`: collection and analysis tools.
- `results/experiment-plan.json`: the fixed input matrix.
- `docs/`: predictions and the joint analysis plan.

Commit code, tests and planning changes. Keep generated experiment outputs, report files and the teammate handoff local, as configured in `.gitignore`.
