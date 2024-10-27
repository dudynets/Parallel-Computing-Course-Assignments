#include <iostream> // Input/output stream
#include <ctime>    // Needed for seed the random number generator
#include <cstdlib>  // Needed for rand and srand
#include <thread>   // Needed for multithreading
#include <functional> // Needed to pass functions as arguments
#include <chrono>   // For timing
#include <cstring>  // For strlen

// Include OpenCL headers
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

using namespace std;

// Function to generate a random matrix
int** generateMatrix(int n, int m) {
    int** matrix = new int*[n];
    for (int i = 0; i < n; i++) {
        matrix[i] = new int[m];
        for (int j = 0; j < m; j++) {
            matrix[i][j] = rand() % 100;
        }
    }
    return matrix;
}

// Function to multiply a row by a column
int multiplyRowByColumn(int* row, int** matrix2, int colIndex, int m) {
    int sum = 0;
    for (int i = 0; i < m; i++) {
        sum += row[i] * matrix2[i][colIndex];
    }
    return sum;
}

// Sequential matrix multiplication
int** multiplyMatricesSequential(int** matrix1, int** matrix2, unsigned int n, unsigned int m, unsigned int l) {
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
int** multiplyMatricesParallel(int** matrix1, int** matrix2, unsigned int n, unsigned int m, unsigned int l, unsigned int numberOfThreads) {
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
        // The first remainingRows threads will take 1 more row to distribute the remainder
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

    delete[] threads;

    return result;
}

// Function to flatten a 2D matrix into a 1D array
int* flattenMatrix(int** matrix, int rows, int cols) {
    int* flatMatrix = new int[rows * cols];
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            flatMatrix[i * cols + j] = matrix[i][j];
        }
    }
    return flatMatrix;
}

// Function to unflatten a 1D array into a 2D matrix
int** unflattenMatrix(int* flatMatrix, int rows, int cols) {
    int** matrix = new int*[rows];
    for (int i = 0; i < rows; i++) {
        matrix[i] = new int[cols];
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = flatMatrix[i * cols + j];
        }
    }
    return matrix;
}

