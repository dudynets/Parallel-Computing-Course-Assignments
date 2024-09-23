#include <iostream>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <functional>
#include <vector>
#include <chrono>
#include <cmath>

using namespace std;

// Generate a diagonally dominant matrix
int** generateDiagonallyDominantMatrix(int n) {
    int** matrix = new int*[n];

    for (int i = 0; i < n; i++) {
        matrix[i] = new int[n];
        int sum = 0;

        for (int j = 0; j < n; j++) {
            if (i != j) {
                matrix[i][j] = rand() % 10; // Random values from 0 to 9
                sum += abs(matrix[i][j]);
            }
        }
        // Make the diagonal element larger than the sum of the others
        matrix[i][i] = sum + rand() % 10 + 1; // Ensure strict diagonal dominance
    }

    return matrix;
}

// Generate a random vector
int* generateVector(int n) {
    int* vector = new int[n];

    for (int i = 0; i < n; i++) {
        vector[i] = rand() % 10; // Random values from 0 to 9
    }

    return vector;
}

// Sequential Jacobi method
double* solveJacobiSequential(
    int** A,
    int* b,
    int n,
    int maxIterations,
    double tolerance
) {
    double* x_old = new double[n];
    double* x_new = new double[n];

    // Initialize x to zero
    for (int i = 0; i < n; i++) {
        x_old[i] = 0.0;
    }

    for (int iter = 0; iter < maxIterations; iter++) {
        for (int i = 0; i < n; i++) {
            double sigma = 0.0;
            for (int j = 0; j < n; j++) {
                if (j != i) {
                    sigma += A[i][j] * x_old[j];
                }
            }
            x_new[i] = (b[i] - sigma) / A[i][i];
        }

        // Check for convergence
        double error = 0.0;
        for (int i = 0; i < n; i++) {
            error += abs(x_new[i] - x_old[i]);
            x_old[i] = x_new[i];
        }

        if (error < tolerance) {
            break;
        }
    }

    delete[] x_old;

    return x_new; // x_new contains the solution
}

// Parallel Jacobi method
double* solveJacobiParallel(
    int** A,
    int* b,
    int n,
    int maxIterations,
    double tolerance,
    unsigned int numberOfThreads
) {
    double* x_old = new double[n];
    double* x_new = new double[n];

    // Initialize x to zero
    for (int i = 0; i < n; i++) {
        x_old[i] = 0.0;
    }

    if (numberOfThreads > n) {
        numberOfThreads = n;
    }

    unsigned int rowsPerThread = n / numberOfThreads;
    unsigned int remainingRows = n % numberOfThreads;

    for (int iter = 0; iter < maxIterations; iter++) {
        std::vector<std::thread> threads;
        unsigned int currentRow = 0;

        for (unsigned int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
            // The first `remainingRows` threads will take 1 more row to distribute the remainder
            bool shouldTakeRemainder = threadIndex < remainingRows;

            unsigned int startRow = currentRow;
            unsigned int endRow = startRow + rowsPerThread + (shouldTakeRemainder ? 1 : 0);
            currentRow = endRow;

            threads.push_back(std::thread([=]() {
                for (unsigned int i = startRow; i < endRow; i++) {
                    double sigma = 0.0;
                    for (int j = 0; j < n; j++) {
                        if (j != i) {
                            sigma += A[i][j] * x_old[j];
                        }
                    }
                    x_new[i] = (b[i] - sigma) / A[i][i];
                }
            }));
        }

        // Wait for all threads to finish
        for (auto& t : threads) {
            t.join();
        }

        // Check for convergence
        double error = 0.0;
        for (int i = 0; i < n; i++) {
            error += abs(x_new[i] - x_old[i]);
            x_old[i] = x_new[i];
        }

        if (error < tolerance) {
            break;
        }
    }

    delete[] x_old;

    return x_new; // x_new contains the solution
}

// Check if two vectors are equal within a tolerance
bool areVectorsEqual(
    double* vec1,
    double* vec2,
    int n,
    double tolerance
) {
    for (int i = 0; i < n; i++) {
        if (abs(vec1[i] - vec2[i]) > tolerance) {
            return false;
        }
    }
    return true;
}

