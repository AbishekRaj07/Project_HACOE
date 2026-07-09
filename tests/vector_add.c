#include <stdio.h>
#include <stdlib.h>

#define N 10000000

int main() {
    float *A = (float*)malloc(N * sizeof(float));
    float *B = (float*)malloc(N * sizeof(float));
    float *C = (float*)malloc(N * sizeof(float));

    // Initialize arrays
    for(int i = 0; i < N; i++) {
        A[i] = 1.0f;
        B[i] = 2.0f;
    }

    // The loop to be vectorized
    for(int i = 0; i < N; i++) {
        C[i] = A[i] + B[i];
    }

    printf("Result: %f\n", C[0]);

    free(A); free(B); free(C);
    return 0;
}
