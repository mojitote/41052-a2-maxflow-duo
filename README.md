# Maximum flow comparison

Two-person C++17 project for UTS 41052 Advanced Algorithms A2. Both Edmonds–Karp and Dinic are implemented and use the same residual graph, generators, correctness certificates and benchmark framework.

| Member | Student ID | Primary algorithm | Shared work |
| --- | --- | --- | --- |
| XinghaoYuan | 25145766 | Edmonds–Karp | Residual graph, generators, correctness tests, benchmark and analysis framework |
| Jinying Li | 14543526 | Dinic | Level-graph and current-arc implementation, integration, structured-input explanation |

## Build and test both algorithms

Requirements: a C++17 compiler and Make. Python 3 runs collection and analysis scripts. Process CPU timing supports macOS and Linux.

```bash
make all
make test
make sanitize
./build/benchmark_pair layered 514 0.125 11 7 1
```

The default build and test targets now include **both algorithms**. `make compare test-pair sanitize-pair` provides the same explicit pair targets. Tests check 1,238 small graphs against an independent exhaustive-cut oracle, a 2,048-node chain with optimum seven, reverse-arc rerouting, counted/uncounted parity and 15 invalid inputs. AddressSanitizer and UndefinedBehaviorSanitizer exercise the pair test harness.

Benchmark CSVs include input size, edges, algorithm, repeat, batch size, maximum flow, solve time, operation counts and graph fingerprint. The CPU executable adds process CPU time. Every timed residual solution is checked after timing against capacity, conservation and minimum-cut certificates.

## Collect experiments

The fixed input plan defines **235 graphs in 47 parameter cells**, with five seeds and seven repetitions per algorithm. Families are sparse random, dense random, layered and unit-capacity bipartite networks. See [the matrix](results/experiment-plan.json) and [the frozen predictions](docs/predictions-before-experiments.md).

A quick integration collection covers all four families:

```bash
python3 scripts/run_experiments.py --smoke --output build/pair-smoke-new
python3 scripts/run_experiments.py --smoke --clock cpu --output build/pair-smoke-new
```

For a complete new collection, choose an unused output directory:

```bash
python3 -m pip install -r requirements.txt
python3 scripts/run_experiments.py --output results/pair-run-new
python3 scripts/run_experiments.py --clock cpu --output results/pair-run-new
python3 scripts/analyze.py --data-dir results/pair-run-new
```

Each complete clock collection contains 3,290 rows. Existing raw CSVs are protected against overwrite. The runner records the compiler, source/binary/plan hashes, input coverage and environment. Preserve the actual build command and flags with each collection; the final evidence package includes `build-record.json` and the corresponding source snapshot.

Generation, residual copying, warm-up, counted solves, certificates and I/O are outside the solve clock. Search-array initialization is inside it. Both solvers see the same ordered edges and start with fresh residual networks. Seven repeats estimate timing on one graph; the five seeds supply the independent graph instances. Analysis checks exact coverage and matching graph hashes, flows and counters, then produces paired ratios, exploratory bootstrap intervals, summaries and figures.

## Reproduce the report figures

The report uses the complete collection dated **19 October 2026**. Its data and environment records are supplied in the separate `A2_Maxflow_Final_Evidence_2026-10-19.zip` archive. Extract that archive into this checkout so that `results/final-2026-10-19/full-raw.csv` exists, then run:

```bash
python3 scripts/analyze.py --data-dir results/final-2026-10-19
```

The evidence package contains both raw clock collections, the plan, calibration, source hashes, actual build record, measured source snapshot, analysis tables and generated figures. `REPRODUCE.txt` in the package explains the layout and commands. Regenerating figures from supplied measurements and collecting new timings are separate operations; new timings depend on the machine and its state.

Reports, measurements, figures and the teammate handoff remain outside Git, as configured in `.gitignore`. Commit code, tests and documentation; supply the report and experiment archive separately.

## CMake

```bash
cmake -S . -B build/cmake-pair -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake-pair
ctest --test-dir build/cmake-pair --output-on-failure
```

Dinic is enabled by default. Use `-DENABLE_DINIC=OFF` for an EK-only build, or `-DSANITIZE=ON` for sanitizer checks. With Make, `make build/benchmark build/test_maxflow` builds the EK-only targets. For another compiler use a clean build, for example `make CXX=g++`, and pass `--compiler g++` to the runner.

## Algorithm and counter contract

Both solvers accept signed 64-bit capacities, parallel edges and opposite original edges. Invalid terminals/endpoints, negative capacities, self-loops and total capacity exceeding `INT64_MAX` are rejected. Each input edge has its own paired residual reverse arc.

Dinic builds complete BFS levels and searches a blocking flow with recursive DFS and per-phase current-arc indices. A successful send preserves its current arc so remaining capacity can be reused. The shared `Network::send` operation updates both residual partners.

Counters count every BFS invocation, including the final failed one; each BFS/DFS adjacency inspection; and each successful top-level source-to-sink send. Compile-time `Count=false` removes counter updates. Both algorithms use O(V+E) storage; the general time bounds are O(VE²) for Edmonds–Karp and O(V²E) for this Dinic implementation. Recursive DFS depends on the process stack for very deep inputs.

## Repository layout

- `include/edmonds_karp.hpp`: XinghaoYuan’s algorithm.
- `include/dinic.hpp`: Jinying Li’s algorithm.
- `include/maxflow.hpp`, `src/maxflow.cpp`: common representation, generators and certificates.
- `src/benchmark.cpp`, `src/benchmark_cpu.cpp`: solve timing and separately counted operations.
- `tests/test_maxflow.cpp`: independent correctness checks.
- `scripts/`: collection and analysis tools.
- `results/experiment-plan.json`: fixed input matrix.
- `docs/`: frozen predictions and interpretation method.
