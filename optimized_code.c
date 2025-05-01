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

// === Technique 1: Avoid Unnecessary Branches ===
void transform_sensor_points_no_branch(float B[N][DIM], float A[DIM][DIM], float C[N][DIM]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < DIM; j++) {
            C[i][j] = 0.0f;
            for (int k = 0; k < DIM; k++) {
                float temp = B[i][k];  // No branch
                C[i][j] += temp * A[k][j];
            }
        }
    }
}

// === Technique 2: Minimize Redundant Computation ===
void transform_sensor_points_cached_B(float B[N][DIM], float A[DIM][DIM], float C[N][DIM]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < DIM; j++) {
            C[i][j] = 0.0f;
            for (int k = 0; k < DIM; k++) {
                float temp = B[i][k];  // Cached here
                if (temp > 1.0f) temp *= 0.99f;
                C[i][j] += temp * A[k][j];
            }
        }
    }
}

// === Technique 4: Loop Unrolling ===
void transform_sensor_points_unrolled(float B[N][DIM], float A[DIM][DIM], float C[N][DIM]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < DIM; j++) {
            float temp0 = B[i][0];
            float temp1 = B[i][1];
            float temp2 = B[i][2];
            float temp3 = B[i][3];

            if (temp0 > 1.0f) temp0 *= 0.99f;
            if (temp1 > 1.0f) temp1 *= 0.99f;
            if (temp2 > 1.0f) temp2 *= 0.99f;
            if (temp3 > 1.0f) temp3 *= 0.99f;

            C[i][j] = temp0 * A[0][j] + temp1 * A[1][j] + temp2 * A[2][j] + temp3 * A[3][j];
        }
    }
}

// === Technique 5: Improve Memory Access Locality ===
void transform_sensor_points_locality(float B[N][DIM], float A[DIM][DIM], float C[N][DIM]) {
    float A_T[DIM][DIM];
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
            A_T[j][i] = A[i][j];

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < DIM; j++) {
            C[i][j] = 0.0f;
            for (int k = 0; k < DIM; k++) {
                float temp = B[i][k];
                if (temp > 1.0f) temp *= 0.99f;
                C[i][j] += temp * A_T[j][k];
            }
        }
    }
}

// === Timing Helpers ===
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

// === Main Loop to Compare All Optimizations ===
int main() {
    float B[N][DIM], C[N][DIM], A[DIM][DIM];
    struct timespec t_start, t_end;

    // Initialize matrix A and B
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
            A[i][j] = (i == j) ? 1.0f : 0.1f * (i + j);

    for (int i = 0; i < N; i++) {
        B[i][0] = i * 0.001f;
        B[i][1] = i * 0.002f;
        B[i][2] = i * 0.003f;
        B[i][3] = 1.0f;
    }

    // Function pointers for each optimization
    void (*functions[])(float[N][DIM], float[DIM][DIM], float[N][DIM]) = {
        transform_sensor_points_no_branch,
        transform_sensor_points_cached_B,
        transform_sensor_points_unrolled,
        transform_sensor_points_locality
    };

    const char* labels[] = {
        "No Branches",
        "Cached B[i][k]",
        "Loop Unrolling",
        "Improved Locality"
    };

    for (int f = 0; f < 4; f++) {
        printf("=== Optimization: %s ===\n", labels[f]);
        for (int cycle = 0; cycle < CYCLES; cycle++) {
            clock_gettime(CLOCK_MONOTONIC, &t_start);
            functions[f](B, A, C);
            clock_gettime(CLOCK_MONOTONIC, &t_end);
            double transform_time = get_elapsed_ms(t_start, t_end);

            sleep_for_ms(FORCED_DELAY_MS);

            double total_time = transform_time + FORCED_DELAY_MS + SUBSYSTEM_OVERHEAD_MS;
            int met = total_time <= CONTROL_LOOP_BUDGET_MS;

            printf("Cycle %2d | Transform: %5.2f ms | Delay: %4.2f ms | Total: %5.2f ms | %s\n",
                   cycle + 1, transform_time, FORCED_DELAY_MS, total_time, met ? "✓ Met" : "⚠ Miss");
        }
        printf("\n");
    }

    return 0;
}
