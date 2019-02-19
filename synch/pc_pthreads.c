// TODO Implement this
// TODO Add this to makefile

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * This file is for the producer/consumer problem using p_threads with blocking
 */ 

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

// pass to pthread_create for producing threads
void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
    
    
    char *msg = (char *) v;
    printf("%s\n", msg);
  }
  return NULL;
}

// pass to pthread_create for consuming threads
void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO


    char *msg = (char *) v;
    printf("%s\n", msg);
  }
  return NULL;
}

int main() {
    // Make the threads
    pthread_t consumer1, producer1;
    const char *msg1 = "I'm a consumer";
    const char *msg2 = "I'm a producer";
    int ret1, ret2;

    ret1 = pthread_create(&consumer1, NULL, consumer, (void *) msg1);
    if(ret1) {
        // error
        perror("Consumer creation failed");
    }

    ret2 = pthread_create(&producer1, NULL, producer, (void *) msg2);
    if(ret2) {
        // error
        perror("Producer creation failed");
    }

    // Join the threads
    pthread_join(consumer1, NULL);
    pthread_join(producer1, NULL);

    // Finish
    printf("Complete\n\n");
}