#include <iostream> // Input/output stream
#include <ctime> // Needed for seed the random number generator
#include <cstdlib> // Needed for rand and srand
#include <thread> // Needed for multithreading
#include <functional>  // Needed to pass functions as arguments

using namespace std;

// Напишіть програми обчислення суми та різниці двох матриць (послідовний та паралельний алгоритми).
// Порахуйте час роботи кожної з програм, обчисліть прискорення та ефективність роботи паралельного алгоритму.
// 
// В матрицях розмірності (n,m) робіть змінними, щоб легко змінювати величину матриці. 
// 
// Кількість потоків k - також змінна величина. Програма повинна показувати час при послідовному способі
// виконання програми, а також при розпаралеленні на k потоків. 
// 
// Зверніть увагу на випадки, коли розмірність матриці не кратна кількості потоків!


int** generateMatrix(
    int n,
    int m
) {
    int** matrix = new int*[n];

    for (int i = 0; i < n; i++) {
        matrix[i] = new int[m];

        for (int j = 0; j < m; j++) {
            matrix[i][j] = rand() % 100;
        }
    }

    return matrix;
}

int* sumVectors(
    int* vector1,
    int* vector2,
    int n
) {
    int* result = new int[n];

    for (int i = 0; i < n; i++) {
        result[i] = vector1[i] + vector2[i];
    }

    return result;
}

int* subtractVectors(
    int* vector1,
    int* vector2,
    int n
) {
    int* result = new int[n];

    for (int i = 0; i < n; i++) {
        result[i] = vector1[i] - vector2[i];
    }

    return result;
}

int** computeMatricesSequential(
    int** matrix1,
    int** matrix2,
    function<int*(int*, int*, int)> operation,
    unsigned int n,
    unsigned int m
) {
    int** result = new int*[n];

    for (unsigned int i = 0; i < n; i++) {
        result[i] = operation(matrix1[i], matrix2[i], m);
    }

    return result;
}

int** computeMatricesParallel(
    int** matrix1,
    int** matrix2,
    function<int*(int*, int*, int)> operation,
    unsigned int n,
    unsigned int m,
    unsigned int numberOfThreads
) {
    // No need to use more threads than rows
    if (numberOfThreads > n) {
        numberOfThreads = n;
    }

    int** result = new int*[n];

    unsigned int rowsPerThread = n / numberOfThreads;
    unsigned int remainingRows = n % numberOfThreads; // This will be distributed among the first threads

    thread* threads = new thread[numberOfThreads];

    unsigned int currentRow = 0;

    for (unsigned int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
        // The first `remainingRows` threads will take 1 more row to distribute the remainder
        bool shouldTakeRemainder = threadIndex < remainingRows;

        unsigned int startRow = currentRow;
        unsigned int endRow = startRow + rowsPerThread + (shouldTakeRemainder ? 1 : 0); // Exclusive, so we don't need to subtract 1
        currentRow = endRow;

        // Creating a thread
        threads[threadIndex] = thread([=]() {
            for (unsigned int i = startRow; i < endRow; i++) {
                result[i] = operation(matrix1[i], matrix2[i], m);
            }
        });
    }

    // Starting the threads
    for (unsigned int i = 0; i < numberOfThreads; i++) {
        threads[i].join();
    }

    return result;
}

// Check if two matrices are equal, for testing purposes
bool areMatricesEqual(
    int** matrix1,
    int** matrix2,
    int n,
    int m
) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (matrix1[i][j] != matrix2[i][j]) {
                return false;
            }
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
    return speedup / numberOfThreads;
}

struct BenchmarkResult {
    long long time;
    int** result;
};

BenchmarkResult benchmarkTime(
    function<int**()> function
) {
    auto start = chrono::high_resolution_clock::now();
    int** result = function();
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    return { duration.count(), result };
}

