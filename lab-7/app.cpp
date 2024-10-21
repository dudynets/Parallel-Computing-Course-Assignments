#include <iostream>
#include <thread>
#include <functional>
#include <chrono>
#include <random>
#include <limits>
#include <mutex>

using namespace std;

void generateGraph(int n, double** graph) {
    for(int i = 0; i < n; ++i) {
        for(int j = 0; j < n; ++j) {
            graph[i][j] = 0;
        }
    }

    mt19937 rng(random_device{}());
    uniform_real_distribution<double> dist(1.0, 10.0);

    int* nodes = new int[n];
    for(int i = 0; i < n; ++i) {
        nodes[i] = i;
    }

    for(int i = n - 1; i > 0; --i) {
        int j = rng() % (i + 1);
        swap(nodes[i], nodes[j]);
    }

    for(int i = 0; i < n - 1; ++i) {
        int u = nodes[i];
        int v = nodes[i+1];
        double weight = dist(rng);
        graph[u][v] = weight;
        graph[v][u] = weight;
    }

    uniform_int_distribution<int> nodeDist(0, n - 1);
    for(int i = 0; i < n * 2; ++i) {
        int u = nodeDist(rng);
        int v = nodeDist(rng);
        if(u != v && graph[u][v] == 0) {
            double weight = dist(rng);
            graph[u][v] = weight;
            graph[v][u] = weight;
        }
    }
    delete[] nodes;
}

void primSequential(int n, double** graph, int startNode, int* parent) {
    bool* inMST = new bool[n];
    double* key = new double[n];

    for(int i = 0; i < n; ++i) {
        key[i] = numeric_limits<double>::infinity();
        inMST[i] = false;
        parent[i] = -1;
    }
    key[startNode] = 0;

    for(int count = 0; count < n - 1; ++count) {
        // Знаходимо вершину з мінімальним значенням ключа, яка не в MST
        double minKey = numeric_limits<double>::infinity();
        int u = -1;
        for(int v = 0; v < n; ++v) {
            if(!inMST[v] && key[v] < minKey) {
                minKey = key[v];
                u = v;
            }
        }
        if(u == -1) {
            break;
        }
        inMST[u] = true;

        // Оновлюємо значення ключів та батьків для сусідніх вершин
        for(int v = 0; v < n; ++v) {
            if(graph[u][v] != 0 && !inMST[v] && graph[u][v] < key[v]) {
                key[v] = graph[u][v];
                parent[v] = u;
            }
        }
    }

    delete[] inMST;
    delete[] key;
}

void primParallel(int n, double** graph, int startNode, int* parent, int numThreads) {
    bool* inMST = new bool[n];
    double* key = new double[n];

    for(int i = 0; i < n; ++i) {
        key[i] = numeric_limits<double>::infinity();
        inMST[i] = false;
        parent[i] = -1;
    }
    key[startNode] = 0;

    mutex mtx;

    int chunkSize = (n + numThreads - 1) / numThreads;

    for(int count = 0; count < n - 1; ++count) {
        // Паралельна частина: знаходимо вершину з мінімальним ключем
        double globalMinKey = numeric_limits<double>::infinity();
        int u = -1;

        double* localMinKeys = new double[numThreads];
        int* localMinVertices = new int[numThreads];
        thread* threads = new thread[numThreads];

        for(int t = 0; t < numThreads; ++t) {
            int start = t * chunkSize;
            int end = (start + chunkSize < n) ? start + chunkSize : n;

            threads[t] = thread([&, t, start, end]() {
                double minKey = numeric_limits<double>::infinity();
                int minVertex = -1;
                for(int v = start; v < end; ++v) {
                    if(!inMST[v] && key[v] < minKey) {
                        minKey = key[v];
                        minVertex = v;
                    }
                }
                localMinKeys[t] = minKey;
                localMinVertices[t] = minVertex;
            });
        }

        // Очікуємо завершення потоків
        for(int t = 0; t < numThreads; ++t) {
            threads[t].join();
        }

        // Знаходимо глобальний мінімум
        for(int t = 0; t < numThreads; ++t) {
            if(localMinKeys[t] < globalMinKey) {
                globalMinKey = localMinKeys[t];
                u = localMinVertices[t];
            }
        }
        delete[] localMinKeys;
        delete[] localMinVertices;
        delete[] threads;

        if(u == -1) {
            break;
        }
        inMST[u] = true;

        // Паралельна частина: оновлюємо ключі та батьків
        threads = new thread[numThreads];
        for(int t = 0; t < numThreads; ++t) {
            int start = t * chunkSize;
            int end = (start + chunkSize < n) ? start + chunkSize : n;

            threads[t] = thread([&, u, start, end]() {
                for(int v = start; v < end; ++v) {
                    if(graph[u][v] != 0 && !inMST[v] && graph[u][v] < key[v]) {
                        lock_guard<mutex> lock(mtx);
                        key[v] = graph[u][v];
                        parent[v] = u;
                    }
                }
            });
        }
        for(int t = 0; t < numThreads; ++t) {
            threads[t].join();
        }
        delete[] threads;
    }

    delete[] inMST;
    delete[] key;
}

