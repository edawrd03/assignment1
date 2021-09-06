#include <algorithm>
#include <stdio.h>
#include <pthread.h>

#include "CycleTimer.h"

typedef struct {
    float x0, x1;
    float y0, y1;
    int width;  // avoid overflow hence change to singned type
    int height;
    int maxIterations;
    int* output;
    int threadId;
    int numThreads;
} WorkerArgs;


extern void mandelbrotSerial(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int numRows,
    int maxIterations,
    int output[]);

extern void mandelbrotSerialInterleave(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int threadId, int threadNum,
    int maxIterations,
    int output[]);

//
// workerThreadStart --
//
// Thread entrypoint.
void* workerThreadStart(void* threadArgs) {

    double startTime = CycleTimer::currentSeconds();
    WorkerArgs* args = static_cast<WorkerArgs*>(threadArgs);

    // block spatial decomposition

    // int numRows = (args->height + 1) / args->numThreads;
    // int startRow = args->threadId * numRows;
    // int totalRows = numRows + startRow > args->height ? args->height - startRow : numRows;

    // mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width, args->height,
    //                  startRow, totalRows, args->maxIterations, args->output);

    // interleave spatial decomposition

    mandelbrotSerialInterleave(args->x0, args->y0, args->x1, args->y1, args->width, args->height,
                     args->threadId, args->numThreads, args->maxIterations, args->output);

    double endTime = CycleTimer::currentSeconds();
    printf("[worker thread %d]:\t\t[%.3f] ms\n", args->threadId, std::min(1e30, endTime - startTime) * 1000);
    return NULL;
}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Multi-threading performed via pthreads.
void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations, int output[])
{
    const static int MAX_THREADS = 32;

    if (numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    pthread_t workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    for (int i=0; i<numThreads; i++) {
        // TODO: Set thread arguments here.
        args[i].x0 = x0;
        args[i].x1 = x1;
        args[i].y0 = y0;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].numThreads = numThreads;
        args[i].maxIterations = maxIterations;
        args[i].output = output;
        args[i].threadId = i;
    }

    // Fire up the worker threads.  Note that numThreads-1 pthreads
    // are created and the main app thread is used as a worker as
    // well.

    for (int i=1; i<numThreads; i++)
        pthread_create(&workers[i], NULL, workerThreadStart, &args[i]);

    workerThreadStart(&args[0]);

    // wait for worker threads to complete
    for (int i=1; i<numThreads; i++)
        pthread_join(workers[i], NULL);
}
