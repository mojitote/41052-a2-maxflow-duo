CXX = clang++
CPPFLAGS = -Iinclude
CXXFLAGS = -std=c++17 -O3 -DNDEBUG -Wall -Wextra -Wpedantic
HEADERS = $(wildcard include/*.hpp)

.PHONY: all test sanitize compare test-pair sanitize-pair clean
all: build/benchmark build/benchmark_cpu build/test_maxflow
build:
	mkdir -p build
build/benchmark: src/benchmark.cpp src/maxflow.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/maxflow.cpp src/benchmark.cpp -o $@
build/benchmark_cpu: src/benchmark_cpu.cpp src/maxflow.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/maxflow.cpp src/benchmark_cpu.cpp -o $@
build/test_maxflow: tests/test_maxflow.cpp src/maxflow.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -std=c++17 -O2 -Wall -Wextra -Wpedantic src/maxflow.cpp tests/test_maxflow.cpp -o $@
test: build/test_maxflow
	./build/test_maxflow
build/test_maxflow_sanitize: tests/test_maxflow.cpp src/maxflow.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer src/maxflow.cpp tests/test_maxflow.cpp -o $@
sanitize: build/test_maxflow_sanitize
	./build/test_maxflow_sanitize

compare: build/benchmark_pair build/benchmark_cpu_pair
build/benchmark_pair: src/benchmark.cpp src/maxflow.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DMF_ENABLE_DINIC src/maxflow.cpp src/benchmark.cpp -o $@
build/benchmark_cpu_pair: src/benchmark_cpu.cpp src/maxflow.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DMF_ENABLE_DINIC src/maxflow.cpp src/benchmark_cpu.cpp -o $@
build/test_maxflow_pair: tests/test_maxflow.cpp src/maxflow.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -std=c++17 -O2 -DMF_ENABLE_DINIC src/maxflow.cpp tests/test_maxflow.cpp -o $@
test-pair: build/test_maxflow_pair
	./build/test_maxflow_pair
build/test_maxflow_pair_sanitize: tests/test_maxflow.cpp src/maxflow.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -std=c++17 -O1 -g -DMF_ENABLE_DINIC -fsanitize=address,undefined -fno-omit-frame-pointer src/maxflow.cpp tests/test_maxflow.cpp -o $@
sanitize-pair: build/test_maxflow_pair_sanitize
	./build/test_maxflow_pair_sanitize
clean:
	rm -f build/benchmark build/benchmark_cpu build/test_maxflow build/test_maxflow_sanitize build/benchmark_pair build/benchmark_cpu_pair build/test_maxflow_pair build/test_maxflow_pair_sanitize
