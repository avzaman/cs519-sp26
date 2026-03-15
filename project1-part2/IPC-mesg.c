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
#define DEBUG_SWITCH 0
#define DEBUG_SWITCH2 0

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
	int m, num_procs, verified, procID;
	double elapsed;
	struct timeval begin,end;

	if(argc > 1){
		m = atoi(argv[1]);
	}else{
		m = MATRIX_SIZE;
	}

	// make two matrices to multiply together
	
	int memidA = shmget(IPC_PRIVATE, m*m*sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	int *A = (int *)shmat(memidA,0,0);

	int memidB = shmget(IPC_PRIVATE, m*m*sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	int *B = (int *)shmat(memidB,0,0);

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

	// create a message passing buffer for each child to the 1 parent process
	// pointers never change but content of their arrays are so no cache line bounce 
	
	int *memids = malloc(m*sizeof(int));
	int **C = malloc(m*sizeof(int *));
	for(int i = 0; i < m; i++){
		memids[i] = shmget(IPC_PRIVATE, m*sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	}

	gettimeofday(&begin, NULL); // start timer!!!
	for(int i = 0; i < m; i++){
		num_procs++;
		procID = fork();
		C[i] = (int *)shmat(memids[i],0,0); // only attach parent and ith proc
		if(procID == 0){	
			int *res = malloc(sizeof(int)*m);
			vector_mult(m,i,A,B,res);
			if(DEBUG_SWITCH){printf("ROW %d, Finished multing, writing to mesg buf...\n",i);}
				
			if(memcpy(C[i],res,m*sizeof(int))<0){
				printf("ROW %d MEMCPY ERROR\n", i);
				exit(1);
			};
			if(DEBUG_SWITCH){printf("ROW %d, wrote row vals to mesg buf! done!\n",i);}
		
			free(res);
			shmdt(A); // detatch child from matrices
			shmdt(B);
			shmdt(C[i]);
			exit(0);

		}else if(procID < 0){
			printf("ERROR creating FORK\n");
			return -1;
		}
	}


	//parent monitors to check if all rows have been filled
	while(wait(NULL) > 0){}

	int *D = malloc(m*m*sizeof(int));
	for(int i = 0; i < m; i++){
		memcpy(&D[i*m],C[i],m*sizeof(int));
	}

	gettimeofday(&end,NULL); // end timer!!!
	elapsed = getdeltatimeofday(&begin,&end); // total time is just multithreaded matmul

	if(DEBUG_SWITCH){printMatrix('D',m,D);}
	
	/* Your completed code must uncomment, and call the below function.*/ 
	
	gettimeofday(&begin,NULL);
	verified = verify(m,A,B,D);
	gettimeofday(&end,NULL);
	//verified = 1;
	
	free(D);
	shmdt(A);
	shmctl(memidA, IPC_RMID, 0); // release shared
	shmdt(B);
	shmctl(memidB, IPC_RMID, 0); // release shared
	for(int i = 0; i < m; i++){
		shmdt(C[i]);
		shmctl(memids[i], IPC_RMID, 0); // release shared
	}
	print_stats(m, num_procs, verified, elapsed);
	//printf("Non-parallel verification took: %f\n",getdeltatimeofday(&begin,&end));

	return 0;
}
