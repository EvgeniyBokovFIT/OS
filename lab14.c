#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#define BUF_SIZE 1024
#define ITERATIONS_COUNT 10
#define SUCCESS 0
#define SEMAPHORES_COUNT 2
#define FIRST_SEMAPHORE 0
#define SECOND_SEMAPHORE 1
#define FIRST_THREAD 1
#define SECOND_THREAD 2
#define NOT_SHARED 0

typedef struct st_ThreadArguments ThreadArguments;
struct st_ThreadArguments {
    int threadNum;
    sem_t* semaphores;
};

void printThreadError(int errnum) {
    char str[BUF_SIZE];
    strerror_r(errnum, str, BUF_SIZE);
    fprintf(stderr, "%s\n", str);
}


int destroySemaphores(int semaphoresCount, sem_t* semaphores) {
    int errnum;
    for (int i = 0; i < semaphoresCount; ++i) {
        errnum = sem_destroy(&semaphores[i]);
        if (errnum != SUCCESS) {
            return errnum;
        }
    }
    return SUCCESS;
}

int initSemaphores(sem_t* semaphores) {
    int errnum;
    errnum = sem_init(&semaphores[FIRST_SEMAPHORE], NOT_SHARED, 1);
    if (errnum != SUCCESS) {
        return errnum;
    }
    errnum = sem_init(&semaphores[SECOND_SEMAPHORE], NOT_SHARED, 0);
    if (errnum != SUCCESS) {
        destroySemaphores(1, semaphores);
        return errnum;
    }
    return SUCCESS;
}

void* print(void* args) {
    ThreadArguments threadArgs = *(ThreadArguments*)args;
    int threadNum = threadArgs.threadNum;
    sem_t* semaphores = threadArgs.semaphores;
    int curSem = FIRST_SEMAPHORE;
    int anotherSem = SECOND_SEMAPHORE;
    if (threadArgs.threadNum == SECOND_THREAD) {
        curSem = SECOND_SEMAPHORE;
        anotherSem = FIRST_SEMAPHORE;
    }
    int errnum;

    for (int i = 0; i < ITERATIONS_COUNT; ++i) {
        errnum = sem_wait(&semaphores[curSem]);
        if (errnum != SUCCESS) {
            printThreadError(errnum);
            return NULL;
        }
        printf("Thread %d: %d\n", threadNum, i);
        errnum = sem_post(&semaphores[anotherSem]);
        if (errnum != SUCCESS) {
            printThreadError(errnum);
            return NULL;
        }
    }
}

int main(int argc, char** argv) {
    ThreadArguments firstThreadArgs;
    ThreadArguments secondThreadArgs;
    pthread_t thread;
    sem_t semaphores[SEMAPHORES_COUNT];

    int errnum;
    errnum = initSemaphores(semaphores);
    if (errnum != SUCCESS) {
        exit(EXIT_FAILURE);
    }

    firstThreadArgs.threadNum = FIRST_THREAD;
    firstThreadArgs.semaphores = semaphores;
    secondThreadArgs.threadNum = SECOND_THREAD;
    secondThreadArgs.semaphores = semaphores;

    errnum = pthread_create(&thread, NULL, print, (void*)&secondThreadArgs);
    if (errnum != SUCCESS) {
        printThreadError(errnum);
        destroySemaphores(SEMAPHORES_COUNT, semaphores);
        exit(EXIT_FAILURE);
    }

    print((void*)&firstThreadArgs);

    errnum = pthread_join(thread, NULL);
    if (errnum != SUCCESS) {
        printThreadError(errnum);
        destroySemaphores(SEMAPHORES_COUNT, semaphores);
        exit(EXIT_FAILURE);
    }

    errnum = destroySemaphores(SEMAPHORES_COUNT, semaphores);
    if (errnum != SUCCESS) {
        printThreadError(errnum);
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
