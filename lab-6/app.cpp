#include <iostream>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <functional>
#include <chrono>
#include <limits>
#include <mutex>

using namespace std;

struct Graph {
    int numVertices;
    int** adjMatrix;
};

// Function to generate a random weighted graph in parallel
Graph generateGraphParallel(int numVertices, unsigned int numberOfThreads) {
    Graph graph;
    graph.numVertices = numVertices;

    graph.adjMatrix = new int*[numVertices];
    for (int i = 0; i < numVertices; i++) {
        graph.adjMatrix[i] = new int[numVertices];
    }

    auto initializeRows = [&](int startRow, int endRow, unsigned int seed) {
        srand(seed);
        for (int i = startRow; i < endRow; i++) {
            for (int j = 0; j < numVertices; j++) {
                if (i == j) {
                    graph.adjMatrix[i][j] = 0;
                } else {
                    if (rand() % 2) {
                        graph.adjMatrix[i][j] = rand() % 10 + 1;
                    } else {
                        graph.adjMatrix[i][j] = 0;
                    }
                }
            }
        }
    };

    unsigned int rowsPerThread = numVertices / numberOfThreads;
    unsigned int remainingRows = numVertices % numberOfThreads;
    thread* threads = new thread[numberOfThreads];
    unsigned int currentRow = 0;

    for (unsigned int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
        bool shouldTakeExtraRow = threadIndex < remainingRows;
        unsigned int startRow = currentRow;
        unsigned int endRow = startRow + rowsPerThread + (shouldTakeExtraRow ? 1 : 0);
        currentRow = endRow;

        unsigned int seed = static_cast<unsigned int>(time(nullptr)) + threadIndex;
        threads[threadIndex] = thread(initializeRows, startRow, endRow, seed);
    }

    for (unsigned int i = 0; i < numberOfThreads; i++) {
        threads[i].join();
    }

    delete[] threads;
    return graph;
}

int* dijkstraSequential(Graph& graph, int src) {
    int V = graph.numVertices;
    int* dist = new int[V];
    bool* sptSet = new bool[V];

    // Initialize distances and sptSet
    for (int i = 0; i < V; i++) {
        dist[i] = numeric_limits<int>::max();
        sptSet[i] = false;
    }
    dist[src] = 0;

    // Find shortest path for all vertices
    for (int count = 0; count < V - 1; count++) {
        // Find the minimum distance vertex from the set of vertices not yet processed
        int min = numeric_limits<int>::max(), min_index = -1;

        for (int v = 0; v < V; v++)
            if (!sptSet[v] && dist[v] <= min)
                min = dist[v], min_index = v;

        int u = min_index;
        if (u == -1)
            break;

        sptSet[u] = true;

        // Update dist value of the adjacent vertices of the picked vertex
        for (int v = 0; v < V; v++)
            if (!sptSet[v] && graph.adjMatrix[u][v] && dist[u] != numeric_limits<int>::max()
                && dist[u] + graph.adjMatrix[u][v] < dist[v])
                dist[v] = dist[u] + graph.adjMatrix[u][v];
    }

    delete[] sptSet;
    return dist;
}

int* dijkstraParallel(Graph& graph, int src, unsigned int numberOfThreads) {
    int V = graph.numVertices;
    int* dist = new int[V];
    bool* sptSet = new bool[V];
    mutex distMutex;

    // Initialize distances and sptSet
    for (int i = 0; i < V; i++) {
        dist[i] = numeric_limits<int>::max();
        sptSet[i] = false;
    }
    dist[src] = 0;

    for (int count = 0; count < V - 1; count++) {
        // Find the minimum distance vertex from the set of vertices not yet processed
        int min = numeric_limits<int>::max(), min_index = -1;

        for (int v = 0; v < V; v++)
            if (!sptSet[v] && dist[v] <= min)
                min = dist[v], min_index = v;

        int u = min_index;
        if (u == -1)
            break;

        sptSet[u] = true;

        // Divide the work among threads
        unsigned int verticesPerThread = V / numberOfThreads;
        unsigned int remainingVertices = V % numberOfThreads;
        thread* threads = new thread[numberOfThreads];
        unsigned int currentVertex = 0;

        for (unsigned int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
            bool shouldTakeRemainder = threadIndex < remainingVertices;
            unsigned int startVertex = currentVertex;
            unsigned int endVertex = startVertex + verticesPerThread + (shouldTakeRemainder ? 1 : 0);
            currentVertex = endVertex;

            // Creating a thread
            threads[threadIndex] = thread([=, &graph, &dist, &sptSet, &distMutex]() {
                for (unsigned int v = startVertex; v < endVertex; v++) {
                    if (!sptSet[v] && graph.adjMatrix[u][v] && dist[u] != numeric_limits<int>::max()) {
                        int newDist = dist[u] + graph.adjMatrix[u][v];
                        if (newDist < dist[v]) {
                            // Lock the mutex before updating shared data
                            lock_guard<mutex> lock(distMutex);
                            if (newDist < dist[v])
                                dist[v] = newDist;
                        }
                    }
                }
            });
        }

        for (unsigned int i = 0; i < numberOfThreads; i++) {
            threads[i].join();
        }

        delete[] threads;
    }

    delete[] sptSet;
    return dist;
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

BenchmarkResult benchmarkTime(function<int*()> function) {
    auto start = chrono::high_resolution_clock::now();
    int* result = function();
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    return { duration.count(), result };
}

void benchmark(
    unsigned int numVertices,
    unsigned int numberOfThreads,
    int sourceNode,
    bool runSequential = true,
    bool runParallel = true
) {
    cout << "Benchmark for Dijkstra algorithm with " << numVertices << " nodes and " << numberOfThreads << " threads:" << endl;
    if (!runSequential) {
        cout << "- Skipping sequential algorithms" << endl;
    }
    if (!runParallel) {
        cout << "- Skipping parallel algorithms" << endl;
    }

    cout << endl << "=====================" << endl << endl;

    cout << "- Generating graph..." << endl << endl;
    auto graphGenerationStart = chrono::high_resolution_clock::now();
    Graph graph = generateGraphParallel(numVertices, numberOfThreads);
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

        sequentialBenchmark = benchmarkTime([&]() {
            return dijkstraSequential(graph, sourceNode);
        });
        cout << "   - Time: " << sequentialBenchmark.time << "ms" << endl << endl;
    }

    if (runParallel) {
        cout << "- Running parallel algorithm:" << endl;

        parallelBenchmark = benchmarkTime([&]() {
            return dijkstraParallel(graph, sourceNode, numberOfThreads);
        });
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
        cout << "   - Shortest paths length equal: " << (areEqual ? "Yes" : "No") << endl << endl;

        // Clean up results
        delete[] sequentialBenchmark.result;
        delete[] parallelBenchmark.result;
    }

    // Clean up graph
    for (int i = 0; i < numVertices; i++) {
        delete[] graph.adjMatrix[i];
    }
    delete[] graph.adjMatrix;
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
