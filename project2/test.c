#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SYS_use_extents 449

int main() {
    /* enable extent tracking for this process */
    syscall(SYS_use_extents);

    /* allocate and touch memory to generate page faults */
    int n = 1000;
    int *arr = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        arr[i] = i;

    printf("done touching %d pages\n", n);
    free(arr);
    return 0;
}
