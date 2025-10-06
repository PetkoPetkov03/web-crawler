#ifndef __WBC_QUEUE__
#define __WBC_QUEUE__
#include <stdio.h>
#include <stdlib.h>

#define MIN_SIZE 5;

typedef struct __fifo_queue__ {
  void** buffer;
  void* front;
  size_t csize;
  size_t capacity;
} fqueue;

void init_queue(fqueue* queue)
{
  queue = (fqueue*)malloc(sizeof(fqueue));

  if(!queue) {
    perror("");
    exit(1);
  }

  queue->capacity = MIN_SIZE;
  queue->csize = 0;
  queue->buffer = (void**)malloc(sizeof(void*) * queue->capacity);

  if(!queue->buffer) {
    perror("");
    exit(1);
  }
}

void qpush(fqueue* queue, void* data) {
  if(!queue) {
    perror("");
    exit(1);
  }

  if(queue->capacity == (queue->csize-1)) {
    queue->capacity *= 2;

    queue->buffer = (void**)realloc(queue->buffer, queue->capacity);
  }

  queue->buffer[queue->csize++] = data;
}

#endif
