/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp, k;
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    if (M == 32 && N == 32){
        for (j = 0; j < 32; j += 8)
            for (i = 0; i < 32; i ++){
                tmp0 = A[i][j + 0];
                tmp1 = A[i][j + 1];
                tmp2 = A[i][j + 2];
                tmp3 = A[i][j + 3];
                tmp4 = A[i][j + 4];
                tmp5 = A[i][j + 5];
                tmp6 = A[i][j + 6];
                tmp7 = A[i][j + 7];
                B[j + 0][i] = tmp0;
                B[j + 1][i] = tmp1;
                B[j + 2][i] = tmp2;
                B[j + 3][i] = tmp3;
                B[j + 4][i] = tmp4;
                B[j + 5][i] = tmp5;
                B[j + 6][i] = tmp6;
                B[j + 7][i] = tmp7;
            }
    }
    else if (M == 64 && N == 64){
        for (j = 0; j < 64; j += 8)
            for (i = 0; i < 64; i += 8){
                for (k = i; k < i + 4; k++){
                    tmp0 = A[k][j + 0];
                    tmp1 = A[k][j + 1];
                    tmp2 = A[k][j + 2];
                    tmp3 = A[k][j + 3];
                    tmp4 = A[k][j + 4];
                    tmp5 = A[k][j + 5];
                    tmp6 = A[k][j + 6];
                    tmp7 = A[k][j + 7];
                    B[j + 0][k + 0] = tmp0;
                    B[j + 1][k + 0] = tmp1;
                    B[j + 2][k + 0] = tmp2;
                    B[j + 3][k + 0] = tmp3;
                    B[j + 0][k + 4] = tmp4;
                    B[j + 1][k + 4] = tmp5;
                    B[j + 2][k + 4] = tmp6;
                    B[j + 3][k + 4] = tmp7;
                }
                for (int k = j; k < j + 4; k++){
                    tmp0 = A[i + 4][k];
                    tmp1 = A[i + 5][k];
                    tmp2 = A[i + 6][k];
                    tmp3 = A[i + 7][k];
                    tmp4 = B[k][i + 4];
                    tmp5 = B[k][i + 5];
                    tmp6 = B[k][i + 6];
                    tmp7 = B[k][i + 7];

                    B[k][i + 4] = tmp0;
                    B[k][i + 5] = tmp1;
                    B[k][i + 6] = tmp2;
                    B[k][i + 7] = tmp3;
                    B[k + 4][i + 0] = tmp4;
                    B[k + 4][i + 1] = tmp5;
                    B[k + 4][i + 2] = tmp6;
                    B[k + 4][i + 3] = tmp7;
                }

                for (int k = j + 4; k < j + 8; k++){
                    tmp0 = A[i + 4][k];
                    tmp1 = A[i + 5][k];
                    tmp2 = A[i + 6][k];
                    tmp3 = A[i + 7][k];

                    B[k][i + 4] = tmp0;
                    B[k][i + 5] = tmp1;
                    B[k][i + 6] = tmp2;
                    B[k][i + 7] = tmp3;
                }
            }

    }
    else if (M == 60 && N == 68){
        for (i = 0; i < 64; i += 8)
            for (j = 0; j < 56; j += 8)
                for (k = j; k < j + 8; k ++){
                    tmp0 = A[i + 0][k];
                    tmp1 = A[i + 1][k];
                    tmp2 = A[i + 2][k];
                    tmp3 = A[i + 3][k];
                    tmp4 = A[i + 4][k];
                    tmp5 = A[i + 5][k];
                    tmp6 = A[i + 6][k];
                    tmp7 = A[i + 7][k];

                    B[k][i + 0] = tmp0;
                    B[k][i + 1] = tmp1;
                    B[k][i + 2] = tmp2;
                    B[k][i + 3] = tmp3;
                    B[k][i + 4] = tmp4;
                    B[k][i + 5] = tmp5;
                    B[k][i + 6] = tmp6;
                    B[k][i + 7] = tmp7;
                }
        i = 64;
        for (j = 0; j < 60; j ++){
            tmp0 = A[i + 0][j];
            tmp1 = A[i + 1][j];
            tmp2 = A[i + 2][j];
            tmp3 = A[i + 3][j];
            B[j][i + 0] = tmp0;
            B[j][i + 1] = tmp1;
            B[j][i + 2] = tmp2;
            B[j][i + 3] = tmp3;
        }
        j = 56;
        for (i = 0; i < 64; i++){
            tmp0 = A[i][j + 0];
            tmp1 = A[i][j + 1];
            tmp2 = A[i][j + 2];
            tmp3 = A[i][j + 3];
            B[j + 0][i] = tmp0;
            B[j + 1][i] = tmp1;
            B[j + 2][i] = tmp2;
            B[j + 3][i] = tmp3;
        }
    }
    else{
        for (i = 0; i < N; i++) 
            for (j = 0; j < M; j++) {
                tmp = A[i][j];
                B[j][i] = tmp;
            }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

 /*
  * trans - A simple baseline transpose function, not optimized for the cache.
  */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

