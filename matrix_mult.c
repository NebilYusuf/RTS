#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 100000
#define DIM 4
#define CONTROL_LOOP_BUDGET_MS 10.0
#define SUBSYSTEM_OVERHEAD_MS 3.9
#define FORCED_DELAY_MS 1.5
#define CYCLES 10

// === UNOPTIMIZED CODE: Reflecting Chapter 12 Anti-Patterns ===

// Redundant computation, memory access pattern issues, branching, no unrolling
void transform_sensor_points(float B[N][DIM], float A[DIM][DIM], float C[N][DIM]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < DIM; j++) {
            C[i][j] = 0.0f;
            for (int k = 0; k < DIM; k++) {
                // Inefficient access: repeatedly loading from memory
                float temp = B[i][k];
                if (temp > 1.0f) temp *= 0.99f;  // Unpredictable branch
                C[i][j] += temp * A[k][j];
            }
        }
    }
}

double get_elapsed_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1e6;
}

void sleep_for_ms(double ms) {
    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = (long)(ms * 1e6);
    nanosleep(&delay, NULL);
}

int main() {
    float B[N][DIM], C[N][DIM], A[DIM][DIM];
    struct timespec t_start, t_end;

    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
            A[i][j] = (i == j) ? 1.0f : 0.1f * (i + j);

    for (int i = 0; i < N; i++) {
        B[i][0] = i * 0.001f;
        B[i][1] = i * 0.002f;
        B[i][2] = i * 0.003f;
        B[i][3] = 1.0f;
    }

    printf("=== PX4 Loop Simulation (Unoptimized, Chapter 12 Anti-Patterns) ===\n");
    printf("Sensor Points: %d | Overhead: %.1f ms | Forced Delay: %.1f ms\n", N, SUBSYSTEM_OVERHEAD_MS, FORCED_DELAY_MS);
    printf("Budget: %.1f ms | Cycles: %d\n\n", CONTROL_LOOP_BUDGET_MS, CYCLES);

    for (int cycle = 0; cycle < CYCLES; cycle++) {
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        transform_sensor_points(B, A, C);
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        double transform_time = get_elapsed_ms(t_start, t_end);

        sleep_for_ms(FORCED_DELAY_MS);

        double total_time = transform_time + FORCED_DELAY_MS + SUBSYSTEM_OVERHEAD_MS;
        int met = total_time <= CONTROL_LOOP_BUDGET_MS;

        printf("Cycle %2d | Transform: %5.2f ms | Delay: %4.2f ms | Total: %5.2f ms | %s\n",
               cycle + 1, transform_time, FORCED_DELAY_MS, total_time, met ? "\xE2\x9C\x85 Met" : "\xE2\x9A\xA0 Miss");
    }

    return 0;
}