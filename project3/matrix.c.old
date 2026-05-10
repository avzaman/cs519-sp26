#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>

#define SYS_SET_INACTIVE  449
#define BLOCK             64      /* tile size tuned to fit A+B_T+C tiles in L1 cache */
#define DEFAULT_N         10000

static int    N           = DEFAULT_N;
static int    num_threads;
static int    cooperative = 0;

/* flat row-major heap arrays; A[i][j] = A[i*N+j]
 * heap allocation required: four 10000x10000 float arrays = ~1.6GB total */
static float *A, *B, *B_T, *C;

/* synchronizes threads between transpose and multiply phases */
static pthread_barrier_t barrier;

typedef struct {
    int  id;
    int  row_start;       /* first row assigned to this thread */
    int  row_end;         /* one past the last row assigned */
    long involuntary_sw;  /* preemptions recorded after work completes */
} thread_arg_t;

/* reads a named scalar field from /proc/self/task/<tid>/sched */
static double read_sched_field(pid_t tid, const char *field)
{
    char path[64], line[256];
    double val = -1.0;
    snprintf(path, sizeof(path), "/proc/self/task/%d/sched", tid);
    FILE *f = fopen(path, "r");
    if (!f) return val;
    while (fgets(line, sizeof(line), f))
        if (strncmp(line, field, strlen(field)) == 0) {
            sscanf(line, "%*s : %lf", &val);
            break;
        }
    fclose(f);
    return val;
}

/* transpose rows [row_start, row_end) of B into the corresponding columns of B_T
 * B_T[j][i] = B[i][j]; storing the transposed value row-by-row in B_T makes
 * the inner loop of multiply_segment access B_T sequentially (cache-friendly) */
static void transpose_segment(int row_start, int row_end)
{
    for (int i = row_start; i < row_end; i++)
        for (int j = 0; j < N; j++)
            B_T[j * N + i] = B[i * N + j];
}

/* cache-blocked matrix multiply for rows [row_start, row_end) of C
 * outer three loops tile the i-j-k iteration space into BLOCK x BLOCK blocks
 * so that the working set of each block fits in L1 cache, minimizing cache misses
 * inner loops accumulate C[ii][jj] += A[ii][kk] * B_T[jj][kk]
 * B_T[jj][kk] is sequential in memory because B was transposed, making both
 * A and B_T accesses sequential and maximizing cache line utilization */
static void multiply_segment(int row_start, int row_end)
{
    for (int i = row_start; i < row_end; i += BLOCK) {
        int i_end = (i + BLOCK < row_end) ? i + BLOCK : row_end;
        for (int j = 0; j < N; j += BLOCK) {
            int j_end = (j + BLOCK < N) ? j + BLOCK : N;
            for (int k = 0; k < N; k += BLOCK) {
                int k_end = (k + BLOCK < N) ? k + BLOCK : N;
                for (int ii = i; ii < i_end; ii++)
                    for (int jj = j; jj < j_end; jj++) {
                        float sum = C[ii * N + jj]; /* accumulate locally, avoids repeated memory writes */
                        for (int kk = k; kk < k_end; kk++)
                            sum += A[ii * N + kk] * B_T[jj * N + kk];
                        C[ii * N + jj] = sum;
                    }
            }
        }
    }
}

static void *thread_func(void *arg)
{
    thread_arg_t *targ = (thread_arg_t *)arg;
    pid_t tid = (pid_t)syscall(SYS_gettid);

    /* phase 1: each thread transposes its assigned rows of B into B_T */
    transpose_segment(targ->row_start, targ->row_end);

    /* barrier: all threads must finish transposing before any thread reads B_T */
    pthread_barrier_wait(&barrier);

    /* phase 2: compute assigned rows of C using the now-complete B_T */
    multiply_segment(targ->row_start, targ->row_end);

    /* thread has no remaining work; if cooperative mode, notify the scheduler
     * so it deprioritizes this thread in favor of any other active process
     * (e.g., the spinning benchmark running concurrently) */
    if (cooperative)
        syscall(SYS_SET_INACTIVE);

    /* record involuntary switches after work completes as a measure of
     * how much the scheduler preempted this thread during computation */
    targ->involuntary_sw = (long)read_sched_field(tid, "nr_involuntary_switches");
    return NULL;
}

int main(int argc, char *argv[])
{
    /* parse arguments: optional matrix size and --cooperative flag */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cooperative") == 0) cooperative = 1;
        else N = atoi(argv[i]);
    }

    num_threads = (int)sysconf(_SC_NPROCESSORS_ONLN);

    /* allocate matrices; C is zero-initialized since multiply_segment accumulates into it */
    A   = malloc((size_t)N * N * sizeof(float));
    B   = malloc((size_t)N * N * sizeof(float));
    B_T = malloc((size_t)N * N * sizeof(float));
    C   = calloc((size_t)N * N,  sizeof(float));

    if (!A || !B || !B_T || !C) {
        fprintf(stderr, "allocation failed: need ~%.1f GB\n",
                4.0 * N * N * sizeof(float) / 1e9);
        return 1;
    }

    /* fill A and B with uniform random values in [0, 1) */
    for (long i = 0; i < (long)N * N; i++) {
        A[i] = (float)rand() / RAND_MAX;
        B[i] = (float)rand() / RAND_MAX;
    }

    /* barrier requires exact num_threads participants */
    pthread_barrier_init(&barrier, NULL, num_threads);

    pthread_t    *threads = malloc(num_threads * sizeof(pthread_t));
    thread_arg_t *args    = calloc(num_threads, sizeof(thread_arg_t));

    /* divide rows as evenly as possible; last thread absorbs any remainder */
    int rows_per = N / num_threads;
    for (int i = 0; i < num_threads; i++) {
        args[i].id        = i;
        args[i].row_start = i * rows_per;
        args[i].row_end   = (i == num_threads - 1) ? N : args[i].row_start + rows_per;
    }

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start); /* begin timing before thread creation */

    for (int i = 0; i < num_threads; i++)
        pthread_create(&threads[i], NULL, thread_func, &args[i]);

    long total_sw = 0;
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL); /* block until thread i finishes */
        total_sw += args[i].involuntary_sw;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    double elapsed = (t_end.tv_sec  - t_start.tv_sec) +
                     (t_end.tv_nsec - t_start.tv_nsec) * 1e-9;

    /* 2*N^3 floating point operations: N^3 multiplications + N^3 additions */
    double gflops = (2.0 * N * N * N) / (elapsed * 1e9);

    printf("mode=%s N=%d threads=%d\n", cooperative ? "cooperative" : "default", N, num_threads);
    printf("time_seconds=%.3f gflops=%.2f involuntary_switches=%ld\n",
           elapsed, gflops, total_sw);

    pthread_barrier_destroy(&barrier);
    free(A); free(B); free(B_T); free(C);
    free(threads); free(args);
    return 0;
}
