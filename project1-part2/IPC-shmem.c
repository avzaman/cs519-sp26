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

#include <sys/wait.h>
#include <limits.h>
#include <sys/stat.h>

//Add all your global variables and definitions here.
#define MATRIX_SIZE 1000

#define RANDOM_MAXIMUM 10
#define DEBUG_SWITCH 1
#define DEBUG_SWITCH2 1

void semaphore_init(int sem_id, int sem_num, int init_val)
{
	//Use semctl to initialize a semaphore
	if(semctl(sem_id, sem_num, SETVAL, init_val)<0){
		printf("ERROR semaphore INIT\n");
	}
}

void semaphore_release(int sem_id, int sem_num)
{
	//Use semop to release a semaphore
	struct sembuf rel = {0, +1, SEM_UNDO};
	if(semop(sem_id, &rel, sem_num)<0){
		printf("ERROR semaphore RELEASE\n");
	}
}

void semaphore_reserve(int sem_id, int sem_num)
{

	//Use semop to acquire a semaphore
	struct sembuf res = {0 , -1, SEM_UNDO};
	if(semop(sem_id, &res, sem_num)<0){
		printf("ERROR semaphore RESERVE\n");
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

void printMatrix(char name, int m, int *M){
	printf("Printing Matrix %c:\n",name);
	for(int i = 0; i < m; i++){
		for(int j = 0; j < m; j++){
			printf("%4d ",M[i*m+j]);
		}
		printf("\n");
	}
}

/* Calculate the vector mults for a row in A across all columns in B*/
void vector_mult(int m,int row,int *A,int *B,int *res){	
	if(DEBUG_SWITCH){printf("Multing row %d \n", row);}

	for(int i = 0; i < m; i++){
		res[i] = 0;
		for(int j = 0; j < m; j++){
			res[i] += A[row*m+j]*B[j*m+i];
		}
	}
}

int verify(int m, int* A, int* B, int* C){
	int sum = 0;
	for(int i = 0; i < m; i++){
		for(int j = 0; j < m; j++){
			sum = 0;
			for(int k = 0; k < m; k++){
				sum += A[i*m+k]*B[k*m+j];
			}
			if(sum!=C[i*m+j]){return 0;}
		}
	}
	return 1;
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
	int *A = malloc(sizeof(int)*m*m); 
	int *B = malloc(sizeof(int)*m*m);
	for(int i = 0; i < m; i++){
		for(int j = 0; j < m; j++){
			A[i*m+j] = rand()%RANDOM_MAXIMUM;
			B[i*m+j] = rand()%RANDOM_MAXIMUM;
		}
	}

	if(DEBUG_SWITCH){
		printMatrix('A',m,A);
		printMatrix('B',m,B);
	}

	// initialize result matrix, create shaared memory matrix C, fork() for each row of A, 
	// child calculates, waits for semaphore, writes (int i, int[m] result) directly to output maatrix after aquiring lock
	pid_t procID; // for deteecting if in parent or child prcoess
	int semaphoreID = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
	semaphore_init(semaphoreID,0,1);
	
	int memid = shmget(IPC_PRIVATE, m*m*sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	int *C = (int *)shmat(memid,0,0);

	// since each fork already has the calcs in its memory,
	// should be able to just pass the pointer into C
	gettimeofday(&begin, NULL); // start timer!!!
	for(int i = 0; i < m; i++){
		num_procs++;
		procID = fork();
		if(procID == 0){	
			int *res = malloc(sizeof(int)*m);
			vector_mult(m,i,A,B,res);
			if(DEBUG_SWITCH){printf("ROW %d, Finished multing, aquiring lock...\n",i);}
			
			semaphore_reserve(semaphoreID,1); // critical section, writing to shared matrix
			if(DEBUG_SWITCH){printf("ROW %d, aquired lock! writing row num...\n",i);}
			
			if(memcpy(&C[i*m],res,m*sizeof(int))<0){
				printf("ROW %d MEMCPY ERROR\n", i);
				exit(1);
			};
			if(DEBUG_SWITCH){printf("ROW %d, wrote row vals! releasing lock...\n",i);}
			
			semaphore_release(semaphoreID,1); // end critical section release shared mat
			if(DEBUG_SWITCH){printf("ROW %d, lock released!!\n",i);}
			free(res);
			free(A);//free matrices copied by subprocess
			free(B);
			shmdt(C);
			exit(0);
		}else if(procID < 0){
			printf("ERROR creating FORK\n");
			return -1;
		}
	}


	//parent monitors to check if all rows have been filled
	while(wait(NULL) > 0)	

	gettimeofday(&end,NULL); // end timer!!!
	elapsed = getdeltatimeofday(&begin,&end); // total time is just multithreaded matmul

	if(DEBUG_SWITCH){printMatrix('C',m,C);}
	
	/* Your completed code must uncomment, and call the below function.*/ 
	
	gettimeofday(&begin,NULL);
	verified = verify(m,A,B,C);
	gettimeofday(&end,NULL);

	free(A);
	free(B);
	shmdt(C);
	shmctl(memid, IPC_RMID, 0); // release shared
	print_stats(m, num_procs, verified, elapsed);
	printf("Non-parallel verification took: %f\n",getdeltatimeofday(&begin,&end));

	return 0;
}