void benchmark(
    unsigned int n,
    unsigned int m,
    unsigned int numberOfThreads,
    bool runSequential = true,
    bool runParallel = true
) {
    cout << "Benchmark for " << n << "x" << m << " matrix with " << numberOfThreads << " threads:" << endl;
    if (!runSequential) {
        cout << "- Skipping sequential algorithms" << endl;
    }
    if (!runParallel) {
        cout << "- Skipping parallel algorithms" << endl;
    }

    cout << endl << "=====================" << endl << endl;

    cout << "- Generating matrices..." << endl << endl;
    int** matrix1 = generateMatrix(n, m);
    int** matrix2 = generateMatrix(n, m);

    BenchmarkResult sumSequentialBenchmark;
    BenchmarkResult sumParallelBenchmark;

    BenchmarkResult subtractSequentialBenchmark;
    BenchmarkResult subtractParallelBenchmark;

    cout << "Press any key to continue..." << endl;
    cin.get();
    cout << "=====================" << endl << endl;

    if (runSequential) {
        cout << "- Running sequential algorithms:" << endl;
        
        sumSequentialBenchmark = benchmarkTime([&]() {
            return computeMatricesSequential(matrix1, matrix2, sumVectors, n, m);
        });
        cout << "   - Sum: " << sumSequentialBenchmark.time << "ms" << endl;

        subtractSequentialBenchmark = benchmarkTime([&]() {
            return computeMatricesSequential(matrix1, matrix2, subtractVectors, n, m);
        });
        cout << "   - Subtract: " << subtractSequentialBenchmark.time << "ms" << endl << endl;
    }

    if (runParallel) {
        cout << "- Running parallel algorithms:" << endl;

        sumParallelBenchmark = benchmarkTime([&]() {
            return computeMatricesParallel(matrix1, matrix2, sumVectors, n, m, numberOfThreads);
        });
        cout << "   - Sum: " << sumParallelBenchmark.time << "ms" << endl;

        subtractParallelBenchmark = benchmarkTime([&]() {
            return computeMatricesParallel(matrix1, matrix2, subtractVectors, n, m, numberOfThreads);
        });
        cout << "   - Subtract: " << subtractParallelBenchmark.time << "ms" << endl << endl;
    }

    if (runSequential && runParallel) {
        cout << "=====================" << endl << endl;
        cout << "- Summary:" << endl;

        cout << "   - Matrix size: " << n << "x" << m << endl;
        cout << "   - Threads: " << numberOfThreads << endl << endl;

        double speedupSum = calculateSpeedup(sumSequentialBenchmark.time, sumParallelBenchmark.time);
        double efficiencySum = calculateEfficiency(speedupSum, numberOfThreads);
        bool areEqualSum = areMatricesEqual(sumSequentialBenchmark.result, sumParallelBenchmark.result, n, m);

        cout << "   - Sum:" << endl;
        cout << "      - Sequential time: " << sumSequentialBenchmark.time << "ms" << endl;
        cout << "      - Parallel time: " << sumParallelBenchmark.time << "ms" << endl;
        cout << "      - Speedup: " << speedupSum << "x" << endl;
        cout << "      - Efficiency: " << int(efficiencySum * 100) << "%" << " (took " << sumParallelBenchmark.time << "ms vs " << sumSequentialBenchmark.time / numberOfThreads << "ms ideal)" << endl;
        cout << "      - Matrices equal: " << (areEqualSum ? "Yes" : "No") << endl << endl;

        double speedupSubtract = calculateSpeedup(subtractSequentialBenchmark.time, subtractParallelBenchmark.time);
        double efficiencySubtract = calculateEfficiency(speedupSubtract, numberOfThreads);
        bool areEqualSubtract = areMatricesEqual(subtractSequentialBenchmark.result, subtractParallelBenchmark.result, n, m);

        cout << "   - Subtract:" << endl;
        cout << "      - Sequential time: " << subtractSequentialBenchmark.time << "ms" << endl;
        cout << "      - Parallel time: " << subtractParallelBenchmark.time << "ms" << endl;
        cout << "      - Speedup: " << speedupSubtract << "x" << endl;
        cout << "      - Efficiency: " << int(efficiencySubtract * 100) << "%" << " (took " << subtractParallelBenchmark.time << "ms vs " << subtractSequentialBenchmark.time / numberOfThreads << "ms ideal)" << endl;
        cout << "      - Matrices equal: " << (areEqualSum ? "Yes" : "No") << endl << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage: <n> <m> <threads> [runSequential] [runParallel]" << endl;
        return 1;
    }

    unsigned int n = atoi(argv[1]);
    unsigned int m = atoi(argv[2]);
    unsigned int threads = atoi(argv[3]);

    bool runSequential = argc < 5 || atoi(argv[4]) == 1;
    bool runParallel = argc < 6 || atoi(argv[5]) == 1;

    srand(time(NULL)); // Seed the random number generator
    benchmark(n, m, threads, runSequential, runParallel);

    return 0;
}
