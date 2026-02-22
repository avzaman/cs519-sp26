#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

#define MY_SYS_CALL_NUM 449

int main(void){
	//int *buffer = malloc(4*sizeof(int));
	syscall(449,1,4);
	//free(buffer);
	return 0;
}
