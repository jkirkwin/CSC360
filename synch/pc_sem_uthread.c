#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_sem.h"
#include "uthread_mutex_cond.h"
/*
 * This file is for the producer/consumer problem using uthreads with semaphores
 */

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

uthread_sem_t empty_slot, full_slot;
uthread_mutex_t mutex;

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    uthread_sem_wait(empty_slot);
    uthread_mutex_lock(mutex);
    items++;
    printf("Producing --- items = %d\n", items);
    histogram[items]++;
    uthread_mutex_unlock(mutex);
    uthread_sem_signal(full_slot);
  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    uthread_sem_wait(full_slot);
    uthread_mutex_lock(mutex);
    items--;
    printf("Consuming - items = %d\n", items);
    histogram[items]++;
    uthread_mutex_unlock(mutex);
    uthread_sem_signal(empty_slot);
  }
  return NULL;
}

int main (int argc, char** argv) {
  // Init/setup
  uthread_t t[NUM_CONSUMERS + NUM_PRODUCERS];
  uthread_init (4);
  mutex = uthread_mutex_create();
  empty_slot = uthread_sem_create(MAX_ITEMS);
  full_slot = uthread_sem_create(0);
  
  // Run threads
  int i;
  for(i = 0; i < NUM_PRODUCERS; i++) {
    t[i] = uthread_create(producer, NULL);
  }
  for(i = NUM_PRODUCERS; i < NUM_CONSUMERS + NUM_PRODUCERS; i++) {
    t[i] = uthread_create(consumer, NULL);
  }

  // Join threads
  for(i = 0; i < NUM_CONSUMERS + NUM_PRODUCERS; i++) {
    uthread_join(t[i], NULL);
  }

  // Cleanup
  uthread_mutex_destroy(mutex);
  uthread_sem_destroy(full_slot);
  uthread_sem_destroy(empty_slot);

  // Print stats, verify correctness
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);
}
