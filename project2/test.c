#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SYS_use_extents 449

int main(int argc, char* argv[]) {
    /* enable extent tracking for this process */
   
    long ret = syscall(SYS_use_extents,0);
    printf("syscall returned %ld\n", ret);
    /* allocate and touch memory to generate page faults */
    int num_pages = atoi(argv[1]);
    int num_ints = num_pages*4096/sizeof(int);
    int *arr = malloc(num_ints*sizeof(int));
    for (int i = 0; i < num_ints; i++)
        arr[i] = i;

    printf("done touching %d pages\n", num_pages);
    syscall(SYS_use_extents,1);
    free(arr);
    //syscall(SYS_use_extents,1);
    return 0;
}
