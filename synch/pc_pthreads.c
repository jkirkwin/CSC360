// TODO Implement this
// TODO Add this to makefile

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * This file is for the producer/consumer problem using p_threads with blocking
 */ 

/*
 * First, model this problem/solution using pthreads with mutexes and condition variables. Your
 * solution must use at least four threads (two producers and two consumers). You will find TONS
 * of examples of this out there in the wild… Feel free to share what you find on the forum!
 */ 

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 2;
const int NUM_CONSUMERS = 3;
const int NUM_PRODUCERS = 3;

pthread_mutex_t mutex;
pthread_cond_t empty_slot;
pthread_cond_t full_slot;

typedef struct buffer {
    int buffer[MAX_ITEMS];
    int consume_from_index;
    int produce_to_index;
    int occupied;
} buffer_t;


// pass to pthread_create for producing threads
void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
    
    /*
     * lock the mutex
     * while(no slots available) {wait(free slot exists)}
     * add item to free slot
     * signal(full slot exists)
     * unlock the mutex
     */ 
    
    char *msg = (char *) v;
    printf("%s\n", msg);
  }
  return NULL;
}

// pass to pthread_create for consuming threads
void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO

    /*
     * lock the mutex
     * while(no slots are full) {wait(full slot exists)}
     * remove item from buffer
     * signal(slot has been freed)
     * unlock mutex
     * do whatever with the item gained, i.e. consume it
     */ 


    char *msg = (char *) v;
    printf("%s\n", msg);
  }
  return NULL;
}

int main() {
    // Initialize mutex and condition vars
    int ret;
    ret = pthread_mutex_init(&mutex, NULL);
    if(ret) {
        perror("Mutex init failed");
        exit(1);
    }
    ret = pthread_cond_init(&empty_slot, NULL);
    if(ret) {
        perror("empty_slot init failed");
        exit(1);
    }
    ret = pthread_cond_init(&full_slot, NULL);
    if(ret) {
        perror("full_slot init failed");
        exit(1);
    }

    // Create and run the threads
    pthread_t consumers[NUM_CONSUMERS], producers[NUM_PRODUCERS];
    const char *msg1 = "I'm a consumer";
    const char *msg2 = "I'm a producer";
    int i;
    for(i = 0; i < NUM_PRODUCERS; i++) {
        ret = pthread_create(&producers[i], NULL, producer, (void *) msg1);
        if(ret) {
            printf("Producers[%d] creation failed.\n", i);
            perror("Producer creation failed");
            exit(1);
        }
    }
    for(i = 0; i < NUM_CONSUMERS; i++) {
        ret = pthread_create(&consumers[i], NULL, consumer, (void *) msg2);
        if(ret) {
            printf("Consumers[%d] creation failed.\n", i);
            perror("Consumer creation failed");
            exit(1);
        }
    }

    // Join the threads
    for(i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    for(i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    // Finish
    printf("Complete\n");
}