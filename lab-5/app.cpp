#include <iostream> // Input/output stream
#include <ctime> // Needed for seed the random number generator
#include <cstdlib> // Needed for rand and srand
#include <thread> // Needed for multithreading
#include <functional>  // Needed to pass functions as arguments
#include <climits> // Needed for INT_MAX
#include <chrono> // Needed for time measurements

using namespace std;

#define INF INT_MAX / 2

// Function to generate a random graph
int** generateGraph(int n) {
    int** graph = new int*[n];

    for (int i = 0; i < n; i++) {
        graph[i] = new int[n];

        for (int j = 0; j < n; j++) {
            if (i == j) {
                graph[i][j] = 0;
            } else {
                // Randomly decide whether to include an edge (20% chance)
                if (rand() % 10 < 2) {
                    // Random weight between 1 and 10
                    graph[i][j] = rand() % 10 + 1;
                } else {
                    graph[i][j] = INF;
                }
            }
        }
    }

    return graph;
}

// Sequential Floyd-Warshall algorithm
int** computeFloydSequential(int** graph, int n) {
    int** dist = new int*[n];
    for (int i = 0; i < n; i++) {
        dist[i] = new int[n];
        for (int j = 0; j < n; j++) {
            dist[i][j] = graph[i][j];
        }
    }

    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (
                    dist[i][k] != INF &&
                    dist[k][j] != INF &&
                    dist[i][j] > dist[i][k] + dist[k][j]
                ) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
    }

    return dist;
}

// Parallel Floyd-Warshall algorithm
int** computeFloydParallel(int** graph, int n, unsigned int numberOfThreads) {
    int** dist = new int*[n];
    for (int i = 0; i < n; i++) {
        dist[i] = new int[n];
        for (int j = 0; j < n; j++) {
            dist[i][j] = graph[i][j];
        }
    }

    for (int k = 0; k < n; k++) {
        // No need to use more threads than rows
        if (numberOfThreads > n) {
            numberOfThreads = n;
        }

        unsigned int rowsPerThread = n / numberOfThreads;
        unsigned int remainingRows = n % numberOfThreads;

        thread* threads = new thread[numberOfThreads];

        unsigned int currentRow = 0;

        for (unsigned int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
            unsigned int startRow = currentRow;
            unsigned int endRow = startRow + rowsPerThread + (threadIndex < remainingRows ? 1 : 0);
            currentRow = endRow;

            threads[threadIndex] = thread([=]() {
                for (unsigned int i = startRow; i < endRow; i++) {
                    for (int j = 0; j < n; j++) {
                        if (
                            dist[i][k] != INF &&
                            dist[k][j] != INF &&
                            dist[i][j] > dist[i][k] + dist[k][j]
                        ) {
                            dist[i][j] = dist[i][k] + dist[k][j];
                        }
                    }
                }
            });
        }

        // Join threads
        for (unsigned int i = 0; i < numberOfThreads; i++) {
            threads[i].join();
        }

        delete[] threads;
    }

    return dist;
}

// Check if two matrices are equal
bool areMatricesEqual(int** matrix1, int** matrix2, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (matrix1[i][j] != matrix2[i][j]) {
                return false;
            }
        }
    }
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
    int** result;
};

BenchmarkResult benchmarkTime(function<int**()> function) {
    auto start = chrono::high_resolution_clock::now();
    int** result = function();
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    return { duration.count(), result };
}

void benchmark(
    unsigned int n,
    unsigned int numberOfThreads,
    int a,
    int b,
    bool runSequential = true,
    bool runParallel = true
) {
    cout << "Benchmark for Floyd algorithm with " << n << " nodes and " << numberOfThreads << " threads:" << endl;
    cout << "Shortest path from node " << a << " to node " << b << endl;

    if (!runSequential) {
        cout << "- Skipping sequential algorithms" << endl;
    }
    if (!runParallel) {
        cout << "- Skipping parallel algorithms" << endl;
    }

    cout << endl << "=====================" << endl << endl;

    cout << "- Generating graph..." << endl << endl;
    int** graph = generateGraph(n);

    BenchmarkResult sequentialBenchmark;
    BenchmarkResult parallelBenchmark;

    cout << "Press any key to continue..." << endl;
    cin.get();
    cout << "=====================" << endl << endl;

    if (runSequential) {
        cout << "- Running sequential algorithm:" << endl;

        sequentialBenchmark = benchmarkTime([&]() {
            return computeFloydSequential(graph, n);
        });
        cout << "   - Time: " << sequentialBenchmark.time << "ms" << endl;
        cout << "   - Shortest path from " << a << " to " << b << " length: " << sequentialBenchmark.result[a][b] << endl << endl;
    }

    if (runParallel) {
        cout << "- Running parallel algorithm:" << endl;

        parallelBenchmark = benchmarkTime([&]() {
            return computeFloydParallel(graph, n, numberOfThreads);
        });
        cout << "   - Time: " << parallelBenchmark.time << "ms" << endl;
        cout << "   - Shortest path from " << a << " to " << b << " length: " << parallelBenchmark.result[a][b] << endl << endl;
    }

    if (runSequential && runParallel) {
        cout << "=====================" << endl << endl;
        cout << "- Summary:" << endl;

        cout << "   - Nodes: " << n << endl;
        cout << "   - Threads: " << numberOfThreads << endl;
        cout << "   - Sequential time: " << sequentialBenchmark.time << "ms" << endl;
        cout << "   - Parallel time: " << parallelBenchmark.time << "ms" << endl;

        double speedup = calculateSpeedup(sequentialBenchmark.time, parallelBenchmark.time);
        double efficiency = calculateEfficiency(speedup, numberOfThreads);
        bool areEqual = areMatricesEqual(sequentialBenchmark.result, parallelBenchmark.result, n);

        cout << "   - Speedup: " << speedup << "x" << endl;
        cout << "   - Efficiency: " << int(efficiency * 100) << "% (took " << parallelBenchmark.time << "ms vs " << sequentialBenchmark.time / numberOfThreads << "ms ideal)" << endl;
        cout << "   - Shortest paths equal: " << (areEqual ? "Yes" : "No") << endl << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        cout << "Usage: <n> <threads> <a> <b> [runSequential] [runParallel]" << endl;
        return 1;
    }

    unsigned int n = atoi(argv[1]);
    unsigned int threads = atoi(argv[2]);
    int a = atoi(argv[3]);
    int b = atoi(argv[4]);

    if (a < 0 || a >= n || b < 0 || b >= n) {
        cout << "Error: Nodes 'a' and 'b' must be between 0 and " << n - 1 << endl;
        return 1;
    }

    bool runSequential = argc < 6 || atoi(argv[5]) == 1;
    bool runParallel = argc < 7 || atoi(argv[6]) == 1;

    srand(time(NULL)); // Seed the random number generator
    benchmark(n, threads, a, b, runSequential, runParallel);

    return 0;
}
