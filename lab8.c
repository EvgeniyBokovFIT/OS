#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#define SUCCESS 0
#define BUF_SIZE 1024
#define CORRECT_ARGC_NUMBER 3
#define NOT_THREAD_ERROR 0

typedef enum _ErrorType {
    invalidArgcNumber,
    emptyArgument,
    notPositiveNumber,
    threadNumRange,
    iterNumRange,
    success,
    memoryAllocationError,
    threadCreatingError,
    threadJoiningError
} ErrorType;

typedef struct st_Error Error;
struct st_Error {
    ErrorType errorType;
    int errCode;
};

typedef struct st_Arguments Arguments;
struct st_Arguments {
    long numThreads;
    long numIterations;
};

typedef struct st_ArgsForThread ArgsForThread;
struct st_ArgsForThread {
    int iterNumber;
    int shift;
    double partialSum;
};

void printThreadError(int errnum) {
    char str[BUF_SIZE];
    strerror_r(errnum, str, BUF_SIZE);
    fprintf(stderr, "%s\n", str);
}

Error makeErrorStruct(ErrorType errorType, int errCode) {
    Error error;
    error.errorType = errorType;
    error.errCode = errCode;
    return error;
}

void printError(Error error) {
    if (error.errCode != NOT_THREAD_ERROR) {
        return printThreadError(error.errCode);
    }

    switch (error.errorType) {
    case invalidArgcNumber:
        fprintf(stderr, "Expected 2 arguments: number of threads and number of iterations\n");
        break;
    case emptyArgument:
        fprintf(stderr, "Empty argument\n");
        break;
    case notPositiveNumber:
        fprintf(stderr, "Only positive numbers are expected in the arguments\n");
        break;
    case threadNumRange:
        fprintf(stderr, "Number of threads is out of range\n");
        break;
    case iterNumRange:
        fprintf(stderr, "Number of iterations is out of range\n");
        break;
    case memoryAllocationError:
        fprintf(stderr, "Can not allocate enough memory. Try to enter not so big number of threads\n");
        break;
    }
}

Error getInputData(int argc, char** argv, Arguments* inputArgsValues) {

    if (argc != CORRECT_ARGC_NUMBER) {
        return makeErrorStruct(invalidArgcNumber, NOT_THREAD_ERROR);
    }
    if (argv[1][0] == '\0' || argv[0][0] == '\0') {
        return makeErrorStruct(emptyArgument, NOT_THREAD_ERROR);
    }
    int i = 0;
    while (argv[1][i] != '\0') {
        if (!isdigit(argv[1][i])) {
            return makeErrorStruct(notPositiveNumber, NOT_THREAD_ERROR);
        }
        i++;
    }
    i = 0;
    while (argv[2][i] != '\0') {
        if (!isdigit(argv[2][i])) {
            return makeErrorStruct(notPositiveNumber, NOT_THREAD_ERROR);
        }
        i++;
    }
    long strtolRes = strtol(argv[1], NULL, 10);
    inputArgsValues->numThreads = strtolRes;
    bool StrtolReturnError = (strtolRes == LONG_MAX || strtolRes == LONG_MIN);
    if (StrtolReturnError && errno == ERANGE) {
        return makeErrorStruct(threadNumRange, NOT_THREAD_ERROR);
    }
    if (strtolRes < 1 || strtolRes > INT_MAX) {
        return makeErrorStruct(threadNumRange, NOT_THREAD_ERROR);
    }

    strtolRes = strtol(argv[2], NULL, 10);
    inputArgsValues->numIterations = strtolRes;
    StrtolReturnError = (strtolRes == LONG_MAX || strtolRes == LONG_MIN);
    if (StrtolReturnError && errno == ERANGE) {
        return makeErrorStruct(iterNumRange, NOT_THREAD_ERROR);
    }
    if (strtolRes < 1 || strtolRes > INT_MAX) {
        return makeErrorStruct(iterNumRange, NOT_THREAD_ERROR);
    }

    return makeErrorStruct(success, NOT_THREAD_ERROR);
}