// OpenCL matrix multiplication
int** multiplyMatricesOpenCL(int** matrix1, int** matrix2, unsigned int n, unsigned int m, unsigned int l) {
    // Flatten the matrices
    int* flatA = flattenMatrix(matrix1, n, m);
    int* flatB = flattenMatrix(matrix2, m, l);
    int* flatC = new int[n * l]; // Result matrix

    // OpenCL variables
    cl_int err;
    cl_uint numPlatforms;
    cl_platform_id platform = NULL;
    cl_uint numDevices = 0;
    cl_device_id device = NULL;
    cl_context context = NULL;
    cl_command_queue queue = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;

    // Get the first platform
    err = clGetPlatformIDs(1, &platform, &numPlatforms);
    if (err != CL_SUCCESS) {
        cerr << "Error getting platform IDs: " << err << endl;
        exit(1);
    }

    // Get the first device (GPU)
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &numDevices);
    if (err != CL_SUCCESS) {
        cout << "   - Note: No GPU device found, trying CPU..." << endl;
        // If failed, try CPU
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, &numDevices);
        if (err != CL_SUCCESS) {
            cerr << "Error getting device IDs: " << err << endl;
            exit(1);
        }
    }

    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        cerr << "Error creating context: " << err << endl;
        exit(1);
    }

    // Create command queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        cerr << "Error creating command queue: " << err << endl;
        exit(1);
    }

    // Create program from source
    const char* kernelSource = R"CLC(
    __kernel void matrixMultiply(
        __global int* A,
        __global int* B,
        __global int* C,
        const int M,
        const int N,
        const int K
    ) {
        int row = get_global_id(0);
        int col = get_global_id(1);

        if (row < M && col < K) {
            int sum = 0;
            for (int i = 0; i < N; i++) {
                sum += A[row * N + i] * B[i * K + col];
            }
            C[row * K + col] = sum;
        }
    }
    )CLC";

    size_t kernelLength = strlen(kernelSource);
    program = clCreateProgramWithSource(context, 1, &kernelSource, &kernelLength, &err);
    if (err != CL_SUCCESS) {
        cerr << "Error creating program: " << err << endl;
        exit(1);
    }

    // Build program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        // Print build log
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = new char[log_size];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        cerr << "Error building program: " << err << endl;
        cerr << log << endl;
        delete[] log;
        exit(1);
    }

    // Create kernel
    kernel = clCreateKernel(program, "matrixMultiply", &err);
    if (err != CL_SUCCESS) {
        cerr << "Error creating kernel: " << err << endl;
        exit(1);
    }

    // Create buffers
    cl_mem bufferA = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n * m * sizeof(int), flatA, &err);
    if (err != CL_SUCCESS) {
        cerr << "Error creating buffer A: " << err << endl;
        exit(1);
    }

    cl_mem bufferB = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m * l * sizeof(int), flatB, &err);
    if (err != CL_SUCCESS) {
        cerr << "Error creating buffer B: " << err << endl;
        exit(1);
    }

    cl_mem bufferC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, n * l * sizeof(int), NULL, &err);
    if (err != CL_SUCCESS) {
        cerr << "Error creating buffer C: " << err << endl;
        exit(1);
    }

    // Set kernel arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufferA);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufferB);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufferC);
    int M = n;
    int N = m;
    int K = l;
    err |= clSetKernelArg(kernel, 3, sizeof(int), &M);
    err |= clSetKernelArg(kernel, 4, sizeof(int), &N);
    err |= clSetKernelArg(kernel, 5, sizeof(int), &K);
    if (err != CL_SUCCESS) {
        cerr << "Error setting kernel arguments: " << err << endl;
        exit(1);
    }

    // Execute kernel
    size_t globalWorkSize[2] = { n, l };
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        cerr << "Error enqueuing kernel: " << err << endl;
        exit(1);
    }

    // Read back result
    err = clEnqueueReadBuffer(queue, bufferC, CL_TRUE, 0, n * l * sizeof(int), flatC, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        cerr << "Error reading buffer C: " << err << endl;
        exit(1);
    }

    // Cleanup
    clReleaseMemObject(bufferA);
    clReleaseMemObject(bufferB);
    clReleaseMemObject(bufferC);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    // Unflatten the result matrix
    int** result = unflattenMatrix(flatC, n, l);

    // Free temporary buffers
    delete[] flatA;
    delete[] flatB;
    delete[] flatC;

    return result;
}

// Check if two matrices are equal
bool areMatricesEqual(int** matrix1, int** matrix2, int n, int m) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (matrix1[i][j] != matrix2[i][j]) {
                return false;
            }
        }
    }
    return true;
}

// Calculate speedup
double calculateSpeedup(double sequentialTime, double parallelTime) {
    if (parallelTime == 0) {
        return 0;
    }
    return sequentialTime / parallelTime;
}

// Calculate efficiency
double calculateEfficiency(double speedup, int numberOfThreads) {
    return speedup / numberOfThreads;
}

struct BenchmarkResult {
    long long time;
    int** result;
};

// Benchmarking function
BenchmarkResult benchmarkTime(function<int**()> function) {
    auto start = chrono::high_resolution_clock::now();
    int** result = function();
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    return { duration.count(), result };
}

