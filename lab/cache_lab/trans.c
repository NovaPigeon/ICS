/* 寿晨宸 2100012945*/
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
    REQUIRES(M > 0);
    REQUIRES(N > 0);
    int i, j, k, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    // i,j,k作为循环变量，tmp1~8存储相应的元素
    if (M == 32 && N == 32)
    {
        /*
        缓存的参数为: s=5 E=1 b=5
        说明缓存共有32组，每组一行，块大小为32字节，可以放下8个 int 类型变量
        经过观察，发现数组A，B起始地址所映射的组数相同，是以A，B中相同下标的元素映射到同一个组，
        所以当转置对角线上的元素时，会发生冲突不命中。
        故A，B中元素所映射的组数如下表所示：
        0 0 0 0 0 0 0 0 .... 3 3 3 3 3 3 3 3
        ....
        28 28 28 28 28  .... 31 31 31 31 31..
        0 0 0 0 0 0 0 0 .... .....
        ....
        也就是说，缓存可以存下数组前 8 行的所有数据，所以我们可以将数组分成 8*8 的块，减少冲突。
        同时考虑由于对角线上元素造成的冲突不命中。
        譬如，我们读入A(2,2)，再访问B(2,2), 此时，会发生冲突不命中，
        并将缓存中的一整行替换为B，此时，再读入A(2,3)，就会又发生不命中。
        (当i!=j时，由于访问的是不同的块，故没有这个问题)
        我们可以将一行8个元素一次性读完，以规避之。
        */
        for (i = 0; i < N; i += 8)
            for (j = 0; j < M; j += 8)
                for (k = 0; k < 8; ++k)
                {
                    tmp1 = A[i + k][j];
                    tmp2 = A[i + k][j + 1];
                    tmp3 = A[i + k][j + 2];
                    tmp4 = A[i + k][j + 3];
                    tmp5 = A[i + k][j + 4];
                    tmp6 = A[i + k][j + 5];
                    tmp7 = A[i + k][j + 6];
                    tmp8 = A[i + k][j + 7];
                    B[j][i + k] = tmp1;
                    B[j + 1][i + k] = tmp2;
                    B[j + 2][i + k] = tmp3;
                    B[j + 3][i + k] = tmp4;
                    B[j + 4][i + k] = tmp5;
                    B[j + 5][i + k] = tmp6;
                    B[j + 6][i + k] = tmp7;
                    B[j + 7][i + k] = tmp8;
                }
    }
    if (M == 64 && N == 64)
    {
        /*
        缓存此时只能存下数组前4行的所有元素，所以简单地使用8分块必然造成冲突
        但是，只使用4分块的话对于缓存的利用率就太低了，所以，需要借用临时变量进行8分块和4分块的组合。
        以一个 8*8 的块为操作的基本单元。
        将A，B矩阵的基本单元分块如下。
        A:A1 A2  B:B1 B2
          A3 A4    B3 B4
        1.将 A1,A2 转置后，放入B1,B2。（为了规避B1与B3的冲突）
        2.将B2写入B3的同时，将A3写入B2。（每一次，先写B2，再写B3，可保证每一次驱逐的块都不会再被使用）
        3.将A4写入B4（B4已在缓存中，直接写即可）
        */
        for (i = 0; i < N; i += 8)
            for (j = 0; j < M; j += 8)
            {
                for (k = 0; k < 4; ++k)
                {
                    tmp1 = A[i + k][j];
                    tmp2 = A[i + k][j + 1];
                    tmp3 = A[i + k][j + 2];
                    tmp4 = A[i + k][j + 3];
                    tmp5 = A[i + k][j + 4];
                    tmp6 = A[i + k][j + 5];
                    tmp7 = A[i + k][j + 6];
                    tmp8 = A[i + k][j + 7];
                    B[j][i + k] = tmp1;
                    B[j + 1][i + k] = tmp2;
                    B[j + 2][i + k] = tmp3;
                    B[j + 3][i + k] = tmp4; // 将A1存入B1
                    B[j][i + k + 4] = tmp5;
                    B[j + 1][i + k + 4] = tmp6;
                    B[j + 2][i + k + 4] = tmp7;
                    B[j + 3][i + k + 4] = tmp8; // 将A2存入B2
                }
                for (k = 0; k < 4; ++k)
                {
                    // 存储B2,A3的值
                    tmp1 = A[i + 4][j + k];
                    tmp2 = A[i + 5][j + k];
                    tmp3 = A[i + 6][j + k];
                    tmp4 = A[i + 7][j + k];
                    tmp5 = B[j + k][i + 4];
                    tmp6 = B[j + k][i + 5];
                    tmp7 = B[j + k][i + 6];
                    tmp8 = B[j + k][i + 7];
                    // 将A3写入B2
                    B[j + k][i + 4] = tmp1;
                    B[j + k][i + 5] = tmp2;
                    B[j + k][i + 6] = tmp3;
                    B[j + k][i + 7] = tmp4;
                    // 将B2(存储的值)写入B3
                    B[j + k + 4][i] = tmp5;
                    B[j + k + 4][i + 1] = tmp6;
                    B[j + k + 4][i + 2] = tmp7;
                    B[j + k + 4][i + 3] = tmp8;
                }
                for (k = 0; k < 4; ++k)
                {
                    // 将A4写入B4
                    tmp1 = A[i + k + 4][j + 4];
                    tmp2 = A[i + k + 4][j + 5];
                    tmp3 = A[i + k + 4][j + 6];
                    tmp4 = A[i + k + 4][j + 7];
                    B[j + 4][i + k + 4] = tmp1;
                    B[j + 5][i + k + 4] = tmp2;
                    B[j + 6][i + k + 4] = tmp3;
                    B[j + 7][i + k + 4] = tmp4;
                }
            }
    }
    if (N == 68 && M == 60)
    {
        // 分成8*8的块，然后再处理余数
        for (j = 0; j < 56; j += 8)
        {
            for (i = 0; i < 64; i += 8)
            {
                for (k = 0; k < 8; ++k)
                {
                    tmp1 = A[i + k][j];
                    tmp2 = A[i + k][j + 1];
                    tmp3 = A[i + k][j + 2];
                    tmp4 = A[i + k][j + 3];
                    tmp5 = A[i + k][j + 4];
                    tmp6 = A[i + k][j + 5];
                    tmp7 = A[i + k][j + 6];
                    tmp8 = A[i + k][j + 7];
                    B[j][i + k] = tmp1;
                    B[j + 1][i + k] = tmp2;
                    B[j + 2][i + k] = tmp3;
                    B[j + 3][i + k] = tmp4;
                    B[j + 4][i + k] = tmp5;
                    B[j + 5][i + k] = tmp6;
                    B[j + 6][i + k] = tmp7;
                    B[j + 7][i + k] = tmp8;
                }
            }
        }
        for (i = 64; i < N; ++i)
            for (j = 56; j < M; ++j)
            {
                tmp1 = A[i][j];
                B[j][i] = tmp1;
            }
        for (i = 0; i < N; ++i)
            for (j = 56; j < M; ++j)
            {
                tmp1 = A[i][j];
                B[j][i] = tmp1;
            }
        for (i = 64; i < N; ++i)
            for (j = 0; j < M; ++j)
            {
                tmp1 = A[i][j];
                B[j][i] = tmp1;
            }
    }
    ENSURES(is_transpose(M, N, A, B));
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

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
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

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
