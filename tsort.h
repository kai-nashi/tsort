#ifndef TSORT_H_INCLUDED
#define TSORT_H_INCLUDED

int bisect(void*, int, const void*, int (*comparator)(const void*, const void*));
int compair_parts(const void*, const void*);
void* merge(struct pthread_args*, int (*comparator)(const void*, const void*), int);
void* tsort(void*, int, int, int (*comparator)(const void*, const void*), int);
void* qsort_thread(void*);

#endif // TSORT_H_INCLUDED