double calculateSpeedup(
    double sequentialTime,
    double parallelTime
) {
    if (parallelTime == 0) {
        return 0;
    }

    return sequentialTime / parallelTime;
}

double calculateEfficiency(
    double speedup,
    int numberOfThreads
) {
    if (numberOfThreads == 0) {
        return 0;
    }
    return speedup / numberOfThreads;
}

struct BenchmarkResult {
    long long time;
    double* result;
};

BenchmarkResult benchmarkTime(
    function<double*()> function
) {
    auto start = chrono::high_resolution_clock::now();
    double* result = function();
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    return { duration.count(), result };
}

void benchmark(
    unsigned int n,
    unsigned int numberOfThreads,
    int maxIterations = 10000,
    double tolerance = 1e-6,
    bool runSequential = true,
    bool runParallel = true
) {
    cout << "Benchmark for solving " << n << "x" << n << " system of linear equations with " << numberOfThreads << " threads using Jacobi method" << endl;
    if (!runSequential) {
        cout << "- Skipping sequential algorithms" << endl;
    }
    if (!runParallel) {
        cout << "- Skipping parallel algorithms" << endl;
    }

    cout << endl << "=====================" << endl << endl;

    cout << "- Generating matrix and vector..." << endl << endl;
    int** A = generateDiagonallyDominantMatrix(n);
    int* b = generateVector(n);

    BenchmarkResult sequentialBenchmark;
    BenchmarkResult parallelBenchmark;
    cout << "Press any key to continue..." << endl;
    cin.get();
    cout << "=====================" << endl << endl;

    if (runSequential) {
        cout << "- Running sequential Jacobi method:" << endl;
        sequentialBenchmark = benchmarkTime([&]() {
            return solveJacobiSequential(A, b, n, maxIterations, tolerance);
        });
        cout << "   - Time: " << sequentialBenchmark.time << "ms" << endl;
    }

    if (runParallel) {
        cout << "- Running parallel Jacobi method:" << endl;
        parallelBenchmark = benchmarkTime([&]() {
            return solveJacobiParallel(A, b, n, maxIterations, tolerance, numberOfThreads);
        });
        cout << "   - Time: " << parallelBenchmark.time << "ms" << endl;
    }

    if (runSequential && runParallel) {
        cout << endl << "=====================" << endl << endl;
        cout << "- Summary:" << endl;

        double speedup = calculateSpeedup(sequentialBenchmark.time, parallelBenchmark.time);
        double efficiency = calculateEfficiency(speedup, numberOfThreads);
        bool areEqual = areVectorsEqual(sequentialBenchmark.result, parallelBenchmark.result, n, tolerance);

        cout << "   - System size: " << n << "x" << n << endl;
        cout << "   - Threads: " << numberOfThreads << endl;
        cout << "   - Sequential time: " << sequentialBenchmark.time << "ms" << endl;
        cout << "   - Parallel time: " << parallelBenchmark.time << "ms" << endl;
        cout << "   - Speedup: " << speedup << "x" << endl;
        cout << "   - Efficiency: " << int(efficiency * 100) << "%" << endl;
        cout << "   - Solutions equal: " << (areEqual ? "Yes" : "No") << endl << endl;
    }

    // Clean up
    for (unsigned int i = 0; i < n; i++) {
        delete[] A[i];
    }
    delete[] A;
    delete[] b;
    if (runSequential) {
        delete[] sequentialBenchmark.result;
    }
    if (runParallel) {
        delete[] parallelBenchmark.result;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: <n> <threads> [runSequential] [runParallel]" << endl;
        return 1;
    }

    unsigned int n = atoi(argv[1]);
    unsigned int threads = atoi(argv[2]);

    bool runSequential = argc < 4 || atoi(argv[3]) == 1;
    bool runParallel = argc < 5 || atoi(argv[4]) == 1;

    srand(time(NULL)); // Seed the random number generator
    benchmark(n, threads, 10000, 1e-6, runSequential, runParallel);

    return 0;
}
