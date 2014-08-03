/* mymalloc.c
 *
 * Description: a memory allocation library, equivalent to the malloc library
 *
 * Author: Olga Tsibulevskaya, ID 260540871
 * Course: COMP 310
 * Date: 29 March 2013
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "mymalloc.h"

typedef struct list_free {
  int size;
  struct list_free* previous;
  struct list_free* next;
} *listFree;

listFree listHeader = NULL;
char* startMalloc = NULL;
char* endMalloc = NULL;

char* my_malloc_error; // error message

static int SIZE_ALLOC = 56; 
POLICY p = 0; // first-fit policy par default
int allocated = 0;

int* startFree;
listFree last;

void* my_malloc(int size) {
  void* pointer;
  if (listHeader == NULL) {// first allocation;
    int toAllocate = size + SIZE_ALLOC*1024 + sizeof(int)*5; // size + SIZE_ALLOC in kBytes + 1 int for length and 4 for tags
    startMalloc = (char*)sbrk(toAllocate); // get the start of the heap
    pointer = startMalloc + sizeof(int)*2; // pointer to return, one flag and size below the pointer 
    startFree = pointer + size + sizeof(int)*2; // a free block starts (+ 2 tags for allocated and free blocks)
    endMalloc = startMalloc + toAllocate;
    endMalloc[-1] = 0; 
    
    int* sizeBlock = (int*)pointer;
    sizeBlock[-1] = size;   // size of the block to return
    sizeBlock[-2] = 1; // set a flag to allocated at the bottom of the block
    char* tag = pointer + size;
    *tag = 1; // a flag at to the top of the block
   
    // initialize the list of free blocks
    listFree l;
    l = (listFree)startFree; // free block starts
    startFree[-1] = 0; // a flag at the bottom
    l->size = toAllocate-size-sizeof(int)*5; // size of the free block (2 tags for allocated, 2 tags for free, 1 size for allocated)
    l->previous = NULL;
    l->next = NULL;
    listHeader = l;
  }  
  else { // if not the first time, check the list of free space
    listFree ptr;
    if (p == 0) { // first-fit
      for (ptr = listHeader; ptr != NULL && ptr->size < size; ptr = ptr->next) {
	if (ptr->next == NULL) {
	  last = ptr;
	}
      }
    }
    else { // best-fit
      int min = INT_MAX;
      listFree minList;
      for (ptr = listHeader; ptr != NULL; ptr = ptr->next) {
	if (ptr->size >= size && ptr->size < min) {
	  min = ptr->size;
	  minList = ptr;
	}
      }
      ptr = minList;
    }
    if (ptr != NULL) {  // there exists some free space, can allocate
      pointer = (char*)ptr + sizeof(int);
      char* tag = pointer + size;
      *tag = 1; // flag at the top
      
      startFree = pointer + size + 8;
      startFree[-1] = 0; // flag at the bottom

      listFree l;
      l = (listFree)startFree; // free block starts
      l->size = ptr->size - size;
      
      if (l->size != 0) { // add to the list
	l->previous = ptr->previous;
	l->next = ptr->next;
	if (l->next)
	  l->next->previous = l;
	if (ptr == listHeader)
	  listHeader = l;
	else 
	  ptr->previous->next = l;
      }
      else { // remove the pointer to the space of 0 bytes
	if (ptr->next) {
	  ptr->previous->next = ptr->next;
	  ptr->next->previous = ptr->previous;
	}
	else {
	  if (ptr->previous)
	    ptr->previous->next = NULL;
	  else
	    listHeader = NULL;
	}
      }
      int* sizeBlock = (int*)pointer; 
      sizeBlock[-1] = size;
      sizeBlock[-2] = 1; // block allocated, tag at the bottom
            
    }
    else { // not enough size available, allocate more from the heap
      int toAllocate = size + SIZE_ALLOC*1024 + sizeof(int)*5; // size + SIZE_ALLOC in kBytes + 2 for length (free and allocated), 4 tags
      startMalloc = (char*)sbrk(toAllocate);
      pointer = startMalloc + sizeof(int)*2; // pointer to return
      endMalloc = startMalloc + toAllocate;
      endMalloc[-1] = 0;

      int* sizeBlock = (int*)pointer;
      sizeBlock[-1] = size;   // size of the block to return
      sizeBlock[-2] = 1; // flag at the bottom
      char* tag = pointer + size;
      *tag = 1; // flag at the top
      
      startFree = pointer + size + 8; // + 2 tags
      
      // add free space to the list
      listFree l;
      l = (listFree)startFree; // free block starts
      startFree[-1] = 0;
      l->size = toAllocate-size-sizeof(int)*5; // size of the free block (2 tags for allocated, 2 tags for free, size of allocated, size if free)
      l->previous = last; 
      last->next = l;
      l->next = NULL;
    }
  }
  if (pointer) {
    allocated += size; // total size of the allocated space
    return pointer;
  }
  else {
    my_malloc_error = "Error occurred while allocating memory";
    return NULL;
  }
}
void my_free(void* ptr) {
  int* size = (int*)ptr;
  startFree = ptr - 4;
  int* end = ptr + size[-1] + 4; // top of the freed space
    
  listFree l = (listFree)startFree;
  l->size = size[-1];
  
  if (ptr < (void*)listHeader) { // free below the listHeader
    l->previous = NULL;
    l->next = listHeader;
    listHeader->previous = l;
    startFree[-1] = 0;
    int* tag = ptr + l->size;
    *tag = 0; //set a flag to free
    if (tag[1] == 0) {// the block above is also free
      int* temp = tag + 8; // new block starts there
      listFree ltemp = (listFree)temp;
      l->size += listHeader->size;
      l->next = listHeader->next; 
    }
    listHeader = l;
  }
  else { // free above the listHeader, find its place in the list (to maintain it sorted)
    listFree temp;
    for (temp = listHeader; temp != NULL && temp < (listFree)ptr; temp = temp->next){
      if (temp->next == NULL) {
	last = temp;
      }
    }
    if (temp != NULL) { // add in the middle of the list
      if (startFree[-2] == 0) { // previous block is free
	listFree old = temp->previous;
	old->size += l->size + 12;
	int* tag = ptr + l->size;
	*tag = 0;
      }
      if (end[0] == 0) {  // the block above is free
	l->size += end[1];
	if (temp->next == NULL && l->size > 128) // remove the last block if it's more than 128 kBytes
	  sbrk(-l->size);
	else {
	  l->next = temp->next;
	  temp->previous->next = l;
	  temp->next->previous = l;
	  l->previous = temp->previous;
	  startFree[-1] = 0; 
	  end[-1] = 0;
	}
      }
      if (startFree[-2] != 0 && end[0] != 0) { // both blocks above and below are allocated
	l->previous = temp->previous;
	l->next = temp;
	temp->previous->next = l;
	temp->previous = l;
	int* tag = ptr+ l->size;
	*tag = 0;
	startFree[-1] = 0;
      }
    }
    else { // add at the end of the list
      if (startFree[-2] == 0) { // the block before is free
	listFree old = last;
	old->size += l->size + 12;
	int* tag = ptr + l->size;
	*tag = 0;
      }
      else {
	last->next = l;
	l->next = NULL;
	l->previous = last;
	int* tag = ptr + l->size;
	*tag = 0;
      }
    }
  }
}
void my_mallopt(int policy) {
  p = policy;
}
void my_mallinfo() {
  listFree ptr;
  int sizeFree = 0;
  int max = 0;
  for (ptr = listHeader; ptr != NULL; ptr = ptr->next) {
    sizeFree += ptr->size;
    if (ptr->size > max) {
      max = ptr->size;
    }
  }
  printf("\nTotal number of bytes allocated:\t%d kBytes\nTotal free space:\t\t\t%d kBytes\nThe largest contiguous free space:\t%d kBytes\n", allocated/1024, sizeFree/1024, max/1024);  
}
