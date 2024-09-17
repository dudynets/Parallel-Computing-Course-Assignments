#include <iostream> // Input/output stream
#include <ctime> // Needed for seed the random number generator
#include <cstdlib> // Needed for rand and srand
#include <thread> // Needed for multithreading
#include <functional>  // Needed to pass functions as arguments

using namespace std;

// Напишіть програми обчислення множення двох матриць (послідовний та паралельний алгоритми).
// Порахуйте час роботи кожної з програм, обчисліть прискорення та ефективність роботи паралельного алгоритму.

// В матрицях розмірності (n,m) (m,l) робіть змінними, щоб легко змінювати величину матриці. 

// Кількість потоків k - також змінна величина. Програма повинна показувати час при послідовному способі
// виконання програми, а також при розпаралеленні на k потоків. 

// Зверніть увагу на випадки, коли розмірність матриці не кратна кількості потоків.!!!

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

int multiplyRowByColumn(int* row, int** matrix2, int colIndex, int m) {
    int sum = 0;
    for (int i = 0; i < m; i++) {
        sum += row[i] * matrix2[i][colIndex];
    }
    return sum;
}

// Sequential matrix multiplication
int** multiplyMatricesSequential(
    int** matrix1,
    int** matrix2,
    unsigned int n,
    unsigned int m,
    unsigned int l
) {
    int** result = new int*[n];
    for (unsigned int i = 0; i < n; i++) {
        result[i] = new int[l];
        for (unsigned int j = 0; j < l; j++) {
            result[i][j] = multiplyRowByColumn(matrix1[i], matrix2, j, m);
        }
    }
    return result;
}

// Parallel matrix multiplication
int** multiplyMatricesParallel(
    int** matrix1,
    int** matrix2,
    unsigned int n,
    unsigned int m,
    unsigned int l,
    unsigned int numberOfThreads
) {
    // No need to use more threads than rows
    if (numberOfThreads > n) {
        numberOfThreads = n;
    }

    int** result = new int*[n];
    for (unsigned int i = 0; i < n; i++) {
        result[i] = new int[l];
    }

    unsigned int rowsPerThread = n / numberOfThreads;
    unsigned int remainingRows = n % numberOfThreads;

    thread* threads = new thread[numberOfThreads];

    unsigned int currentRow = 0;

    for (unsigned int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
        // The first `remainingRows` threads will take 1 more row to distribute the remainder
        bool shouldTakeRemainder = threadIndex < remainingRows;

        unsigned int startRow = currentRow;
        unsigned int endRow = startRow + rowsPerThread + (shouldTakeRemainder ? 1 : 0);
        currentRow = endRow;

        // Creating a thread
        threads[threadIndex] = thread([=]() {
            for (unsigned int i = startRow; i < endRow; i++) {
                for (unsigned int j = 0; j < l; j++) {
                    result[i][j] = multiplyRowByColumn(matrix1[i], matrix2, j, m);
                }
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
    unsigned int l,
    unsigned int numberOfThreads,
    bool runSequential = true,
    bool runParallel = true
) {
    cout << "Benchmark for " << n << "x" << m << " matrix with " << l << " result columns and " << numberOfThreads << " threads:" << endl;
    if (!runSequential) {
        cout << "- Skipping sequential algorithms" << endl;
    }
    if (!runParallel) {
        cout << "- Skipping parallel algorithms" << endl;
    }

    cout << endl << "=====================" << endl << endl;

    cout << "- Generating matrices..." << endl << endl;
    int** matrix1 = generateMatrix(n, m);
    int** matrix2 = generateMatrix(m, l);

    BenchmarkResult multiplySequentialBenchmark;
    BenchmarkResult multiplyParallelBenchmark;
    cout << "Press any key to continue..." << endl;
    cin.get();
    cout << "=====================" << endl << endl;

    if (runSequential) {
        cout << "- Running sequential matrix multiplication:" << endl;
        multiplySequentialBenchmark = benchmarkTime([&]() {
            return multiplyMatricesSequential(matrix1, matrix2, n, m, l);
        });
        cout << "   - Time: " << multiplySequentialBenchmark.time << "ms" << endl;
    }

    if (runParallel) {
        cout << "- Running parallel matrix multiplication:" << endl;
        multiplyParallelBenchmark = benchmarkTime([&]() {
            return multiplyMatricesParallel(matrix1, matrix2, n, m, l, numberOfThreads);
        });
        cout << "   - Time: " << multiplyParallelBenchmark.time << "ms" << endl;
    }

    if (runSequential && runParallel) {
        cout << "=====================" << endl << endl;
        cout << "- Summary:" << endl;

        double speedup = calculateSpeedup(multiplySequentialBenchmark.time, multiplyParallelBenchmark.time);
        double efficiency = calculateEfficiency(speedup, numberOfThreads);
        bool areEqual = areMatricesEqual(multiplySequentialBenchmark.result, multiplyParallelBenchmark.result, n, l);

        cout << "   - Matrix size: " << n << "x" << m << " to " << m << "x" << l << endl;
        cout << "   - Threads: " << numberOfThreads << endl;
        cout << "   - Sequential time: " << multiplySequentialBenchmark.time << "ms" << endl;
        cout << "   - Parallel time: " << multiplyParallelBenchmark.time << "ms" << endl;
        cout << "   - Speedup: " << speedup << "x" << endl;
        cout << "   - Efficiency: " << int(efficiency * 100) << "%" << " (took " << multiplyParallelBenchmark.time << "ms vs " << multiplySequentialBenchmark.time / numberOfThreads << "ms ideal)" << endl;
        cout << "   - Matrices equal: " << (areEqual ? "Yes" : "No") << endl << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        cout << "Usage: <n> <m> <l> <threads> [runSequential] [runParallel]" << endl;
        return 1;
    }

    unsigned int n = atoi(argv[1]);
    unsigned int m = atoi(argv[2]);
    unsigned int l = atoi(argv[3]);
    unsigned int threads = atoi(argv[4]);

    bool runSequential = argc < 6 || atoi(argv[5]) == 1;
    bool runParallel = argc < 7 || atoi(argv[6]) == 1;

    srand(time(NULL)); // Seed the random number generator
    benchmark(n, m, l, threads, runSequential, runParallel);

    return 0;
}
