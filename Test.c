#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


void *computation()
{
    printf("Computation thread is running.\n");

    return NULL;

}

int main()
{
    pthread_t thread1;

    pthread_create(&thread1, NULL, computation, NULL);

    pthread_join(thread1, NULL);

    return 0;
}