#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
int main(void)
{
    printf("char: %d\n", sizeof(char));
    printf("short %d\n", sizeof(short));
    printf("short int %d\n", sizeof(short int));
    printf("int %d\n", sizeof(int));
    printf("longint %d\n", sizeof(long int));
    printf("double %d\n", sizeof(double));
    printf("float %d\n", sizeof(float));
    printf("u_long %d\n", sizeof(u_long));
    printf("double * %d\n", sizeof(double *));
    printf("time_val: %d\n", sizeof(struct timeval));
    return 0;
}