bool areArraysEqual(int* arr1, int* arr2, int n) {
    for (int i = 0; i < n; i++)
        if (arr1[i] != arr2[i])
            return false;
    return true;
}

double calculateSpeedup(double sequentialTime, double parallelTime) {
    if (parallelTime == 0) {
        return 0;
    }
    return sequentialTime / parallelTime;
}

double calculateEfficiency(double speedup, int numberOfThreads) {
    return speedup / numberOfThreads;
}

struct BenchmarkResult {
    long long time;
    int* result;
};

BenchmarkResult benchmarkSequentialTime(int n, double** graph, int startNode) {
    int* parent = new int[n];

    auto start = chrono::high_resolution_clock::now();
    primSequential(n, graph, startNode, parent);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    return { duration.count(), parent };
}

BenchmarkResult benchmarkParallelTime(int n, double** graph, int startNode, int numThreads) {
    int* parent = new int[n];

    auto start = chrono::high_resolution_clock::now();
    primParallel(n, graph, startNode, parent, numThreads);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    return { duration.count(), parent };
}

void benchmark(
    unsigned int numVertices,
    unsigned int numberOfThreads,
    int sourceNode,
    bool runSequential = true,
    bool runParallel = true
) {
    cout << "Benchmark for Prim's algorithm with " << numVertices << " nodes and " << numberOfThreads << " threads:" << endl;
    if (!runSequential) {
        cout << "- Skipping sequential algorithms" << endl;
    }
    if (!runParallel) {
        cout << "- Skipping parallel algorithms" << endl;
    }

    cout << endl << "=====================" << endl << endl;

    cout << "- Generating graph..." << endl << endl;
    auto graphGenerationStart = chrono::high_resolution_clock::now();

    double** graph = new double*[numVertices];
    for(int i = 0; i < numVertices; ++i) graph[i] = new double[numVertices];
    generateGraph(numVertices, graph);

    auto graphGenerationEnd = chrono::high_resolution_clock::now();
    auto graphGenerationDuration = chrono::duration_cast<chrono::milliseconds>(graphGenerationEnd - graphGenerationStart);
    cout << "Graph generated in " << graphGenerationDuration.count() << "ms" << endl << endl;

    cout << "Press any key to continue..." << endl;
    cin.get();
    cout << "=====================" << endl << endl;

    BenchmarkResult sequentialBenchmark;
    BenchmarkResult parallelBenchmark;

    if (runSequential) {
        cout << "- Running sequential algorithm:" << endl;

        sequentialBenchmark = benchmarkSequentialTime(numVertices, graph, sourceNode);
        cout << "   - Time: " << sequentialBenchmark.time << "ms" << endl << endl;
    }

    if (runParallel) {
        cout << "- Running parallel algorithm:" << endl;

        parallelBenchmark = benchmarkParallelTime(numVertices, graph, sourceNode, numberOfThreads);
        cout << "   - Time: " << parallelBenchmark.time << "ms" << endl << endl;
    }

    if (runSequential && runParallel) {
        cout << "=====================" << endl << endl;
        cout << "- Summary:" << endl;

        cout << "   - Nodes: " << numVertices << endl;
        cout << "   - Threads: " << numberOfThreads << endl;
        cout << "   - Source Node: " << sourceNode << endl;

        double speedup = calculateSpeedup(sequentialBenchmark.time, parallelBenchmark.time);
        double efficiency = calculateEfficiency(speedup, numberOfThreads);
        bool areEqual = areArraysEqual(sequentialBenchmark.result, parallelBenchmark.result, numVertices);

        cout << "   - Sequential time: " << sequentialBenchmark.time << "ms" << endl;
        cout << "   - Parallel time: " << parallelBenchmark.time << "ms" << endl;
        cout << "   - Speedup: " << speedup << "x" << endl;
        cout << "   - Efficiency: " << int(efficiency * 100) << "%" << " (took " << parallelBenchmark.time << "ms vs " << sequentialBenchmark.time / numberOfThreads << "ms ideal)" << endl;
        cout << "   - MST equal: " << (areEqual ? "Yes" : "No") << endl << endl;

        // Clean up results
        delete[] sequentialBenchmark.result;
        delete[] parallelBenchmark.result;
    }

    // Clean up graph
    for(int i = 0; i < numVertices; ++i) {
        delete[] graph[i];
    }
    delete[] graph;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage: <numVertices> <threads> <sourceNode> [runSequential] [runParallel]" << endl;
        return 1;
    }

    unsigned int numVertices = atoi(argv[1]);
    unsigned int threads = atoi(argv[2]);
    int sourceNode = atoi(argv[3]);

    bool runSequential = argc < 5 || atoi(argv[4]) == 1;
    bool runParallel = argc < 6 || atoi(argv[5]) == 1;

    benchmark(numVertices, threads, sourceNode, runSequential, runParallel);

    return 0;
}
