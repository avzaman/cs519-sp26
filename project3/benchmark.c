#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>
#include <stdatomic.h>

#define SYS_SET_INACTIVE  449
#define VECTOR_SIZE       256

static int        cooperative = 0;
static int        num_threads;
static atomic_int stop_flag   = 0;

typedef struct {
    int    idx;
    pid_t  tid;
    long   iterations;
    double vruntime_inactive;
    double sum_exec_ns;
    long   involuntary_switches;
} thread_arg_t;

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

static void vector_add(volatile float *a, volatile float *b, volatile float *c, int n)
{
    for (int i = 0; i < n; i++)
        c[i] = a[i] + b[i];
}

/* sets stop_flag on SIGTERM or SIGINT; threads check this each iteration */
static void sigterm_handler(int sig)
{
    (void)sig;
    atomic_store(&stop_flag, 1);
}

static void *spin_thread(void *arg)
{
    thread_arg_t *targ = (thread_arg_t *)arg;
    volatile float a[VECTOR_SIZE], b[VECTOR_SIZE], c[VECTOR_SIZE];

    for (int i = 0; i < VECTOR_SIZE; i++) {
        a[i] = (float)i;
        b[i] = (float)(VECTOR_SIZE - i);
    }

    targ->tid = (pid_t)syscall(SYS_gettid);

    if (cooperative)
        /* mark inactive: put_prev_entity will add INACTIVE_PENALTY_NS to vruntime
         * on every preemption, pushing this thread to the back of the CFS tree */
        syscall(SYS_SET_INACTIVE);

    long count = 0;
    while (!atomic_load(&stop_flag)) {
        vector_add(a, b, c, VECTOR_SIZE);
        count++;
    }

    /* read vruntime before reactivation so we see the inflated value,
     * not the min_vruntime it gets reset to on reactivation */
    targ->vruntime_inactive = read_sched_field(targ->tid, "vruntime");

    if (cooperative)
        /* reactivate: syscall resets vruntime to min_vruntime, clears inactive flag */
        syscall(SYS_SET_INACTIVE);

    targ->sum_exec_ns         = read_sched_field(targ->tid, "sum_exec_runtime");
    targ->involuntary_switches = (long)read_sched_field(targ->tid, "nr_involuntary_switches");
    targ->iterations           = count;
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "--cooperative") == 0)
        cooperative = 1;

    num_threads = (int)sysconf(_SC_NPROCESSORS_ONLN);

    /* register signal handler so SIGTERM from the script triggers a clean exit */
    struct sigaction sa = { .sa_handler = sigterm_handler, .sa_flags = 0 };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    pthread_t    *threads = malloc(num_threads * sizeof(pthread_t));
    thread_arg_t *args    = calloc(num_threads, sizeof(thread_arg_t));

    printf("mode=%s threads=%d\n", cooperative ? "cooperative" : "default", num_threads);

    for (int i = 0; i < num_threads; i++) {
        args[i].idx = i;
        pthread_create(&threads[i], NULL, spin_thread, &args[i]);
    }

    /* run indefinitely until SIGTERM or SIGINT; sleep(1) loop avoids busy-waiting in main */
    while (!atomic_load(&stop_flag))
        sleep(1);

    long   total_iters   = 0;
    double total_exec_ms = 0.0;

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        printf("thread=%d tid=%d iterations=%ld vruntime_ms=%.2f sum_exec_ms=%.2f involuntary_switches=%ld\n",
               args[i].idx, args[i].tid, args[i].iterations,
               args[i].vruntime_inactive,
               args[i].sum_exec_ns / 1e6,
               args[i].involuntary_switches);
        total_iters   += args[i].iterations;
        total_exec_ms += args[i].sum_exec_ns / 1e6;
    }

    printf("total_iterations=%ld total_cpu_ms=%.2f\n", total_iters, total_exec_ms);

    free(threads);
    free(args);
    return 0;
}
