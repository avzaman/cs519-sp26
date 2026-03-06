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

void semaphore_init(int sem_id, int sem_num, int init_val)
{
	//Use semctl to init semaphore
	if(semctl(sem_id, sem_num, SETVAL, init_val)<0){
		printf("ERROR semaphore INIT");
	}
	
}

void semaphore_release(int sem_id, int sem_num)
{
	//Use semop to release a semaphore
	struct sembuf rel = {0, +1, SEM_UNDO};
	if(semop(sem_id, &rel, sem_num)<0){
		printf("ERROR semaphore RELEASE");
	}
}

void semaphore_reserve(int sem_id, int sem_num)
{
	//Use semop to acquire a semaphore
	struct sembuf res = {0 , -1, SEM_UNDO};
	if(semop(sem_id, &res, sem_num)<0){
		printf("ERROR semaphore RESERVE");
	}
}

/* Time function that calculates time between start and end */
double getdeltatimeofday(struct timeval *begin, struct timeval *end)
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

int vector_mult(int m,int col,int u[m],int B[m][m]){
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
	struct timeval begin,end;

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
	int C[m][m]; // result matrix
	pid_t procID; // for deteecting if in parent or child prcoess
	int pipefd[2]; // init pipe array then make it
	pipe(pipefd);
	
	int semaphoreID = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
	semaphore_init(semaphoreID,0,1);
	
	int res = 0;
	
	gettimeofday(&begin, NULL);
	for(int i = 0; i < m; i++){
		for(int j = 0; j < m; j++){
			num_procs++;
			procID = fork();
			if(procID == 0){
				res = vector_mult(m,j,A[i],B);
				semaphore_reserve(semaphoreID,1); // critical section, writing to pipe
				write(pipefd[1], &i, sizeof(int));
				write(pipefd[1], &j, sizeof(int));
				write(pipefd[1], &res, sizeof(int));
				semaphore_release(semaphoreID,1);
				exit(0);
			}else if(procID < 0){
				printf("ERROR creating FORK");
			}
		}
	}
	
	// parent process reads pipe 12 bytes at a time format i, j, result
	int readI, readJ, readAns;
	for(int i = 0; i < m*m; i++){
		read(pipefd[0], &readI, sizeof(int));
		read(pipefd[0], &readJ, sizeof(int));
		read(pipefd[0], &readAns, sizeof(int));
		C[readI][readJ] = readAns;
	}
	
	gettimeofday(&end,NULL);
	elapsed = getdeltatimeofday(&begin,&end);

	if(DEBUG_SWITCH){printMatrix(m,C);}

	/* Your completed code must uncomment, and call the below function.*/ 
	print_stats(m, num_procs, verified, elapsed);
	return 0;
}
