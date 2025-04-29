#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Allocate 2D matrix dynamically
int** allocateMatrix(int rows, int cols) {
    int** mat = malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++)
        mat[i] = malloc(cols * sizeof(int));
    return mat;
}

// Fill matrix with random numbers (0-9)
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

// Print first 2 rows and 10 columns
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
    if (argc != 4) {
        printf("Usage: %s <M_rows> <N_inner> <Q_cols>\n", argv[0]);
        return 1;
    }

    int M = atoi(argv[1]);  // Rows of A and C
    int N = atoi(argv[2]);  // Columns of A, Rows of B
    int Q = atoi(argv[3]);  // Columns of B and C

    if (M <= 0 || N <= 0 || Q <= 0) {
        printf("Invalid matrix dimensions.\n");
        return 1;
    }

    int** A = allocateMatrix(M, N);
    int** B = allocateMatrix(N, Q);
    int** C = allocateMatrix(M, Q);

    fillMatrix(A, M, N);
    fillMatrix(B, N, Q);
    zeroMatrix(C, M, Q);

    clock_t start = clock();

    for (int i = 0; i < M; i++)
        for (int j = 0; j < Q; j++)
            for (int k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];

    clock_t end = clock();
    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Serial Matrix Multiplication completed in %.4f seconds.\n", time_taken);
    printf("First few elements of matrix C:\n");
    printSample(C, M, Q);

    freeMatrix(A, M);
    freeMatrix(B, N);
    freeMatrix(C, M);

    return 0;
}
