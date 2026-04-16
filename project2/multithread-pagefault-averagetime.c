#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include<pthread.h>
#define NULL 0

#define SYS_use_extents 449
#define DEFAULT_THREADS 16
#define DEFAULT_NUM_PAGES 256
#define DEFAULT_NUM_ITERATIONS 100

typedef enum lock_types{
    MUTEX = 4
} lock_types;


// wrapper type and methods for different lock implementations
typedef struct lock_t {
    int lock_type;
    void *lock;
} lock_t;


pthread_mutex_t* mutex_create(){
    pthread_mutex_t* lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    return lock;
}

void mutex_init(pthread_mutex_t *lock)
{
  pthread_mutex_init(lock, NULL);
}

void mutex_destroy(pthread_mutex_t *lock)
{
  pthread_mutex_destroy(lock);
}

void mutex_unlock(pthread_mutex_t *lock)
{
  pthread_mutex_unlock(lock); 
}

void mutex_lock(pthread_mutex_t *lock)
{
    pthread_mutex_lock(lock);
}

lock_t* create_lock(lock_types lock_type){
    lock_t *lock = (lock_t*)malloc(sizeof(lock_t));

    switch (lock_type)
    {
    case MUTEX:
        lock->lock_type = MUTEX;
        lock->lock = mutex_create();
        break;
    }

    return lock;
}

void destroy_lock(lock_t *lock){
    switch(lock->lock_type){
        case MUTEX:
        mutex_destroy((pthread_mutex_t*)lock->lock);
        break;
    }

    free(lock);
    lock=NULL;
}


void init_lock(lock_t *lock){
    switch(lock->lock_type){
        case MUTEX:
        mutex_init((pthread_mutex_t*)lock->lock);
        break;
    }
}


void acquire_lock(lock_t *lock){
    switch(lock->lock_type){
        case MUTEX:
        mutex_lock((pthread_mutex_t*)lock->lock);
        break;
    }
}

void release_lock(lock_t *lock){
    switch(lock->lock_type){
        case MUTEX:
        mutex_unlock((pthread_mutex_t*)lock->lock);
        break;
    }
}


int num_threads;
int num_pages;
int iterations;
int buf_size;
int using_extents;
int reporting;
char **thread_buffers;
lock_t *lock;
static struct timeval start_time;
static struct timeval end_time;

//a single thread will allocate [num_pages] pages, fault all of them in, then free them
void *inc_thread(void *id) {

    int thread_id = (int)(long) id;
    int own_buffer = thread_id % num_threads;

    //step 1:
    //a bunch of virtual page numbers
    thread_buffers[own_buffer] = (char*) mmap(0, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (thread_buffers[own_buffer] == MAP_FAILED) {
        perror("mmap\n");
        return EXIT_FAILURE;
    }
    //step 2:
    //Initialize buffer with 67; this will guarantee each page is faulted in
    //i'm mature i swear
    memset(thread_buffers[own_buffer], 67, buf_size);

    //step 3:
    //deallocate/remove all mappings. this will force updates in the extent tree
    if(munmap(thread_buffers[own_buffer], buf_size)){
        perror("munmap\n");
        return EXIT_FAILURE;
    }

}

// A helper function to get current time in microseconds
static long get_time_us(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) {
        perror("gettimeofday");
        exit(EXIT_FAILURE);
    }
    return (tv.tv_sec * 1000000L) + tv.tv_usec;
}

void print_usage(char *cmd) {
    printf("Usage: %s [threadcount] [page count] [iterations] [any, use extents] [any, show extent table on thread create/exit] \n", cmd);
}

int main(int argc, char **argv) {
    pthread_t *thr;
    int ret = 0;

    if (argc > 5) {
        print_usage(argv[0]);
        exit(1);
    }

    int lock_type = 4; //always use mutex
    
    num_threads = (argc > 1) ? atoi(argv[1]) : DEFAULT_THREADS;
    num_pages = (argc > 2) ? atoi(argv[2]) : DEFAULT_NUM_PAGES;
    iterations = (argc > 3) ? atoi(argv[3]) : DEFAULT_NUM_ITERATIONS;
    using_extents = (argc > 4) ? 1 : 0;
    // reporting = (argc > 5) ? 1 : 0;

    if(using_extents)
        syscall(SYS_use_extents, 0);
    
    //create a bunch of buffers for threads to allocate their mappings to
    thread_buffers = malloc(sizeof(char*)*num_threads);

    //lock (just in case)
    //printf creates a physical page -- remember that.
    printf("Using %d threads each faulting in %d pages, %d iterations\n", num_threads, num_pages, iterations);
    thr = calloc(sizeof(*thr), num_threads);
    lock = create_lock(lock_type);
    init_lock(lock);

    buf_size = num_pages * 4096;

    // if(using_extents && reporting)
    //     syscall(SYS_use_extents, 1);

    unsigned long long time_sum = 0;

    // Start threads
    for(int i = 0; i < iterations; i++){

        long x0 = get_time_us();

        for (long i = 0; i < num_threads; i++) {
            if (pthread_create(&thr[i], NULL, inc_thread, (void *)i) != 0) {
                perror("Thread creation failed");
            }
        }

        // Join threads
        //on thread terminate, print out the state of the extent table
        for (int i = 0; i < num_threads; i++){
            pthread_join(thr[i], NULL);
        }

        long x = get_time_us();
        time_sum += (x - x0);
    }

    // //get the final state of the extent table
    // if(using_extents && reporting){
    //     syscall(SYS_use_extents, 1);
    // }
    printf("Total time to all-fault/all-free: %llu\n microseconds\n", time_sum);
    printf("Average time per thread group: %llu microseconds\n", time_sum/iterations);

    destroy_lock(lock);
    free(thr);

    return ret;
}
