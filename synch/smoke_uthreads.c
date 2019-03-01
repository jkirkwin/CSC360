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
// package_t* make_package(struct Agent *agent, enum Resource resource) {
//   package_t *pkg = (package_t*)malloc(sizeof(package_t));
//   if(!pkg) {
//     perror("Malloc fail");
//     exit(1);
//   } 
//   pkg->agent = agent;
//   pkg->resource = resource;
//   return pkg;
// }

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

typedef struct Smoker_Pkg {
  struct Agent * agent;
  enum Resource resource;
} smoker_package_t;

smoker_package_t* make_package(struct Agent* a, enum Resource r) {
  smoker_package_t *pkg = (smoker_package_t *) malloc(sizeof(smoker_package_t));
  if(!pkg) {
    perror("malloc failed\n");
    exit(1);
  }
  pkg->agent = a;
  pkg->resource = r;
  return pkg;
} 

uthread_mutex_t match_reset_mutex, paper_reset_mutex, tobacco_reset_mutex;
int match_reset_flag, paper_reset_flag, tobacco_reset_flag;
int success;

void* smoker(void *p) {
  package_t *pkg = (package_t *) p;
  struct Agent *agent = pkg->agent;
  enum Resource resource = pkg->resource;
  
  // TODO once we've verified that this approach works
  
  return NULL;
} 

void* match_smoker(void *a) {

  struct Agent *agent = (struct Agent *) a;
  uthread_mutex_lock(agent->mutex);
  while(1) {
    // Lock must be held at the end of every iteration
    
    // Safely change reset flag to true before waiting on next agent iteration
    uthread_mutex_lock(match_reset_mutex);
    match_reset_flag = 1; // true
    
    // Next agent iteration is started. Change flag to indicate thread is not in the reset position.
    uthread_cond_wait(agent->paper); // first resource required for success
    uthread_mutex_lock(match_reset_mutex);
    match_reset_flag = 0; // false
    uthread_mutex_unlock(match_reset_mutex);

    uthread_mutex_lock(agent->mutex);
    
    if(success) {
      continue;
    }
    // // re-signal to allow others to move forward?
    // // TODO examine how this will work with other threads
    // uthread_cond_signal(agent->paper);

    uthread_cond_wait(agent->tobacco); // second resource required for success
    uthread_mutex_lock(agent->mutex);
    if(!success) { // if no other smoker has succeeded on this agent iteration
      // this smoker has now succeeded 

      // ensure the other 2 threads reset by signalling all resources 
      // in same order as agent
      uthread_cond_signal(agent->match);
      uthread_cond_signal(agent->paper);
      uthread_cond_signal(agent->tobacco);

      // busy wait until the other two threads reset
      uthread_mutex_unlock(agent->mutex);
      while(1) {
        uthread_mutex_lock(tobacco_reset_mutex);
        if(!tobacco_reset_flag) {
          uthread_mutex_unlock(tobacco_reset_mutex);
          continue;
        }
        uthread_mutex_unlock(tobacco_reset_mutex);

        uthread_mutex_lock(paper_reset_mutex);
        if(!paper_reset_flag) {
          uthread_mutex_unlock(paper_reset_mutex);
          continue;
        }
        uthread_mutex_unlock(paper_reset_mutex);

        break;
      }
      uthread_mutex_lock(agent->mutex);
      uthread_cond_signal(agent->smoke);
      success = 0;
    }
  }
  return NULL;
}

// add in wakeup condition variables for the 3 smokers
// this will let you simplify the logic a little bit by extracting it from the smoker threads themselves.
// also note that signal does not behave as you thought - signaling only wakes up to one thread, what you 
// had thought was signalling functionality is actually how broadcast works.

int main (int argc, char** argv) {
  printf("Main started\n");
  uthread_init (7);
  struct Agent*  a = createAgent();

  printf("Agent created\n");

  paper_reset_mutex = uthread_mutex_create();
  match_reset_mutex = uthread_mutex_create();
  tobacco_reset_mutex = uthread_mutex_create();

  paper_reset_flag = 0;
  match_reset_flag = 0;
  tobacco_reset_flag = 0;
  success = 0; 

  printf("Reset Mutexes Created\n");

  uthread_t p, m, t;
  m = uthread_create(match_smoker, (void *) a);
  // paper_smoker = uthread_create(smoker, (void *) make_package(a, PAPER));
  // match_smoker = uthread_create(smoker, (void *) make_package(a, MATCH));
  // tobacco_smoker = uthread_create(smoker, (void *) make_package(a, TOBACCO));

  // TODO Busy wait until all 3 are reset before creatinging and joining the agent thread

  uthread_join (uthread_create (agent, a), 0);
  
  printf("Agent thread complete\n");
  
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}
