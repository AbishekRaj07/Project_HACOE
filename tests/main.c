#include <stdio.h>
#include <stdlib.h>

// [Target 1] High Math-to-Memory Ratio (Compute Bound)
void compute_scalar(int iterations) {
    double result = 0.0;
    // This loop creates a back-edge in the CFG
    for (int i = 0; i < iterations; i++) {
        result += (i * 3.14159) / 2.71828;
    }
    printf("Scalar Result: %f\n", result);
}

// [Target 2] High Memory-to-Math Ratio (Memory Bound)
void compute_memory(int size) {
    int *data = (int *)malloc(size * sizeof(int));
    if (!data) {
        printf("Memory allocation failed.\n");
        return; // Creates an early-exit branch in the CFG
    }
    
    // Memory write loop
    for (int i = 0; i < size; i++) {
        data[i] = i % 256;
    }
    
    // Memory read loop
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    
    printf("Memory Sum: %d\n", sum);
    free(data);
}

int main(int argc, char **argv) {
    printf("--- HACOE Standard Test Harness ---\n");
    
    // This conditional creates a distinct bifurcation (split) in the CFG
    if (argc > 1) {
        printf("Branch A Taken: Executing Memory Workload...\n");
        compute_memory(5000000);
    } else {
        printf("Branch B Taken: Executing Scalar Workload...\n");
        compute_scalar(5000000);
    }
    
    return 0;
}
