/*
 * 4 threads
 *    Agent - has infinite supply of all resources
 *    3 smokers - each has an infinite supply of 1 resource
 *              - each has a different resource
 * 3 Resources
 *    Tobacco
 *    Paper
 *    Matches
 * 
 * Smoker threads loop, attempting to smoke. To smoke they need one 
 * unit of the 2 resources they don't already have.
 * 
 * Agent loops, randomly choosing 2 resources to make available 
 * to the smokers. Use random() % 3 for this.
 * 
 * Each loop, one of the smokers should be able to smoke.
 * 
 * Agent can only communicate by signaling a resource is available 
 * via a condition variable
 * 
 * Smokers cannot ask the agent which resources it is currently providing.
 * 
 * Agent can't know which smokers have/need which resources
 * 
 * On each loop, agent must wait on a condition variable indicating a smoker
 * has succeeded
 * 
 * need a condition variable for each resource, signaled by the agent
 * 
 * need a condition variable for smoker threads to signal when they are
 * successful that wakes up the agent
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create (agent->mutex);
  agent->match   = uthread_cond_create (agent->mutex);
  agent->tobacco = uthread_cond_create (agent->mutex);
  agent->smoke   = uthread_cond_create (agent->mutex);
  return agent;
}

//
// TODO
// You will probably need to add some procedures and struct etc.
//

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked


/* 
 * A package containing the agent giving resources and the resource a smoker
 * has by default.
 */ 
typedef struct Smoker_Package {
  struct Agent *agent;
  enum Resource resource;
} package_t;

/*
 * Helper function to create a new smoker package
 */ 
package_t* make_package(struct Agent *agent, enum Resource resource) {
  package_t *pkg = (package_t*)malloc(sizeof(package_t));
  if(!pkg) {
    perror("Malloc fail");
    exit(1);
  } 
  pkg->agent = agent;
  pkg->resource = resource;
  return pkg;
}

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
  
  uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        uthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        uthread_cond_signal (a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      uthread_cond_wait (a->smoke);
    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}


int paper_flag, match_flag, tobacco_flag;

void paper_smoker(struct Agent *agent) {
  paper_flag = 0;
  uthread_mutex_lock(agent->mutex); // To allow us to get into the loop and release immediately
  while(1) {
    // Give up lock once the thread is reset.
    // This way the agent can't go until all three have been reset (hopefully)
    uthread_mutex_unlock(agent->mutex); 

    uthread_cond_wait(agent->match); // these are in the order that the agent signals them
    uthread_cond_wait(agent->tobacco); 

    uthread_mutex_lock(agent->mutex);
    
    // flag is set if another thread was able to smoke 
    if(paper_flag) { 
      continue;
    }

    // If we got here, it means that this is the thread that can smoke on this round
    tobacco_flag = 1; // set flags to ensure other threads are reset before control is given back to agent
    match_flag = 1;

    // allow  the other two threads to reset and busy wait until this happens 
    uthread_cond_signal(agent->match); // same order as agent
    uthread_cond_signal(agent->paper);
    uthread_cond_signal(agent->tobacco);
    uthread_mutex_unlock(agent->mutex); 
    while(tobacco_flag || match_flag);

    uthread_mutex_lock(agent->mutex); // prevent agent from continuing until this thread has been reset too
    uthread_cond_signal(agent->smoke);
    paper_flag = 0;
  }
}

void match_smoker(struct Agent *agent) {
  match_flag = 0;
  uthread_mutex_lock(agent->mutex); // To allow us to get into the loop and release immediately
  while(1) {
    // Give up lock once the thread is reset.
    // This way the agent can't go until all three have been reset (hopefully)
    uthread_mutex_unlock(agent->mutex); 

    uthread_cond_wait(agent->paper); // these are in the order that the agent signals them
    uthread_cond_wait(agent->tobacco); 

    uthread_mutex_lock(agent->mutex);
    
    // flag is set if another thread was able to smoke 
    if(match_flag) { 
      continue;
    }

    // If we got here, it means that this is the thread that can smoke on this round
    tobacco_flag = 1; // set flags to ensure other threads are reset before control is given back to agent
    paper_flag = 1;

    // allow  the other two threads to reset and busy wait until this happens 
    uthread_cond_signal(agent->match);  // same order as agent
    uthread_cond_signal(agent->paper);
    uthread_cond_signal(agent->tobacco);
    uthread_mutex_unlock(agent->mutex); 
    while(tobacco_flag || paper_flag);

    uthread_mutex_lock(agent->mutex); // prevent agent from continuing until this thread has been reset too
    uthread_cond_signal(agent->smoke);
    match_flag = 0;
  }
}