void countIterNumForEachThread(int numThreads, int numIterations, int* IterNumForThread) {
    int iterationsPerThread = numIterations / numThreads;
    for (int i = 0; i < numThreads; ++i) {
        IterNumForThread[i] = iterationsPerThread;
    }

    int remainderOfTheDiv = numIterations % numThreads;
    {
        for (int i = 0; i < remainderOfTheDiv; ++i) {
            IterNumForThread[i] += 1;
        }
    }
}

ArgsForThread* createThreadsFunctionArgs(int numThreads, int numIterations, const int* iterNums, ArgsForThread* args) {
    args[0].iterNumber = iterNums[0];
    args[0].shift = 0;
    args[0].partialSum = 0;
    for (int i = 1; i < numThreads; ++i) {
        args[i].iterNumber = iterNums[i];
        args[i].shift = args[i - 1].shift + iterNums[i - 1];
        args[i].partialSum = 0;
    }
    return args;
}

void* countPartialSum(void* arg) {
    ArgsForThread* threadArg = (ArgsForThread*)arg;
    for (int i = threadArg->shift; i < threadArg->shift + threadArg->iterNumber; ++i) {
        threadArg->partialSum += 1.0 / (i * 4.0 + 1.0);
        threadArg->partialSum -= 1.0 / (i * 4.0 + 3.0);
    }
    pthread_exit(&threadArg->partialSum);
}

Error createThreads(int numThreads, void* (*start_routine)(void*), ArgsForThread* args, pthread_t* threadID) {
    int errnum;
    for (int i = 0; i < numThreads; ++i) {
        errnum = pthread_create(&threadID[i], NULL, start_routine, &args[i]);
        if (errnum != SUCCESS) {
            return makeErrorStruct(threadCreatingError, errnum);
        }
    }
    return makeErrorStruct(success, NOT_THREAD_ERROR);
}

Error gatherPartialSums(pthread_t* threadID, int numThreads, ArgsForThread* args, double* result) {
    *result = 0;
    void* partialSum;
    int errnum;
    for (int i = 0; i < numThreads; ++i) {
        errnum = pthread_join(threadID[i], &partialSum);
        if (errnum != SUCCESS) {
            return makeErrorStruct(threadJoiningError, errnum);
        }
        *result += *(double*)partialSum;
    }
    *result *= 4.0;
    return makeErrorStruct(success, NOT_THREAD_ERROR);
}

Error countPi(int numThreads, int numIterations, double* result) {

    int* iterNumForThread = (int*)malloc(sizeof(int) * numThreads);
    if (iterNumForThread == NULL) {
        return makeErrorStruct(memoryAllocationError, NOT_THREAD_ERROR);
    }
    countIterNumForEachThread(numThreads, numIterations, iterNumForThread);


    ArgsForThread* threadArgs = (ArgsForThread*)malloc(sizeof(ArgsForThread) * numThreads);
    if (threadArgs == NULL) {
        free(iterNumForThread);
        return makeErrorStruct(memoryAllocationError, NOT_THREAD_ERROR);
    }
    threadArgs = createThreadsFunctionArgs(numThreads, numIterations, iterNumForThread, threadArgs);
    free(iterNumForThread);

    pthread_t* threadID = (pthread_t*)malloc(sizeof(pthread_t) * numThreads);
    if (threadID == NULL) {
        free(iterNumForThread);
        free(threadArgs);
        return makeErrorStruct(memoryAllocationError, NOT_THREAD_ERROR);
    }
    Error state = createThreads(numThreads, countPartialSum, threadArgs, threadID);
    if (state.errorType != success) {
        return state;
    }
    state = gatherPartialSums(threadID, numThreads, threadArgs, result);

    free(iterNumForThread);
    free(threadArgs);
    free(threadID);

    return makeErrorStruct(success, NOT_THREAD_ERROR);
}


int main(int argc, char** argv) {
    Arguments ArgsValues;
    Error error = getInputData(argc, argv, &ArgsValues);
    if (error.errorType != success) {
        printError(error);
        exit(EXIT_FAILURE);
    }

    double pi;
    error = countPi(ArgsValues.numThreads, ArgsValues.numIterations, &pi);
    if (error.errorType != success) {
        printError(error);
        exit(EXIT_FAILURE);
    }

    printf("pi = %.15g\n", pi);
    return EXIT_SUCCESS;
}
