#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int** generateMatrix(int rows, int cols) {
    int** matrix = (int**)malloc(rows * sizeof(int*));
    int i, j;
    for (i = 0; i < rows; i++) {
        matrix[i] = (int*)malloc(cols * sizeof(int));
        for (j = 0; j < cols; j++) {
            matrix[i][j] = rand() % 100;
        }
    }
    return matrix;
}

void freeMatrix(int** matrix, int rows) {
    int i;
    for (i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

int** multiplyMatricesSequential(int** matrix1, int** matrix2, int n, int m, int l) {
    int** result = generateMatrix(n, l);
    int i, j, k;
    for (i = 0; i < n; i++) {
        for (j = 0; j < l; j++) {
            result[i][j] = 0;
            for (k = 0; k < m; k++) {
                result[i][j] += matrix1[i][k] * matrix2[k][j];
            }
        }
    }
    return result;
}

int** multiplyMatricesParallel(int** matrix1, int** matrix2, int n, int m, int l, int rank, int size) {
    int rowsPerProcess = n / size;
    int extraRows = n % size;
    int startRow = rank * rowsPerProcess + (rank < extraRows ? rank : extraRows);
    int endRow = startRow + rowsPerProcess + (rank < extraRows ? 1 : 0);
    
    int localRowCount = endRow - startRow;
    int** localResult = generateMatrix(localRowCount, l);

    int i, j, k;
    for (i = 0; i < localRowCount; i++) {
        for (j = 0; j < l; j++) {
            localResult[i][j] = 0;
            for (k = 0; k < m; k++) {
                localResult[i][j] += matrix1[startRow + i][k] * matrix2[k][j];
            }
        }
    }

    return localResult;
}

void gatherResults(int** result, int** localResult, int n, int l, int rank, int size) {
    int rowsPerProcess = n / size;
    int extraRows = n % size;
    int startRow = rank * rowsPerProcess + (rank < extraRows ? rank : extraRows);
    int localRowCount = rowsPerProcess + (rank < extraRows ? 1 : 0);

    int i, j, p;
    if (rank == 0) {
        for (i = 0; i < localRowCount; i++) {
            for (j = 0; j < l; j++) {
                result[startRow + i][j] = localResult[i][j];
            }
        }
        
        for (p = 1; p < size; p++) {
            int pStartRow = p * rowsPerProcess + (p < extraRows ? p : extraRows);
            int pRowCount = rowsPerProcess + (p < extraRows ? 1 : 0);
            for (i = 0; i < pRowCount; i++) {
                MPI_Recv(result[pStartRow + i], l, MPI_INT, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    } else {
        for (i = 0; i < localRowCount; i++) {
            MPI_Send(localResult[i], l, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 5) {
        if (rank == 0) printf("Usage: <n> <m> <l> <threads> [runSequential] [runParallel]\n");
        MPI_Finalize();
        return 1;
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    int l = atoi(argv[3]);
    int threads = atoi(argv[4]);
    int runSequential = argc < 6 || atoi(argv[5]);
    int runParallel = argc < 7 || atoi(argv[6]);

    srand(time(NULL));
    int** matrix1 = generateMatrix(n, m);
    int** matrix2 = generateMatrix(m, l);
    int** sequentialResult = NULL;
    int** parallelResult = NULL;
    double sequentialTime = 0.0, parallelTime = 0.0;

    if (rank == 0 && runSequential) {
        clock_t start = clock();
        sequentialResult = multiplyMatricesSequential(matrix1, matrix2, n, m, l);
        clock_t end = clock();
        sequentialTime = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    }

    MPI_Bcast(&(matrix2[0][0]), m * l, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    clock_t parallelStart = clock();

    int** localResult = multiplyMatricesParallel(matrix1, matrix2, n, m, l, rank, size);
    if (rank == 0) parallelResult = generateMatrix(n, l);
    gatherResults(parallelResult, localResult, n, l, rank, size);

    MPI_Barrier(MPI_COMM_WORLD);
    clock_t parallelEnd = clock();

    if (rank == 0 && runParallel) {
        parallelTime = (double)(parallelEnd - parallelStart) / CLOCKS_PER_SEC * 1000;
        double speedup = sequentialTime / parallelTime;
        double efficiency = speedup / threads * 100;

        int i, j, matricesEqual = 1;
        if (runSequential) {
            for (i = 0; i < n && matricesEqual; i++) {
                for (j = 0; j < l; j++) {
                    if (sequentialResult[i][j] != parallelResult[i][j]) {
                        matricesEqual = 0;
                        break;
                    }
                }
            }
        }

        printf("   - Matrix size: %dx%d to %dx%d\n", n, m, m, l);
        printf("   - Threads: %d\n", threads);
        printf("   - Sequential time: %.2f ms\n", sequentialTime);
        printf("   - Parallel time: %.2f ms\n", parallelTime);
        printf("   - Speedup: %.2fx\n", speedup);
        printf("   - Efficiency: %.2f%%\n", efficiency);
        printf("   - Matrices equal: %s\n\n", matricesEqual ? "Yes" : "No");
    }

    if (rank == 0) {
        freeMatrix(matrix1, n);
        freeMatrix(matrix2, m);
        if (sequentialResult) freeMatrix(sequentialResult, n);
        if (parallelResult) freeMatrix(parallelResult, n);
    }
    freeMatrix(localResult, (n / size) + (rank < (n % size) ? 1 : 0));

    MPI_Finalize();
    return 0;
}
