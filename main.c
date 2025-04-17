#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <time.h>

int main() {
    struct timespec start, end;

    // Start the timer
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Simulate a heavy computation
    volatile long long sum = 0;
    for (long long i = 0; i < 100000000; i++) {
        sum += i;
    }

    // End the timer
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate elapsed time in seconds
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Execution time: %f seconds\n", elapsed);
    printf("Computation result: %lld\n", sum);

    return 0;
}
