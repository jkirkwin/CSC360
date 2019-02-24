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
const int NUM_ITERATIONS = 1; // TODO Set to a large number
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

spinlock_t lock;

void* producer (void* v) {
  printf("Producer\n");
  assert(0 <= items && items <= MAX_ITEMS);

  for (int i=0; i<NUM_ITERATIONS; i++) {
    producer_wait_count++; 
    while(items >= MAX_ITEMS);  // Wait/Spin
    spinlock_lock(&lock);  // Aquire lock
    if(items >= MAX_ITEMS) {  // Go back to waiting
      spinlock_unlock(&lock);  
      i--;
      continue;
    } else {  // Produce 
      items++;
      histogram[items]++;
      spinlock_unlock(&lock);  
    }
  }

  assert(0 <= items && items <= MAX_ITEMS);
  return NULL;
}

void* consumer (void* v) {
  printf("Consumer\n");
  assert(0 <= items && items <= MAX_ITEMS);
  
  for (int i=0; i<NUM_ITERATIONS; i++) {
    consumer_wait_count++;
    while(items <= 0); // Wait/Spin
    spinlock_lock(&lock); // Aquire lock
    if(items <= 0) { // Go back to waiting
      spinlock_unlock(&lock);
      i--;
      continue;

    } else { // Consume
      items--;
      histogram[items]++;
      spinlock_unlock(&lock);
    }
  }
  
  assert(0 <= items && items <= MAX_ITEMS);
  return NULL;
}

int main (int argc, char** argv) {
  
  // Setup
  uthread_t t[NUM_CONSUMERS + NUM_PRODUCERS];
  uthread_init(4);
  spinlock_create(&lock);

  producer_wait_count = 0;
  consumer_wait_count = 0;
  int i;
  for(i = 0; i < MAX_ITEMS + 1; i++) {
    histogram[i] = 0;
  }

  // Create Threads and Join
  printf("Init done\n");
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
  // TODO There doesn't appear to be a spinlock_destroy function()...
  printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);
}
