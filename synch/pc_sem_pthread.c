#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>


/*
 * This file is for the producer/consumer problem using pthreads with semaphores
 */

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

// Two semaphores and one mutex
sem_t full_space, empty_space;
pthread_mutex_t mutex; 

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    sem_wait(&empty_space);
    pthread_mutex_lock(&mutex);
    items++;
    printf("Producing --- Items = %d\n", items);
    histogram[items]++;
    pthread_mutex_unlock(&mutex);
    sem_post(&full_space);
  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    sem_wait(&full_space);
    pthread_mutex_lock(&mutex);
    items--;
    printf("Consuming - Items = %d\n", items);
    histogram[items]++;
    pthread_mutex_unlock(&mutex);
    sem_post(&empty_space);
  }
  return NULL;
}

int main (int argc, char** argv) {

  // Initialize mutex and semaphores
  int ret;
  ret = pthread_mutex_init(&mutex, NULL);
  if(ret) {
    perror("Mutex init failed");
    exit(1);
  }
  ret = sem_init(&empty_space, 0, MAX_ITEMS);
  if(ret) {
    perror("empty_slot init failed");
    exit(1);
  }
  ret = sem_init(&full_space, 0, 0);
  if(ret) {
    perror("full_slot init failed");
    exit(1);
  }

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

  // Join threads
  for(i = 0; i < NUM_CONSUMERS; i++) {
    pthread_join(consumers[i], NULL);
  }
  for(i = 0; i < NUM_PRODUCERS; i++) {
    pthread_join(producers[i], NULL);
  }

  // Destroy semaphores and mutex
  sem_destroy(&empty_space);
  sem_destroy(&full_space);
  pthread_mutex_destroy(&mutex);

  // Print stats
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
}
