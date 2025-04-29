#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// Allocate 2D matrix dynamically
int** allocateMatrix(int rows, int cols) {
    int** mat = malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        mat[i] = malloc(cols * sizeof(int));
    }
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
    int max_rows = rows < 2 ? rows : 2;
    int max_cols = cols < 10 ? cols : 10;

    for (int i = 0; i < max_rows; i++) {
        for (int j = 0; j < max_cols; j++) {
            printf("%d ", mat[i][j]);
        }
        printf("\n");
    }
}

// Free matrix
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

    if (argc != 4) {
        if (rank == 0) printf("Usage: %s <M_rows> <N_inner> <Q_cols>\n", argv[0]);
	MPI_Finalize();
        return 1;
    }

    int M = atoi(argv[1]);  // Rows of A and C
    int N = atoi(argv[2]);  // Columns of A, Rows of B
    int Q = atoi(argv[3]);  // Columns of B and C

    if (M % size != 0) {
        if (rank == 0) printf("Error: Rows (%d) must be divisible by processes (%d)\n", M, size);
	fflush(stdout);
        MPI_Finalize();
        return 1;
    }

    int rows_per_proc = M / size;

    // Initalize matrices; B gets allocated on all processes, A and C only on rank 0
    int** A = NULL;
    int** B = allocateMatrix(N, Q);
    int** C = NULL;

    // Smaller matrices for local computation for rows per process
    int** local_A = allocateMatrix(rows_per_proc, N);
    int** local_C = allocateMatrix(rows_per_proc, Q);

    if (rank == 0) {
        A = allocateMatrix(M, N);
        C = allocateMatrix(M, Q);

        fillMatrix(A, M, N);
        fillMatrix(B, N, Q);
        zeroMatrix(C, M, Q);

        // Send A and B to other processes
        for (int p = 1; p < size; p++) {
            for (int i = 0; i < rows_per_proc; i++)
                MPI_Send(A[p * rows_per_proc + i], N, MPI_INT, p, 0, MPI_COMM_WORLD);
        }

        // Send B to all processes
        for (int p = 1; p < size; p++) {
            for (int i = 0; i < N; i++)
                MPI_Send(B[i], Q, MPI_INT, p, 1, MPI_COMM_WORLD);
        }

        // Copy localA from A for rank 0
        for (int i = 0; i < rows_per_proc; i++)
            for (int j = 0; j < N; j++)
                local_A[i][j] = A[i][j];

    } else {
        // Receive A and B from rank 0
        for (int i = 0; i < rows_per_proc; i++)
            MPI_Recv(local_A[i], N, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // Receive B from rank 0
        for (int i = 0; i < N; i++)
            MPI_Recv(B[i], Q, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    // zero out local_C for all processes
    zeroMatrix(local_C, rows_per_proc, Q);

    double start = MPI_Wtime();

    for (int i = 0; i < rows_per_proc; i++)
        for (int j = 0; j < Q; j++)
            for (int k = 0; k < N; k++)
                local_C[i][j] += local_A[i][k] * B[k][j];

    double end = MPI_Wtime();

    if (rank == 0) {
        for (int i = 0; i < rows_per_proc; i++)
            for (int j = 0; j < Q; j++)
                C[i][j] = local_C[i][j];

        for (int p = 1; p < size; p++) {
            for (int i = 0; i < rows_per_proc; i++)
                // C is received from each process
                MPI_Recv(C[p * rows_per_proc + i], Q, MPI_INT, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        printf("MPI 1D Matrix Multiplication (Rectangular) completed in %.4f seconds.\n", end - start);
        printf("First few elements of matrix C:\n");
        printSample(C, M, Q);

        freeMatrix(A, M);
        freeMatrix(C, M);
    } else {
        for (int i = 0; i < rows_per_proc; i++)
            // local_C[i] is sent back to rank 0
            MPI_Send(local_C[i], Q, MPI_INT, 0, 2, MPI_COMM_WORLD);
    }

    freeMatrix(B, N);
    freeMatrix(local_A, rows_per_proc);
    freeMatrix(local_C, rows_per_proc);

    MPI_Finalize();
    return 0;
}