void benchmark(unsigned int n, unsigned int m, unsigned int l, unsigned int numberOfThreads, bool runSequential = true, bool runParallel = true, bool runOpenCL = true) {
    cout << "Benchmark for " << n << "x" << m << " matrix with " << l << " result columns and " << numberOfThreads << " threads:" << endl;
    if (!runSequential) {
        cout << "- Skipping sequential algorithms" << endl;
    }
    if (!runParallel) {
        cout << "- Skipping parallel algorithms" << endl;
    }
    if (!runOpenCL) {
        cout << "- Skipping OpenCL algorithms" << endl;
    }

    cout << endl << "=====================" << endl << endl;

    cout << "- Generating matrices..." << endl << endl;
    int** matrix1 = generateMatrix(n, m);
    int** matrix2 = generateMatrix(m, l);

    BenchmarkResult multiplySequentialBenchmark;
    BenchmarkResult multiplyParallelBenchmark;
    BenchmarkResult multiplyOpenCLBenchmark;

    cout << "Press Enter to continue..." << endl;
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

    if (runOpenCL) {
        cout << "- Running OpenCL matrix multiplication:" << endl;
        multiplyOpenCLBenchmark = benchmarkTime([&]() {
            return multiplyMatricesOpenCL(matrix1, matrix2, n, m, l);
        });
        cout << "   - Time: " << multiplyOpenCLBenchmark.time << "ms" << endl;
    }

    cout << "=====================" << endl << endl;
    cout << "- Summary:" << endl;
    cout << "   - Matrix size: " << n << "x" << m << " to " << m << "x" << l << endl;
    cout << "   - Threads: " << numberOfThreads << endl;

    if (runSequential) {
        cout << "   - Sequential time: " << multiplySequentialBenchmark.time << "ms" << endl;
    }
    if (runParallel) {
        cout << "   - Parallel time: " << multiplyParallelBenchmark.time << "ms" << endl;
    }
    if (runOpenCL) {
        cout << "   - OpenCL time: " << multiplyOpenCLBenchmark.time << "ms" << endl;
    }

    if (runSequential && runParallel) {
        double speedup = calculateSpeedup(multiplySequentialBenchmark.time, multiplyParallelBenchmark.time);
        double efficiency = calculateEfficiency(speedup, numberOfThreads);
        cout << "   - Speedup (Parallel): " << speedup << "x" << endl;
        cout << "   - Efficiency (Parallel): " << int(efficiency * 100) << "%" << endl;
        bool areEqual = areMatricesEqual(multiplySequentialBenchmark.result, multiplyParallelBenchmark.result, n, l);
        cout << "   - Matrices equal (Sequential vs Parallel): " << (areEqual ? "Yes" : "No") << endl;
    }

    if (runSequential && runOpenCL) {
        double speedupOpenCL = calculateSpeedup(multiplySequentialBenchmark.time, multiplyOpenCLBenchmark.time);
        cout << "   - Speedup (OpenCL): " << speedupOpenCL << "x" << endl;
        bool areOpenCLEqual = areMatricesEqual(multiplySequentialBenchmark.result, multiplyOpenCLBenchmark.result, n, l);
        cout << "   - Matrices equal (Sequential vs OpenCL): " << (areOpenCLEqual ? "Yes" : "No") << endl;
    }

    // Clean up matrices
    for (unsigned int i = 0; i < n; i++) {
        delete[] matrix1[i];
    }
    delete[] matrix1;

    for (unsigned int i = 0; i < m; i++) {
        delete[] matrix2[i];
    }
    delete[] matrix2;

    if (runSequential) {
        for (unsigned int i = 0; i < n; i++) {
            delete[] multiplySequentialBenchmark.result[i];
        }
        delete[] multiplySequentialBenchmark.result;
    }

    if (runParallel) {
        for (unsigned int i = 0; i < n; i++) {
            delete[] multiplyParallelBenchmark.result[i];
        }
        delete[] multiplyParallelBenchmark.result;
    }

    if (runOpenCL) {
        for (unsigned int i = 0; i < n; i++) {
            delete[] multiplyOpenCLBenchmark.result[i];
        }
        delete[] multiplyOpenCLBenchmark.result;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        cout << "Usage: <n> <m> <l> <threads> [runSequential] [runParallel] [runOpenCL]" << endl;
        return 1;
    }

    unsigned int n = atoi(argv[1]);
    unsigned int m = atoi(argv[2]);
    unsigned int l = atoi(argv[3]);
    unsigned int threads = atoi(argv[4]);

    bool runSequential = argc < 6 || atoi(argv[5]) == 1;
    bool runParallel = argc < 7 || atoi(argv[6]) == 1;
    bool runOpenCL = argc < 8 || atoi(argv[7]) == 1;

    srand(time(NULL)); // Seed the random number generator
    benchmark(n, m, l, threads, runSequential, runParallel, runOpenCL);

    return 0;
}
