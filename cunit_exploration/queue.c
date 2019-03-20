/*
 * Jamie Kirkwin 
 * March 15, 2019
 * A quick integer queue implementation to let me get a feel for cunit in the hopes that it will help me in
 * the testing for CSC 360 assignment 3 where I'll be implementing a simple file system.
 */ 

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "queue.h"

#include <cunit.h>

int n; // number of items in the queue
int max_size; // space allocated
int *queue; // a pointer to the first entry of the queue
int head_offset; // the index of the 'oldest' item in the queue (i.e. the one which will be dequeued next)

int main(int argc, char **argv) {
}

/*
 * Initialize globals
 * Return 0 iff success
 */ 
int init() {
    n = 0;
    max_size = INITIAL_SIZE;
    head_offset = 0;
    queue = (int *) malloc(sizeof(int) * max_size);
    return queue == NULL ? -1 : 0;
}

void enqueue(int item) {
    while(n >= max_size) {
        max_size = max_size * 2;
        queue = (int *) realloc(queue, max_size);
    }

    int index = (head_offset + n) % max_size;
    queue[index] = item;
    n++;
}

int dequeue() {
    assert(!is_empty());
    int value = queue[head_offset];
    head_offset = (head_offset + 1) % max_size;
    n--;
    return value;
}

int size() {
    return n;
}

bool is_empty() {
    return size() == 0;
}