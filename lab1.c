#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#define THREAD_CREATED_SUCCESSFULLY 0
#define BUF_SIZE 256

void *print(void *arg) {
    for(int i = 0; i < 10; i++) {
        printf("%s %d\n", (char*)arg, i + 1);
    }
    free(arg);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t threadID;
    int errnum;
    char* child = (char*)malloc(5 * sizeof(char));
    child = "CHILD";
    errnum = pthread_create(&threadID, NULL, print, (void*)child);
    if (errnum != THREAD_CREATED_SUCCESSFULLY) {
        char str[BUF_SIZE];
        strerror_r(errnum, str, BUF_SIZE);
        fprintf(stderr, "%s", str);
        exit(EXIT_FAILURE);
    }

    print("PARENT");
    pthread_exit(NULL);
}
