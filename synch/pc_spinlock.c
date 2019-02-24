#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"
#include "spinlock.h"

/*
 * This file is for the producer/consumer problem using uthreads with spinlocks
 */ 

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

void* producer (void* v) {
  printf("Producer\n");
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO

    // in here we want to increment <items> safely

    // we also need to update producer_wait_count and histogram[whatever the new value is]
  }
  return NULL;
}

void* consumer (void* v) {
  printf("Consumer\n");
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO

    // in here we want to decrement <items> safely

    // we also need to update consumer_wait_count and histogram[whatever the new value is]
  }
  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[4];
  uthread_init(4);
  
  // TODO: Create Threads and Join
  printf("Main\n");
  uthread_t prod;
  uthread_t cons;
  prod = uthread_create(producer, NULL);
  cons = uthread_create(consumer, NULL);
  uthread_join(prod, NULL);
  uthread_join(cons, NULL);

  printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);
}
