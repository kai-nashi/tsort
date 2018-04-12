#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "tsort.h"

struct test_struct {
    int value0;
    int value1;
    int value2;
    int value3;
};

int comparator_char(const void*, const void*);
int comparator_int(const void*, const void*);
int comparator_struct(const void*, const void*);

void dialog(int*, int*, int*, int*);

void* generator_char(void*, int);
void* generator_int(void*, int);
void* generator_struct(void*, int);

void print_massive_char(void*, int, char*);
void print_massive_int(void*, int, char*);
void print_massive_struct(void*, int, char*);

float* test_body(int, int, int, void* (*generator)(void*, int), int (*comparator)(const void*, const void*), void (*printer)(void*, int, char*), int);
void test_suite(int, int, int, int, void* (*generator)(void*, int), int (*comparator)(const void*, const void*), void (*printer)(void*, int, char*), char*, int);

int main() {

    int     length = 1e6;
    int     processes = 4;
    int     tests = 10;
    int     verbose = 0;

    // CHAR
    dialog(&length, &processes, &tests, &verbose);
    test_suite(length, processes, tests, sizeof(char), generator_char, comparator_char, print_massive_char, "CHARS", verbose);
    printf("\n\nPress enter to continue test with integers\n");
    getchar();
    printf("Starting...\n");

    // INT
    dialog(&length, &processes, &tests, &verbose);
    test_suite(length, processes, tests, sizeof(int), generator_int, comparator_int, print_massive_int, "INT", verbose);
    printf("\n\nPress enter to continue test with structures\n");
    getchar();
    printf("Starting...\n");

    // STRUCT
    dialog(&length, &processes, &tests, &verbose);
    test_suite(length, processes, tests, sizeof(struct test_struct), generator_struct, comparator_struct, print_massive_struct, "STRUCT", verbose);
    printf("\n\nPress enter to exit...\n");
    getchar();

}

int comparator_char(const void *pointer_a, const void *pointer_b) {
    return (int)*(char*)pointer_a - (int)*(char*)pointer_b;
}

int comparator_int(const void *pointer_a, const void *pointer_b) {
    return *(int*)pointer_a - *(int*)pointer_b;
}

int comparator_struct(const void* pointer_a, const void* pointer_b) {
    return comparator_int((void*)&(((struct test_struct*)pointer_a)->value0), (void*)&(((struct test_struct *)pointer_b)->value0));
}

void dialog(int* length, int* processes, int* tests, int* verbose) {
    printf("Length of arrays: ");
    scanf("%d", length);

    printf("Split to processes: ");
    scanf("%d", processes);

    printf("Count of tests: ");
    scanf("%d", tests);

    printf("Verbose: ");
    scanf("%d", verbose);
    getchar(); // remove endline from last scanf
}


void* generator_char(void* massive, int length) {
    char* massive_typed = massive;
    for (int i=0; i < length; i++) {
        char value = 40 + rand() % 80;
        massive_typed[i] = value;
    }
    return massive_typed;
}

void* generator_int(void* massive, int length) {
    int* massive_typed = massive;
    for (int i=0; i < length; i++) {
        massive_typed[i] = rand() % 100000;
    }
    return massive_typed;
}

void* generator_struct(void* massive, int length) {
    struct test_struct* massive_typed = massive;
    for (int i=0; i < length; i++) {
        massive_typed[i].value0 = rand() % 100000;
    }
    return massive_typed;
}

void print_massive_char(void* massive, int massive_length, char* msg) {
    printf("%s (first 100):\n", msg);
    for (int i = 0; (i < massive_length && i < 100); i++) {
        printf("%5d", *(char*)(massive + i));
        if (i % 10 == 9) {
            printf("\n");
        }
    }
    printf("\n");
}

void print_massive_int(void* massive, int massive_length, char msg[]) {
    printf("%s (first 100):\n", msg);
    for (int i = 0; (i < massive_length && i < 100); i++) {
        printf("%6d", *(int*)(massive + i*4));
        if (i % 10 == 9) {
            printf("\n");
        }
    }
    printf("\n");
}

