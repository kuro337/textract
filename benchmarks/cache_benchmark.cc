#include <folly/AtomicUnorderedMap.h>
#include <folly/Benchmark.h>
#include <folly/concurrency/ConcurrentHashMap.h>
#include <folly/init/Init.h>

/*

1 second:                         1
1 millisecond (ms):           0.001 seconds
1 microsecond (us):        0.000001 seconds
1 nanosecond  (ns):     0.000000001 seconds
*/

void mutexMapBenchmark(int threadCount, int insertionsPerThread) {
    std::unordered_map<int, std::string> map;
    std::mutex                           mapMutex;

    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&map, &mapMutex, i, insertionsPerThread] {
            for (int j = 1; j <= insertionsPerThread; ++j) {
                int         key   = i * insertionsPerThread + j;
                std::string value = "Value " + std::to_string(key);

                std::lock_guard<std::mutex> lock(mapMutex);
                map[key] = std::move(value);
            }
        });
    }

    for (auto &t: threads) {
        t.join();
    }
}

void concurrentMapBenchmark(int threadCount, int insertionsPerThread) {
    folly::ConcurrentHashMap<int, std::string> map;

    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&map, i, insertionsPerThread] {
            for (int j = 0; j < insertionsPerThread; ++j) {
                map.insert_or_assign(j, "Value " + std::to_string(i * 100 + j));
            }
        });
    }

    for (auto &t: threads) {
        t.join();
    }
}

void concurrentMapBenchmarkComplex(int threadCount, int insertionsPerThread) {
    auto v = std::vector<std::string> {"Complex", "Data", "Type"};

    folly::ConcurrentHashMap<std::string, std::vector<std::string>> map;

    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&map, &v, i, insertionsPerThread] {
            for (int j = 1; j <= insertionsPerThread; ++j) {
                std::string key = "Key" + std::to_string(i * insertionsPerThread + j);
                map.insert_or_assign(std::move(key), v);
            }
        });
    }

    for (auto &t: threads) {
        t.join();
    }
}

void atomicMapBenchmark(int threadCount, int insertionsPerThread) {
    folly::AtomicUnorderedInsertMap<int, std::string> atomicMap(threadCount * insertionsPerThread);

    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&atomicMap, i, insertionsPerThread] {
            for (int j = 1; j <= insertionsPerThread; ++j) {
                int key = i * insertionsPerThread + j;
                atomicMap.emplace(key, "Value " + std::to_string(key));
            }
        });
    }

    for (auto &t: threads) {
        t.join();
    }
}

void atomicMapBenchmarkComplex(int threadCount, int insertionsPerThread) {
    auto v = std::vector<std::string> {"Complex", "Data", "Type"};

    folly::AtomicUnorderedInsertMap<std::string, std::vector<std::string>> atomicMap(
        threadCount * insertionsPerThread);

    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&atomicMap, &v, i, insertionsPerThread] {
            for (int j = 1; j <= insertionsPerThread; ++j) {
                std::string key = "Key" + std::to_string(i * insertionsPerThread + j);
                atomicMap.emplace(std::move(key), v);
            }
        });
    }

    for (auto &t: threads) {
        t.join();
    }
}

BENCHMARK(UnorderedMapMutexedSingleThreaded, n) { mutexMapBenchmark(1, n); }
BENCHMARK(UnorderedMapMutexedMultiThreaded, n) { mutexMapBenchmark(5, n); }
BENCHMARK(UnorderedMapMutexedMaxThreads, n) { mutexMapBenchmark(12, n); }

BENCHMARK(ConcurrentHashMapSingleThreaded, n) { concurrentMapBenchmark(1, n); }
BENCHMARK(ConcurrentHashMapMultiThreaded, n) { concurrentMapBenchmark(5, n); }
BENCHMARK(ConcurrentHashMapMaxThreads, n) { concurrentMapBenchmark(12, n); }

BENCHMARK(ConcurrentHashMapComplexSingleThreaded, n) { concurrentMapBenchmarkComplex(1, n); }

BENCHMARK(ConcurrentHashMapComplexMultiThreaded, n) { concurrentMapBenchmarkComplex(5, n); }

BENCHMARK(ConcurrentHashMapComplexMaxThreads, n) { concurrentMapBenchmarkComplex(12, n); }

BENCHMARK(AtomicUnorderedMapSingleThreaded, n) { atomicMapBenchmark(1, n); }
BENCHMARK(AtomicUnorderedMapMultiThreaded, n) { atomicMapBenchmark(5, n); }
BENCHMARK(AtomicUnorderedMapMaxThreads, n) { atomicMapBenchmark(12, n); }

BENCHMARK(AtomicUnorderedMapComplexSingleThreaded, n) { atomicMapBenchmarkComplex(1, n); }

BENCHMARK(AtomicUnorderedMapComplexMultiThreaded, n) { atomicMapBenchmarkComplex(5, n); }

BENCHMARK(AtomicUnorderedMapComplexMaxThreads, n) { atomicMapBenchmarkComplex(12, n); }

auto main(int argc, char *argv[]) -> int {
    folly::Init init(&argc, &argv);
    folly::runBenchmarks();
    return 0;
}