void tobacco_smoker(struct Agent *agent) {
  tobacco_flag = 0;
  uthread_mutex_lock(agent->mutex); // To allow us to get into the loop and release immediately
  while(1) {
    // Give up lock once the thread is reset.
    // This way the agent can't go until all three have been reset (hopefully)
    uthread_mutex_unlock(agent->mutex); 

    uthread_cond_wait(agent->match);  // these are in the order that the agent signals them  
    uthread_cond_wait(agent->paper);

    uthread_mutex_lock(agent->mutex);
    
    // flag is set if another thread was able to smoke 
    if(tobacco_flag) { 
      continue;
    }

    // If we got here, it means that this is the thread that can smoke on this round
    match_flag = 1; // set flags to ensure other threads are reset before control is given back to agent
    paper_flag = 1;

    // allow  the other two threads to reset and busy wait until this happens 
    uthread_cond_signal(agent->match); // same order as agent
    uthread_cond_signal(agent->paper);
    uthread_cond_signal(agent->tobacco);
    uthread_mutex_unlock(agent->mutex); 
    while(match_flag || paper_flag);

    uthread_mutex_lock(agent->mutex); // prevent agent from continuing until this thread has been reset too
    uthread_cond_signal(agent->smoke);
    tobacco_flag = 0;
  }
}

/*
 * The procedure for a smoker thread. 
 * 
 * Initially I will write this for a smoker who has the paper resource but not the other 2.
 * If the solution is parameterizable, I will parameterize it. Otherwise I will rename this
 * to paper_smoker() and write tobacco_smoker() and matches_smoker(). Hopefully that doesn't happen.
 */ 
void* smoker(void *arg) {
  package_t *pkg = (package_t *) arg;
  switch(pkg->resource) {
    case PAPER:
      paper_smoker(pkg->agent);
      break;
    case TOBACCO:
      tobacco_smoker(pkg->agent);
      break;
    case MATCH:
      match_smoker(pkg->agent);
      break;

  }
  return NULL;
  // need two condition variables to wait on.
  // If both** are signaled, smoke.
  // Otherwise, go back to waiting on both.

  //idea - supposing the two cond vars are cond1, cond2
  /*
   
    wait(cond1);
    wait(cond2);
    lock(mutex);
    if(both resources available) {
      smoke
      release lock
    } else {
      release lock and continue
    }
    -------------------------

    paper thread:
      wait on tobacco
      wait on matches

    tobacco thread:
      wait on paper
      wait on matches

    matches thread:
      wait on tobacco
      wait on paper


    what if smokers could communicate?

    as we have it now, all 3 smokers will wake up from one of the waits, but only one will actually get past both.
    what if the successful smoker could cause the other two to be reset?

    wait(cond1);
    wait(cond2);
    lock(mutex);
    if(need to reset) {
      continue
    }

    need a way to tell if wait-wakeups happened from the same batch of signals


   */

  // Continue until agent stops providing resources
  // while(1) {
    // wait on two condition variables?
    // alternatively we could create 3 additional threads to handle each smoker, but we need the same logic either way it seems to me.

    // TODO ensure this only happens when smoker can get all 3 resources
    // uthread_cond_signal(/*todo*/)

  // }
}

int main (int argc, char** argv) {
  printf("Main started\n");
  uthread_init (7);
  struct Agent*  a = createAgent();

  printf("Agent created\n");

  uthread_t paper_smoker, matche_smoker, tobacco_smoker;
  paper_smoker = uthread_create(smoker, (void *) make_package(a, PAPER));
  matche_smoker = uthread_create(smoker, (void *) make_package(a, MATCH));
  tobacco_smoker = uthread_create(smoker, (void *) make_package(a, TOBACCO));

  uthread_join (uthread_create (agent, a), 0);
  
  printf("Agent thread complete\n");
  
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}