void print_massive_struct(void* massive, int massive_length, char msg[]) {
    printf("%s (first 100):\n", msg);
    for (int i = 0; (i < massive_length && i < 100); i++) {
        printf("%6d", ((struct test_struct*)(massive + i*sizeof(struct test_struct)))->value0);
        if (i % 10 == 9) {
            printf("\n");
        }
    }
    printf("\n");
}

float* test_body(int massive_length, int process, int data_size, void* (*generator)(void*, int), int (*comparator)(const void*, const void*), void (*printer)(void*, int, char*), int verbose) {

    // ========================================================================
    // BEFORE
    // ========================================================================

    void* massive = malloc(massive_length * data_size);
    if (massive == NULL) {
        printf("\n> Can't allocate memory. Need %d bytes. Exit.\n", massive_length * data_size);
        return NULL;
    }

    struct timeval start, stop;
    double secs;
    static float result[2];

    // ========================================================================
    // QSORT
    // ========================================================================

    if (verbose) printf("\n> Generating massive chars[%d]: ", massive_length);

    gettimeofday(&start, NULL);
    massive = generator(massive, massive_length);
    gettimeofday(&stop, NULL);

    secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
    if (verbose) printf("%.4f sec\n", secs);

    if (verbose) printer(massive, massive_length, "> Look at massive before sorting");

    if (verbose) printf("\n>Sorting massive by qsort[%d]: ", massive_length);

    gettimeofday(&start, NULL);
    qsort(massive, massive_length, data_size, comparator);
    gettimeofday(&stop, NULL);

    secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
    result[0] = secs;
    if (verbose) printf("%.4f sec\n", result[0]);

    if (verbose) printer(massive, massive_length, "> Look at massive after sorting");

    // ========================================================================
    // THREADED SORT
    // ========================================================================

    if (verbose) printf("\n> Generating massive chars[%d]: ", massive_length);

    gettimeofday(&start, NULL);
    massive = generator(massive, massive_length);
    gettimeofday(&stop, NULL);

    secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
    if (verbose) printf("%.4f sec\n", secs);

    if (verbose) printer(massive, massive_length, "> Look at massive before sorting");

    if (verbose) printf("\n> Sorting massive by tsort[%d]: ", massive_length);

    gettimeofday(&start, NULL);
    if (NULL == tsort(massive, massive_length, data_size, comparator, process)) {
        printf("tsort end with error, it's return NULL");
        return result;
    }
    gettimeofday(&stop, NULL);

    secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
    result[1] = secs;
    if (verbose) printf("%.4f sec\n", result[1]);

    if (verbose) printer(massive, massive_length, "> Look at massive after sorting");

    // ========================================================================
    // AFTER
    // ========================================================================

    if (verbose) printf("WARN: delete sorted massive from memory\n");
    free(massive);

    return result;
}

void test_suite(int length, int processes, int tests, int data_size, void* (*generator)(void*, int), int (*comparator)(const void*, const void*), void (*printer)(void*, int, char*), char* test_name, int verbose) {

    char    result_qsort_char[10], result_tsort_char[10];

    float*  result;
    float   result_qsort[tests], result_tsort[tests];
    float   result_qsort_mean = 0;
    float   result_tsort_mean = 0;

    if (verbose) {
        printf("\n\n=== START TEST BY SORTING %s ===\n", test_name);
        printf("length: %d; threads: %d; tests: %d;\n", length, processes, tests);
    }

    for (int i=0; i < tests; i++) {
        if (verbose) printf("\nStarting test #%d ...\n", i);
        result = test_body(length, processes, data_size, generator, comparator, printer, verbose);
        result_qsort[i] = *(result+0);
        result_tsort[i] = *(result+1);
    }

    printf("\n\n===RESULT OF TEST FOR %s===\n", test_name);
    printf("%14s |%14s\n", "qsort", "tsort");
    for (int i=0; i < tests; i++) {

        sprintf(result_qsort_char, "%.4f", result_qsort[i]);
        sprintf(result_tsort_char, "%.4f", result_tsort[i]);
        printf("%10s sec | %10s sec\n", result_qsort_char, result_tsort_char);

        result_qsort_mean += result_qsort[i];
        result_tsort_mean += result_tsort[i];
    }
    printf("MEAN:\n");
    printf("qsort: %.4f sec\n", result_qsort_mean / tests);
    printf("tsort: %.4f sec\n", result_tsort_mean / tests);

}
