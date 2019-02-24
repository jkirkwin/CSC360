// TODO Add this to makefile
// TODO Test this thoroughly

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

// TODO: BUG
// Verified on windows, unverified on linux
// When NUM_ITERATIONS is 6 with 2 consumers and 2 producers, we see that a consumer reports that 
// it has consumed a vary large item which was never produced. This needs to be fixed!

/*
 * This file is for the producer/consumer problem using p_threads with blocking
 *
 * I used the oracle docs https://docs.oracle.com/cd/E19455-01/806-5257/sync-31/index.html 
 * and the lecture 12 slides to wrap my head around this. 
 */ 
#define MAX_ITEMS 10
const int NUM_ITERATIONS = 6;
const int NUM_CONSUMERS = 2;
const int NUM_PRODUCERS = 2;

pthread_mutex_t mutex;
pthread_cond_t empty_slot;
pthread_cond_t full_slot;

typedef struct buffer {
  int buffer[MAX_ITEMS];
  int consume_from_index;
  int produce_to_index;
  int occupied;
} buffer_t;

buffer_t *buff;

/*
 * Error-handling wrapper for malloc
 */
void *emalloc(int size) {
  void *p = malloc(size);
  if(!p) {
    printf("Emalloc Failed. Requested size: %d", size);
    exit(1);
  }
  return p;
}

int produce_item() {
  // TODO return random integer
  return random() % 100;
}

// pass to pthread_create for producing threads
void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    pthread_mutex_lock(&mutex);
    while(buff->occupied >= MAX_ITEMS) {
      pthread_cond_wait(&empty_slot, &mutex);
      }
      int item = produce_item();
      buff->buffer[buff->produce_to_index] = item;
      buff->produce_to_index = (buff->produce_to_index + 1 % MAX_ITEMS);
      buff->occupied++;
      printf("produced item: %d\n", item);
      pthread_cond_signal(&full_slot);
      pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

// pass to pthread_create for consuming threads
void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
    int item;
    pthread_mutex_lock(&mutex);
    while(buff->occupied <= 0){
      pthread_cond_wait(&full_slot, &mutex);
    }
    item = buff->buffer[buff->consume_from_index];
    buff->consume_from_index = (buff->consume_from_index + 1) % MAX_ITEMS;
    buff->occupied--;
    pthread_cond_signal(&empty_slot);
    pthread_mutex_unlock(&mutex);
    printf("Consumed item: %d\n", item);
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

  // Initialize buffer
  buff = emalloc(sizeof(buffer_t));
  buff->occupied = 0;
  buff->produce_to_index = 0;
  buff->consume_from_index = 0;

  // Create and run the threads
  pthread_t consumers[NUM_CONSUMERS], producers[NUM_PRODUCERS];
  int i;
  for(i = 0; i < NUM_PRODUCERS; i++) {
    ret = pthread_create(&producers[i], NULL, producer, NULL);
    if(ret) {
        printf("Producers[%d] creation failed.\n", i);
        perror("Producer creation failed");
        exit(1);
    }
  }
  for(i = 0; i < NUM_CONSUMERS; i++) {
      ret = pthread_create(&consumers[i], NULL, consumer, NULL);
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
  free(buff);
  pthread_cond_destroy(&empty_slot);
  pthread_cond_destroy(&full_slot);
  pthread_mutex_destroy(&mutex);
}