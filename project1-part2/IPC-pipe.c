/* CS 519, Spring 2025: Project 1 - Part 2
 * IPC using pipes to perform matrix multiplication.
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
#define RANDOM_MAXIMUM 10
#define DEBUG_SWITCH 1

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

void printMatrix(int m, int M[m][m]){
	printf("Printing Matrix:\n");
	for(int i = 0; i < m; i++){
		for(int j = 0; j < m; j++){
			printf("%4d ",M[i][j]);
		}
		printf("\n");
	}
}

int vector_mult(m,col,u[m],B[m][m]){
	int sum = 0;
	for(int i = 0; i < m; i++){
		sum += u[i]*B[i][col];
	}
	return sum;
}

int main(int argc, char const *argv[])
{
	int m, num_procs, verified;
	double elapsed;

	if(argc > 1){
		m = atoi(argv[1]);
	}else{
		m = MATRIX_SIZE;
	}

	// make two matrices to multiply together
	int A[m][m], B[m][m];
	for(int i = 0; i < m; i++){
		for(int j = 0; j < m; j++){
			A[i][j] = rand()%RANDOM_MAXIMUM;
			B[i][j] = rand()%RANDOM_MAXIMUM;
		}
	}

	if(DEBUG_SWITCH){
		printMatrix(m,A);
		printMatrix(m,B);
	}

	// initialize result matrix, create pipe, fork() for each row of A column of B, 
	// child calculates, waits for semaphore, writes (int i, int j, int result) to pipe up aquiring lock
	int C[m][m];
	pid_t procID;
	int pipe[2] = pipe();
	int semaphoreID = 1;
	init_semaphore(semaphoreID);
	int res = 0;
	for(int i = 0; i < m; i++){
		for(int j = 0; j < m; j++){
			procID = fork();
			if(procID == 0){
				res = vector_mult(m,j,A[i],B);
				semaphore_reserve();
				write(pipe[1], i, sizeof(int));
				write(pipe[1], j, sizeof(int));
				write(pipe[1], res, sizeof(int));
				semaphore_release();
				exit(0);
			}
		}
	}

	// parent process reads pipe 12 bytes at a time format i, j, result

	/* Your completed code must uncomment, and call the below function.*/ 
	print_stats(m, num_procs, verified, elapsed);
	return 0;
}


