#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 10000


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

int flag; 

uthread_cond_t match_wakeup, paper_wakeup, tobacco_wakeup;

void check_flag() {
  switch(flag) {
    case MATCH + PAPER:
      uthread_cond_signal(tobacco_wakeup);
      flag = 0;
      break;

    case MATCH + TOBACCO:
      uthread_cond_signal(paper_wakeup);
      flag = 0;
      break;

    case PAPER + TOBACCO:
      uthread_cond_signal(match_wakeup);
      flag = 0;
      break;
  }
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
  printf("Agent thread running\n");
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

void* match_listener(void *a) {
  VERBOSE_PRINT("Match listener running\n");
  struct Agent *agent = (struct Agent *) a;
  uthread_mutex_lock(agent->mutex);
  while(1) {
    VERBOSE_PRINT("Match listener ready\n");
    uthread_cond_wait(agent->match);
    VERBOSE_PRINT("Match listener woken up\n");
    flag += MATCH;
    check_flag();
  }
}

void* paper_listener(void *a) {
  VERBOSE_PRINT("Paper listener running\n");
  struct Agent *agent = (struct Agent *) a;
  uthread_mutex_lock(agent->mutex);
  while(1) {
    VERBOSE_PRINT("Paper listener ready\n");
    uthread_cond_wait(agent->paper);
    VERBOSE_PRINT("Paper listener woken up\n");
    flag += PAPER;
    check_flag();
  }
}

void* tobacco_listener(void *a) {
  VERBOSE_PRINT("Tobacco listener running\n");
  struct Agent *agent = (struct Agent *) a;
  uthread_mutex_lock(agent->mutex);
  while(1) {
    VERBOSE_PRINT("Tobacco listener ready\n");
    uthread_cond_wait(agent->tobacco);
    VERBOSE_PRINT("Tobacco listener woken up\n");
    flag += TOBACCO;
    check_flag();
  }
}

void* match_smoker(void *a) {
  VERBOSE_PRINT("Match smoker running\n");
  struct Agent *agent = (struct Agent *) a;
  uthread_mutex_lock(agent->mutex);
  while(1) {
    VERBOSE_PRINT("Match smoker ready\n");
    uthread_cond_wait(match_wakeup);
    VERBOSE_PRINT("Match smoker SMOKING\n");
    uthread_cond_signal(agent->smoke);
    smoke_count[MATCH]++;
  }
}

void* paper_smoker(void *a) {
  VERBOSE_PRINT("Paper smoker running\n");
  struct Agent *agent = (struct Agent *) a;
  uthread_mutex_lock(agent->mutex);
  while(1) {
    VERBOSE_PRINT("Paper smoker ready\n");
    uthread_cond_wait(paper_wakeup);
    VERBOSE_PRINT("Paper smoker SMOKING\n");
    uthread_cond_signal(agent->smoke);
    smoke_count[PAPER]++;
  }
}

void* tobacco_smoker(void *a) {
  VERBOSE_PRINT("Tobacco smoker running\n");
  struct Agent *agent = (struct Agent *) a;
  uthread_mutex_lock(agent->mutex);
  while(1) {
    VERBOSE_PRINT("Tobacco smoker ready\n");
    uthread_cond_wait(tobacco_wakeup);
    VERBOSE_PRINT("Tobacco smoker SMOKING\n");
    uthread_cond_signal(agent->smoke);
    smoke_count[TOBACCO]++;
  }
}

int main (int argc, char** argv) {
  uthread_init (7);
  struct Agent*  a = createAgent();
  flag = 0;

  VERBOSE_PRINT("MAIN STARTED\n");

  match_wakeup = uthread_cond_create(a->mutex);
  paper_wakeup = uthread_cond_create(a->mutex);
  tobacco_wakeup = uthread_cond_create(a->mutex);

  VERBOSE_PRINT("WAKEUPS INITIALIZED\n");

  uthread_t match_smoker_ut, paper_smoker_ut, tobacco_smoker_ut; 
  uthread_t match_listener_ut, paper_listener_ut, tobacco_listener_ut;

  // Start listener threads and wait until they are ready to go
  match_listener_ut = uthread_create(match_listener, a);
  paper_listener_ut = uthread_create(paper_listener, a);
  tobacco_listener_ut = uthread_create(tobacco_listener, a);
  
  sleep(2); // TODO add a global flag and mutex for the listeners to increment once they are ready to go instead of this

  VERBOSE_PRINT("LISTENERS CREATED\n");

  // Make smoker threads and wait until they are ready to go
  match_smoker_ut = uthread_create(match_smoker, a);
  paper_smoker_ut = uthread_create(paper_smoker, a);
  paper_smoker_ut = uthread_create(tobacco_smoker, a);
  
  sleep(3); // TODO add a global flag and mutex for smokers to increment once they are ready to go

  VERBOSE_PRINT("SMOKERS CREATED. RUNNING AGENT\n");

  uthread_join (uthread_create (agent, a), 0);

  // Teardown
  uthread_mutex_destroy(a->mutex);

  uthread_cond_destroy(a->match);
  uthread_cond_destroy(a->tobacco);
  uthread_cond_destroy(a->paper);
  
  uthread_cond_destroy(match_wakeup);
  uthread_cond_destroy(paper_wakeup);
  uthread_cond_destroy(tobacco_wakeup);

  // Verify
  VERBOSE_PRINT("Smoke  counts: %d matches, %d paper, %d tobacco\n", smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
  VERBOSE_PRINT("Signal counts: %d matches, %d paper, %d tobacco\n", signal_count [MATCH], signal_count [PAPER], signal_count [TOBACCO]);
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}
