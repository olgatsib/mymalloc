#ifndef __MEMORY_H__
#define __MEMORY_H__

extern char* my_malloc_error;
void* my_malloc(int size);
void my_free(void* ptr);
void my_mallopt(int policy);
void my_mallinfo();

typedef enum { FIRST_FIT,  BEST_FIT } POLICY;

#endif
