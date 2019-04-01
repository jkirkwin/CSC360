/*
 * Jamie Kirkwin
 * V00875987
 * March 2019
 * CSC 360 Assignment 3
 * 
 * A "Little Log File System" (LLFS)
 */ 

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

/*============================FREE LIST=======================================*/
// TODO Ensure changes go to disk eventually
// TODO Ensure that this is loaded correctly

// Inspiration for free list implementation drawn from 
// http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
unsigned char free_list[512]; // A bit map showing whether a given block is free

/*
 * Returns true iff block is not being used
 * bit is 0 if in use, 1 otherwise
 */ 
bool test_free_list_bit(short block_num) {
    unsigned char byte = free_list[block_num/8];
    unsigned char mask = 1; // 0000  0001
    mask = mask << block_num % 8;
    return byte & mask;
}

/*
 * Mark as in use
 */ 
void clear_free_list_bit(short block_num) {
    unsigned char byte = free_list[block_num/8];
    unsigned char mask = 1; // 0000 0001
    mask = mask << block_num % 8;
    free_list[block_num/8] = byte & mask;
}

/*
 * Mark as available
 */ 
void set_free_list_bit(short block_num) {
    unsigned char byte = free_list[block_num/8];
    unsigned char mask = 1; // 0000 0001 
    mask = mask << block_num % 8;
    free_list[block_num/8] = byte | mask;
}

/*=================================== INODE MAP ==============================*/