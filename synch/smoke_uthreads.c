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

// lets us track resources (thread-safely)
int flag; 
uthread_mutex_t flag_mutex;

// single cond vars for smokers to wait on. smoker with resource 1 waits
// on wakeups[resource 2 | resource 3]
// still uses the agent's mutex
uthread_cond_t wakeups[5];

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
      
      printf("Starting agent loop\n");
      
      // uthread_mutex_lock(flag_mutex);
      flag = 0; // Added to assignment skeleton
      // uthread_mutex_unlock(flag_mutex);

      printf("flag set to zero in agent\n");

      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        printf("\tagent signals match\n");
        uthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        printf("\tagent signals paper\n");
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        printf("\tagent signals tobacco\n");
        uthread_cond_signal (a->tobacco);
      }
      printf("agent waiting for smoke signal\n");
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      uthread_cond_wait (a->smoke);
    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

// need 3 condition variables to be used to wakeup the 3 smokers
// sum must be wiped at the start of every agent iteration
// sum needs a mutex to protect it
// each smoker needs a pointer one of these as well as to the agent so that they can signal smoke
// smoker package should also contain a resource enum to let us print the resource name

typedef struct Smoker_Package {
  enum Resource resource;
  uthread_cond_t wait_on;
  struct Agent *agent;
} smoker_pkg_t;

typedef struct Listener_Package {
  uthread_cond_t listen_for;
  uthread_mutex_t mutex;
  enum Resource resource;
} listener_pkg_t;

smoker_pkg_t* get_smoker_package(enum Resource r, uthread_cond_t wait_on, struct Agent *a) {
  smoker_pkg_t *pkg = (smoker_pkg_t *) malloc(sizeof(smoker_pkg_t));
  if(!pkg) {
    perror("Smoker pkg creation failed");
    exit(1);
  }
  pkg->resource = r;
  pkg->wait_on = wait_on;
  pkg->agent = a;
  return pkg;
}

listener_pkg_t* get_listener_package(uthread_cond_t listen, uthread_mutex_t m) {
  listener_pkg_t *pkg = (listener_pkg_t *) malloc(sizeof(listener_pkg_t));
  if(!pkg) {
    perror("listener pkg creation failed");
    exit(2);
  }
  pkg->listen_for = listen;
  pkg->mutex = m;
}

void* smoker(void *p) {
  smoker_pkg_t *pkg = (smoker_pkg_t *) p;
  char * rsrc_name = resource_name[pkg->resource];
  printf("%s smoker created\n", rsrc_name);
  pkg->agent;
  uthread_mutex_lock(pkg->agent->mutex);
  while(1) {
    printf("%s smoker ready\n", rsrc_name);
    uthread_cond_wait(pkg->wait_on);
    printf("%s smoker woken up\n", rsrc_name);
    uthread_mutex_lock(pkg->agent->mutex);
    uthread_cond_signal(pkg->agent->smoke);
    smoke_count[pkg->resource]++; 
    printf("%s smoker smokes\n", rsrc_name);
  }
  uthread_mutex_unlock(pkg->agent->mutex); // better safe than deadlocked
  return NULL;
}


void* listener(void *p) {
  listener_pkg_t *pkg = (listener_pkg_t*) p;
  char * rsrc_name = resource_name[pkg->resource];
  printf("%s listener started\n", rsrc_name);

  uthread_mutex_lock(pkg->mutex);
  while(1) {
    printf("%s listener ready\n", rsrc_name);
    uthread_cond_wait(pkg->listen_for);
    printf("%s listener woken up\n", rsrc_name);
    // resource signalled by agent
    uthread_mutex_lock(flag_mutex);
    flag += pkg->resource;
    // wake up the appropriate smoker if 2 signals have been recorded in flag
    switch(flag) {
      case MATCH + TOBACCO:
        uthread_cond_signal(wakeups[MATCH | TOBACCO]);
        break;
      case PAPER + TOBACCO:
        uthread_cond_signal(wakeups[PAPER | TOBACCO]);
        break;
      case PAPER + MATCH:
        uthread_cond_signal(wakeups[PAPER | MATCH]);
        break;
    }
    uthread_mutex_unlock(flag_mutex);
  }
  uthread_mutex_unlock(pkg->mutex); // I don't think this will ever execute, but better safe than deadlocked
  return NULL;
}

int main (int argc, char** argv) {
  printf("Main started\n");
  uthread_init (7);
  struct Agent*  a = createAgent();

  flag_mutex = uthread_mutex_create();
  uthread_mutex_lock(flag_mutex);
  flag = 0;
  printf("flag set to 0 in main\n");
  uthread_mutex_unlock(flag_mutex);
  printf("Locked and unlocked FLAG in main\n");

  wakeups[MATCH | TOBACCO] = uthread_cond_create(a->mutex);
  wakeups[MATCH | PAPER] = uthread_cond_create(a->mutex);
  wakeups[PAPER | TOBACCO] = uthread_cond_create(a->mutex);

  // Start listener threads and wait until they are ready to go
  listener_pkg_t *lp_match, *lp_paper, *lp_tobacco;
  lp_match = get_listener_package(a->match, a->mutex);
  lp_paper = get_listener_package(a->paper, a->mutex);
  lp_tobacco = get_listener_package(a->tobacco, a->mutex);

  uthread_t match_listener, paper_listener, tobacco_listener;
  match_listener = uthread_create(listener, lp_match);
  paper_listener = uthread_create(listener, lp_paper);
  tobacco_listener = uthread_create(listener, lp_tobacco);
  sleep(2); // TODO add a global flag and mutex for the listeners to increment once they are ready to go instead of this

  printf("listener threads created in main\n");

  // Make smoker threads and wait until they are ready to go
  smoker_pkg_t *sp_match, *sp_paper, *sp_tobacco;
  sp_match = get_smoker_package(MATCH, wakeups[PAPER | TOBACCO], a); 
  sp_paper = get_smoker_package(PAPER, wakeups[MATCH | TOBACCO], a); 
  sp_tobacco = get_smoker_package(TOBACCO, wakeups[PAPER | MATCH], a); 

  uthread_t match_smoker, paper_smoker, tobacco_smoker;
  match_smoker = uthread_create(smoker, sp_match);
  paper_smoker = uthread_create(smoker, sp_paper);
  tobacco_smoker = uthread_create(smoker, sp_tobacco);
  sleep(3); // TODO add a global flag and mutex for smokers to increment once they are ready to go

  printf("smoker threads created in main\n");

  uthread_join (uthread_create (agent, a), 0);

  // TODO teardown
  uthread_mutex_destroy(flag_mutex);
  
  uthread_mutex_destroy(a->mutex);

  uthread_cond_destroy(a->match);
  uthread_cond_destroy(a->tobacco);
  uthread_cond_destroy(a->paper);
  
  uthread_cond_destroy(wakeups[MATCH | TOBACCO]);
  uthread_cond_destroy(wakeups[MATCH | PAPER]);
  uthread_cond_destroy(wakeups[PAPER | TOBACCO]);

  // Verify
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}
