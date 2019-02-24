#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"
#include "spinlock.h"

/*
 * This file is for the producer/consumer problem using uthreads with 
 * mutexes and condition variables so that all waiting is now blocking
 */ 

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

uthread_mutex_t mutex;
uthread_cond_t empty_space;
uthread_cond_t full_space;

void* producer (void* v) {
  assert(0 <= items && items <= MAX_ITEMS);
  for (int i=0; i<NUM_ITERATIONS; i++) {
    uthread_mutex_lock(mutex);
    while(items >= MAX_ITEMS) {
      producer_wait_count++; // TODO verify that this is what they want 
      uthread_cond_wait(empty_space);
    }
    items++;
    histogram[items]++;
    uthread_cond_signal(full_space);
    assert(0 <= items && items <= MAX_ITEMS);
    uthread_mutex_unlock(mutex);
  }

  assert(0 <= items && items <= MAX_ITEMS);
  return NULL;
}

void* consumer (void* v) {
  assert(0 <= items && items <= MAX_ITEMS);
  for (int i=0; i<NUM_ITERATIONS; i++) {
    uthread_mutex_lock(mutex);
    while(items <= 0) {
      consumer_wait_count++;
      uthread_cond_wait(full_space);
    } 
    items--;
    histogram[items]++;
    assert(0 <= items && items <= MAX_ITEMS);
    uthread_cond_signal(empty_space);
    uthread_mutex_unlock(mutex);
  }
  assert(0 <= items && items <= MAX_ITEMS);
  return NULL;
}

int main (int argc, char** argv) {
  // Setup
  uthread_t t[NUM_CONSUMERS + NUM_PRODUCERS];
  uthread_init(4);
  mutex = uthread_mutex_create();
  full_space = uthread_cond_create(mutex);
  empty_space = uthread_cond_create(mutex);

  producer_wait_count = 0;
  consumer_wait_count = 0;
  int i;
  for(i = 0; i < MAX_ITEMS + 1; i++) {
    histogram[i] = 0;
  }

  // Create Threads and Join
  for(i = 0; i < NUM_PRODUCERS; i++) {
    t[i] = uthread_create(producer, NULL);
  }
  for(i = NUM_PRODUCERS; i < NUM_CONSUMERS + NUM_PRODUCERS; i++) {
    t[i] = uthread_create(consumer, NULL);
  }
  for(i = 0; i < NUM_CONSUMERS + NUM_PRODUCERS; i++) {
    uthread_join(t[i], NULL);
  }
  
  // Report/teardown
  uthread_mutex_destroy(mutex);
  uthread_cond_destroy(full_space);
  uthread_cond_destroy(empty_space);

  printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);
}
