#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 100000                     // Number of sensor points (Elios 3 LiDAR-scale)
#define DIM 4                        // 3D + homogeneous coord
#define CONTROL_LOOP_BUDGET_MS 10.0  // PX4 attitude loop at 100 Hz
#define SUBSYSTEM_OVERHEAD_MS 3.9    // Sensor + Filter + Control + Comm
#define CYCLES 10                    // Number of control loop iterations
#define SPIKE_JITTER_MS 2.0          // Optional processing spike for 1 out of 10 cycles

// Simulate sensor transformation: B * Aᵀ = C
void transform_sensor_points(float B[N][DIM], float A[DIM][DIM], float C[N][DIM]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < DIM; j++) {
            C[i][j] = 0.0f;
            for (int k = 0; k < DIM; k++) {
                float val = B[i][k];  // Intentional inefficient memory access
                C[i][j] += val * A[k][j];
            }
        }
    }
}

double get_elapsed_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1e6;
}

int main() {
    float B[N][DIM], C[N][DIM], A[DIM][DIM];
    struct timespec start, end;

    // Initialize transformation matrix A (slightly perturbed identity)
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
            A[i][j] = (i == j) ? 1.0f : 0.1f * (i + j);

    // Initialize sensor points (x, y, z, 1)
    for (int i = 0; i < N; i++) {
        B[i][0] = i * 0.001f;
        B[i][1] = i * 0.002f;
        B[i][2] = i * 0.003f;
        B[i][3] = 1.0f;
    }

    printf("=== Simulated Elios 3 PX4 Attitude Loop (%d Cycles) ===\n", CYCLES);
    printf("Subsystems = Sensor+Filter+Control+Comm (%.1fms)\n", SUBSYSTEM_OVERHEAD_MS);
    printf("Loop Budget = %.1fms | ±0.5ms jitter | 1/10 spike = +%.1fms\n\n",
           CONTROL_LOOP_BUDGET_MS, SPIKE_JITTER_MS);

    for (int cycle = 0; cycle < CYCLES; cycle++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        transform_sensor_points(B, A, C);
        clock_gettime(CLOCK_MONOTONIC, &end);

        double transform_time = get_elapsed_ms(start, end);
        double total_time = transform_time + SUBSYSTEM_OVERHEAD_MS;

        // Inject artificial 1-in-10 spike
        int is_spike = (cycle == 9);  // Last cycle
        if (is_spike) {
            total_time += SPIKE_JITTER_MS;
        }

        int met_deadline = total_time <= CONTROL_LOOP_BUDGET_MS;

        printf("Cycle %2d | Transform: %5.2f ms | Total: %5.2f ms | %s%s\n",
               cycle + 1,
               transform_time,
               total_time,
               met_deadline ? "✅ Met  " : "⚠️  Miss",
               is_spike ? " (+Spike)" : "");
    }

    return 0;
}
