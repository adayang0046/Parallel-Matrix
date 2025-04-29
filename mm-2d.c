#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// Allocate matrix dynamically
int** allocateMatrix(int rows, int cols) {
    int** mat = malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++)
        mat[i] = malloc(cols * sizeof(int));
    return mat;
}

// Fill matrix w/ random #
void fillMatrix(int** mat, int rows, int cols) {
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            mat[i][j] = rand() % 10;
}

// Zero out matrix
void zeroMatrix(int** mat, int rows, int cols) {
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            mat[i][j] = 0;
}

void printSample(int** mat, int rows, int cols) {
    for (int i = 0; i < (rows < 2 ? rows : 2); i++) {
        for (int j = 0; j < (cols < 10 ? cols : 10); j++)
            printf("%d ", mat[i][j]);
        printf("\n");
    }
}

// Free allocated matrix
void freeMatrix(int** mat, int rows) {
    for (int i = 0; i < rows; i++)
        free(mat[i]);
    free(mat);
}

int main(int argc, char* argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Expect 3 inputs: programName , M, N, Q
    if (argc != 4) {
        if (rank == 0) printf("Usage: %s <M_rows> <N_inner> <Q_cols>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    int M = atoi(argv[1]), N = atoi(argv[2]), Q = atoi(argv[3]);
    int sqrt_P = sqrt(size);

    // Check if processes is a perfect square b/c algorithm requires a 2D grid of processes
    if (sqrt_P * sqrt_P != size || M % sqrt_P != 0 || Q % sqrt_P != 0) {
        if (rank == 0) printf("Processes must be perfect square, and M, Q divisible by sqrt(P).\n");
        MPI_Finalize();
        return 1;
    }

    // Calculate block sizes
    int block_rows = M / sqrt_P, block_cols = Q / sqrt_P;
    // Allocate matrices
    int** local_A = allocateMatrix(block_rows, N);
    int** local_B = allocateMatrix(N, block_cols);
    int** local_C = allocateMatrix(block_rows, block_cols);
    // Make full matrices only on rank 0
    int** A_full = NULL, **B_full = NULL, **C_full = NULL;

    if (rank == 0) {
        A_full = allocateMatrix(M, N);
        B_full = allocateMatrix(N, Q);
        C_full = allocateMatrix(M, Q);

        fillMatrix(A_full, M, N);
        fillMatrix(B_full, N, Q);
        zeroMatrix(C_full, M, Q);

        for (int p = 1; p < size; p++) {
            // Calculate the block position for process p
            int row_block = p / sqrt_P, col_block = p % sqrt_P;
            for (int i = 0; i < block_rows; i++)
                MPI_Send(A_full[row_block * block_rows + i], N, MPI_INT, p, 0, MPI_COMM_WORLD);
            for (int i = 0; i < N; i++)
                MPI_Send(&B_full[i][col_block * block_cols], block_cols, MPI_INT, p, 1, MPI_COMM_WORLD);
        }

        for (int i = 0; i < block_rows; i++)
            for (int j = 0; j < N; j++)
                local_A[i][j] = A_full[i][j];

        for (int i = 0; i < N; i++)
            for (int j = 0; j < block_cols; j++)
                local_B[i][j] = B_full[i][j];
    } else {
        // Receive matrix A and B from rank 0
        for (int i = 0; i < block_rows; i++)
            MPI_Recv(local_A[i], N, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // Receive matrix B from rank 0
        for (int i = 0; i < N; i++)
            MPI_Recv(local_B[i], block_cols, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    zeroMatrix(local_C, block_rows, block_cols);
    double start = MPI_Wtime();

    for (int i = 0; i < block_rows; i++)
        for (int j = 0; j < block_cols; j++)
            for (int k = 0; k < N; k++)
                local_C[i][j] += local_A[i][k] * local_B[k][j];

    double end = MPI_Wtime();

    if (rank == 0) {
        for (int i = 0; i < block_rows; i++)
            for (int j = 0; j < block_cols; j++)
                C_full[i][j] = local_C[i][j];

        for (int p = 1; p < size; p++) {
            // Calculate block position for process p
            int row_block = p / sqrt_P, col_block = p % sqrt_P;
            // Receive results from other processes
            for (int i = 0; i < block_rows; i++)
                MPI_Recv(C_full[row_block * block_rows + i] + col_block * block_cols, block_cols, MPI_INT, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        printf("2D MPI completed in %.4f seconds.\n", end - start);
        printSample(C_full, M, Q);

        freeMatrix(A_full, M);
        freeMatrix(B_full, N);
        freeMatrix(C_full, M);
    } else {
        // Send results back to rank 0
        for (int i = 0; i < block_rows; i++)
            MPI_Send(local_C[i], block_cols, MPI_INT, 0, 2, MPI_COMM_WORLD);
    }

    freeMatrix(local_A, block_rows);
    freeMatrix(local_B, N);
    freeMatrix(local_C, block_rows);
    MPI_Finalize();
    return 0;
}