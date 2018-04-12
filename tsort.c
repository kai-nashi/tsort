#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "tsort.h"

// ============================================================================
// = STUFF
// ============================================================================

int bisect(void* massive[], int massive_length, const void* value, int (*comparator)(const void*, const void*)) {

    int first = 0;
    int last = massive_length - 1;
    int middle;

    while (first <= last) {
        middle = floor((first + last)/2.0);

        int compair_result = comparator(&massive[middle], &value);
        if (compair_result == 0)
            return middle;
        else if (compair_result < 0)
            first = middle + 1;
        else
            last = middle - 1;
    }

    if (first > last || comparator(&massive[first], &value) > 0)
        return first;
    else
        return first + 1;

}

int compair_parts(const void* pointer_a, const void* pointer_b) {

    if (pointer_a == NULL) {
        return 1;
    }

    if (pointer_b == NULL) {
        return -1;
    }

    int (*comparator)(const void*, const void*);
    comparator = (*(struct pthread_args*)*(int**)pointer_a).comparator;
    void* a = (void*)(((struct pthread_args*)*(int**)pointer_a)->dataPointer);
    int ptr = *(int**)pointer_b;
    void* b = (void*)(((struct pthread_args*)*(int**)pointer_b)->dataPointer);
    return comparator(a, b);
}

void* merge(struct pthread_args* args, int (*comparator)(const void*, const void*), int threads_max) {
    // pointer in args should be in right order
    // args[0] = part_0
    // args[1] = part_1
    // ...
    // args[n] = part_n
    // return pointer to merged massive

    // create massive of pointers to first element of each parts
    void* particles[threads_max];
    int particles_remain = threads_max;

    int resultLength = 0;
    for (int i=0; i<particles_remain; i++) {
        if (args[i].dataLength > 0) {
            resultLength += args[i].dataLength;
            particles[i] = &args[i];
        } else {
            particles_remain -= 1;
        }
    }

    // sort it and remove all NULL pointers by compair parts
    int dataSize = args[0].dataSize;
    qsort(particles, particles_remain, sizeof(void*), compair_parts);

    // merge particles
    void* result = malloc(resultLength * dataSize);
    if (result == NULL) {
        printf("\n> tsort: Can't allocate memory. Need %d bytes. Exit.\n", resultLength*dataSize);
        return NULL;
    }

    void* result_pointer = args[0].dataPointer;
    void* particle_tmp;
    int result_index = 0;
    while (particles_remain > 0) {
        //memcpy(result[result_index], *particles[0]->dataPointer, dataSize);
        /*
        printf("\n\n%d + %d = %d; ", result, result_index*dataSize, result+result_index*dataSize);
        printf("next %d\n", (int)*(char*)((struct pthread_args *)particles[0])->dataPointer);
        */
        memcpy(result+result_index*dataSize, ((struct pthread_args *)particles[0])->dataPointer, dataSize);
        result_index++;
        /*
        for (int i=0; i < result_index; i++) {
            printf("%d, ",(int)*(char*)(result+i*dataSize));
        }
        printf("\n");

        for (int i = 0; i < particles_remain; i++) {
            printf("[%d]:", i);
            for (int j = 0; j < ((struct pthread_args *)particles[i])->dataLength; j++) {
                printf("%4d, ", (int)*(char*)(((struct pthread_args *)particles[i])->dataPointer+j*dataSize));
            }
            printf("\n");
        }
        */
        ((struct pthread_args *)particles[0])->dataLength -= 1;
        if (((struct pthread_args *)particles[0])->dataLength <= 0) {
            particles[0] = NULL;
            particles_remain -= 1;

            if (particles_remain == 1) {
                memcpy(result+result_index*dataSize, ((struct pthread_args *)particles[1])->dataPointer, dataSize*(((struct pthread_args *)particles[1])->dataLength));
                break;
            }

            //printf("%d + 1 = %d\n", particles, particles+1);
            memmove(particles, particles+1, sizeof(void*) * particles_remain);
            /*
            for (int i = 0; i < particles_remain; i++) {
                printf("%d, ", (int)*(char*)((struct pthread_args *)particles[i])->dataPointer);
            }
            printf("\n");
            */
        }
        else {
            ((struct pthread_args *)particles[0])->dataPointer += dataSize;
            if (compair_parts(&particles[0], &particles[1]) > 0) {
                // we should move first element, not inset copy
                int index = bisect(particles+1, particles_remain-1, particles[0], compair_parts);
                particle_tmp = particles[0];
                memmove(particles, particles+1, sizeof(void*) * index);
                particles[index] = particle_tmp;
            }
        }
    }

    memcpy(result_pointer, result, resultLength * dataSize);
    /*
    printf("\n\nResult:\n");
    for (int i=0; i < resultLength; i++) {
        printf("%5d", (int)*(char*)(result_pointer+i*dataSize));
        if (i % 10 == 9) {
            printf("\n");
        }
    }
    printf("\n");
    */
    free(result);
    return result_pointer;
}

void* qsort_thread(void* args_pointer) {
    struct pthread_args args = *(struct pthread_args*)args_pointer;
    qsort(args.dataPointer, args.dataLength, args.dataSize, args.comparator);
}

void* tsort(void* dataPointer, int dataLength, int dataSize, int (*comparator)(const void*, const void*), int threads_max) {

    if (dataLength <= 1 || threads_max < 1)
        return;

    int shift = ceil((float)dataLength / threads_max);
    struct pthread_args args[threads_max];
    pthread_t threads[threads_max];

    int i = 0;
    for (i=0; i<threads_max; i++) {

        args[i].comparator = comparator;
        args[i].dataPointer = dataPointer + shift*i*dataSize;
        args[i].dataSize = dataSize;

        // i+1 -> sort next <shift> elements
        if (shift*(i+1) > dataLength) {
            if (dataLength - shift*(i) > 0) {
                args[i].dataLength = dataLength - shift*(i);
            } else {
                // no elements for qsort_thread;
                args[i].dataPointer = dataPointer + dataLength*dataSize;
                args[i].dataLength = 0;
            }
        } else {
            args[i].dataLength = shift;
        }

        int status = pthread_create(&threads[i], NULL, qsort_thread, &args[i]);
        if (status != 0) {
            printf("> tsort: can't create thread: %s\n", strerror(status));
        }
    }

    for (i = 0; i < threads_max; i++) {
        int status = pthread_join(threads[i], NULL);
        if (status != 0) {
            printf("> tsort: can't join thread: %s\n", strerror(status));
            return NULL;
        }
    }

    return merge(args, comparator, threads_max);
}
