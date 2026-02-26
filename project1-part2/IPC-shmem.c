/* CS 519, Spring 2025: Project 1 - Part 2
 * IPC using shared memory to perform matrix multiplication.
 * Feel free to extend or change any code or functions below.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>

//Add all your global variables and definitions here.
#define MATRIX_SIZE 1000


void semaphore_init(int sem_id, int sem_num, int init_valve)
{

   //Use semctl to initialize a semaphore
}

void semaphore_release(int sem_id, int sem_num)
{
  //Use semop to release a semaphore
}

void semaphore_reserve(int sem_id, int sem_num)
{

  //Use semop to acquire a semaphore
}

/* Time function that calculates time between start and end */
double getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}


/* Do not add printf() calls anywhere else in your final submission.
 *
 * Required output (match field names and spacing exactly):
 *
 *   Input size: <N> x <N>
 *   Number of processes: <P>
 *   Verification: <PASS|FAIL>
 *   Total runtime: <X.XXXXXX> seconds
 *
 * Suggested signature — adapt parameters to match your implementation:
 */
void print_stats(int matrix_size, int num_processes, int verified, double elapsed)
{
    printf("Input size: %d x %d\n",        matrix_size, matrix_size);
    printf("Number of processes: %d\n",    num_processes);
    printf("Verification: %s\n",           verified ? "PASS" : "FAIL");
    printf("Total runtime: %.6f seconds\n", elapsed);
}


int main(int argc, char const *argv[])
{


   /* Your completed code must uncomment, and call the below function.*/ 
   //print_stats(MATRIX_SIZE, num_procs, verified, elapsed);
   return 0;
}


