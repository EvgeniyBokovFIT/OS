#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 1024
#define MUTEX_COUNT 3
#define ITERATIONS_COUNT 10
#define SUCCESS 0
#define ONE_SECOND 1
#define FIRST_MUTEX 1
#define SECOND_MUTEX 2
#define FIRST_THREAD 1
#define SECOND_THREAD 2

typedef struct st_ThreadArguments ThreadArguments;
struct st_ThreadArguments {
    int threadNum;
    pthread_mutex_t *mutexes;
};

void printThreadError(int errnum) {
    char str[BUF_SIZE];
    strerror_r(errnum, str, BUF_SIZE);
    fprintf(stderr, "%s\n", str);
}


int destroyMutexes(int count, pthread_mutex_t* mutexes) {
    int errnum;
    for (int i = 0; i < count; ++i) {
        errnum = pthread_mutex_destroy(&mutexes[i]);
        if (errnum != SUCCESS) {
            return errnum;
        }
    }
    return SUCCESS;
}

int initMutexes(pthread_mutex_t* mutexes) {
    pthread_mutexattr_t mattr;
    int errnum;
    errnum = pthread_mutexattr_init(&mattr);
    if (errnum != SUCCESS) {
        return errnum;
    }

    errnum = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
    if (errnum != SUCCESS) {
        return errnum;
    }

    for (int i = 0; i < MUTEX_COUNT; ++i) {
        errnum = pthread_mutex_init(&mutexes[i], &mattr);
        if (errnum != SUCCESS) {
            destroyMutexes(i, mutexes);
            return errnum;
        }
    }
    return SUCCESS;
}

void* print(void* args) {
    ThreadArguments threadArgs = *(ThreadArguments*)args;
    int threadNum = threadArgs.threadNum;
    pthread_mutex_t* mutexes = threadArgs.mutexes;
    int curMutex = FIRST_MUTEX;
    int errnum;
    if (threadNum == SECOND_THREAD) {
        errnum = pthread_mutex_lock(&mutexes[SECOND_MUTEX]);
        if (errnum != SUCCESS) {
            printThreadError(errnum);
            return NULL;
        }
        curMutex = SECOND_MUTEX;
    }
    for (int i = 0; i < ITERATIONS_COUNT; ++i) {
        errnum = pthread_mutex_lock(&mutexes[(curMutex + 2) % MUTEX_COUNT]);
        if (errnum != SUCCESS) {
            printThreadError(errnum);
            return NULL;
        }
        printf("Thread %d: %d\n", threadNum, i);
        errnum = pthread_mutex_unlock(&mutexes[curMutex]);
        if (errnum != SUCCESS) {
            printThreadError(errnum);
            return NULL;
        }
        errnum = pthread_mutex_lock(&mutexes[(curMutex + 1) % MUTEX_COUNT]);
        if (errnum != SUCCESS) {
            printThreadError(errnum);
            return NULL;
        }
        errnum = pthread_mutex_unlock(&mutexes[(curMutex + 2) % MUTEX_COUNT]);
        if (errnum != SUCCESS) {
            printThreadError(errnum);
            return NULL;
        }
        errnum = pthread_mutex_lock(&mutexes[curMutex]);
        if (errnum != SUCCESS) {
            printThreadError(errnum);
            return NULL;
        }
        errnum = pthread_mutex_unlock(&mutexes[(curMutex + 1) % MUTEX_COUNT]);
        if (errnum != SUCCESS) {
            printThreadError(errnum);
            return NULL;
        }
    }
    errnum = pthread_mutex_unlock(&mutexes[curMutex]);
    if (errnum != SUCCESS) {
        printThreadError(errnum);
        return NULL;
    }
}

int main(int argc, char** argv) {
    ThreadArguments firstThreadArgs;
    ThreadArguments secondThreadArgs;
    pthread_t thread;
    pthread_mutex_t mutexes[MUTEX_COUNT];

    int errnum;
    errnum = initMutexes(mutexes);
    if (errnum != SUCCESS) {
        exit(EXIT_FAILURE);
    }
    errnum = pthread_mutex_lock(&mutexes[FIRST_MUTEX]);
    if (errnum != SUCCESS) {
        printThreadError(errnum);
        destroyMutexes(MUTEX_COUNT, mutexes);
        exit(EXIT_FAILURE);
    }

    firstThreadArgs.threadNum = FIRST_THREAD;
    firstThreadArgs.mutexes = mutexes;
    secondThreadArgs.threadNum = SECOND_THREAD;
    secondThreadArgs.mutexes = mutexes;

    errnum = pthread_create(&thread, NULL, print, (void*)&secondThreadArgs);
    if (errnum != SUCCESS) {
        printThreadError(errnum);
        destroyMutexes(MUTEX_COUNT, mutexes);
        exit(EXIT_FAILURE);
    }

    sleep(ONE_SECOND);

    print((void*)&firstThreadArgs);

    errnum = pthread_join(thread, NULL);
    if (errnum != SUCCESS) {
        printThreadError(errnum);
        destroyMutexes(MUTEX_COUNT, mutexes);
        exit(EXIT_FAILURE);
    }

    errnum = destroyMutexes(MUTEX_COUNT, mutexes);
    if (errnum != SUCCESS) {
        printThreadError(errnum);
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
