#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_ITERATIONS 1000

// TODO put this back for sumbmission
// #ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
// #else
// #define VERBOSE_PRINT(S, ...) ;
// #endif

void * emalloc(size_t size) {
    void * ptr = malloc(size);
    if(!ptr) {
        perror("emalloc failed");
        exit(1);
    return ptr;
    }
}

struct Agent {
  pthread_mutex_t mutex;
  pthread_cond_t match;
  pthread_cond_t paper;
  pthread_cond_t tobacco;
  pthread_cond_t smoke;
};

struct Agent* createAgent() {
  struct Agent *agent = emalloc (sizeof (struct Agent));
  pthread_mutex_init(&(agent->mutex), NULL);
  pthread_cond_init(&(agent->paper), NULL);
  pthread_cond_init(&(agent->match), NULL);
  pthread_cond_init(&(agent->tobacco), NULL);
  pthread_cond_init(&(agent->smoke), NULL);
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

int flag;

pthread_cond_t wakeup_match, wakeup_paper, wakeup_tobacco;

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
  
  pthread_mutex_lock (&(a->mutex));
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        pthread_cond_signal (&(a->match));
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        pthread_cond_signal (&(a->paper));
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        pthread_cond_signal (&(a->tobacco));
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      pthread_cond_wait (&(a->smoke), &(a->mutex));
    }
  pthread_mutex_unlock (&(a->mutex));
  return NULL;
}

void check_flag() {
  switch(flag) {
    case MATCH + PAPER:
      pthread_cond_signal(&wakeup_tobacco);
      flag = 0;
      break;

    case MATCH + TOBACCO:
      pthread_cond_signal(&wakeup_paper);
      flag = 0;
      break;

    case PAPER + TOBACCO:
      pthread_cond_signal(&wakeup_match);
      flag = 0;
      break;
  }
}

void* match_listener(void *a) {
  VERBOSE_PRINT("Match listener running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(&(agent->mutex));
  while(1) {
    VERBOSE_PRINT("Match listener ready\n");
    pthread_cond_wait(&(agent->match), &(agent->mutex));
    VERBOSE_PRINT("Match listener woken up\n");
    flag += MATCH;
    check_flag();
  }
}

void* paper_listener(void *a) {
  VERBOSE_PRINT("Paper listener running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(&(agent->mutex));
  while(1) {
    VERBOSE_PRINT("Paper listener ready\n");
    pthread_cond_wait(&(agent->paper), &(agent->mutex));
    VERBOSE_PRINT("Paper listener woken up\n");
    flag += PAPER;
    check_flag();
  }
}

void* tobacco_listener(void *a) {
  VERBOSE_PRINT("Tobacco listener running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(&(agent->mutex));
  while(1) {
    VERBOSE_PRINT("Tobacco listener ready\n");
    pthread_cond_wait(&(agent->tobacco), &(agent->mutex));
    VERBOSE_PRINT("Tobacco listener woken up\n");
    flag += TOBACCO;
    check_flag();
  }
}

void* match_smoker(void *a) {
  VERBOSE_PRINT("Match smoker running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(&(agent->mutex));
  while(1) {
    VERBOSE_PRINT("Match smoker ready\n");
    pthread_cond_wait(&wakeup_match, &(agent->mutex));
    VERBOSE_PRINT("Match smoker SMOKING\n");
    pthread_cond_signal(&(agent->smoke));
    smoke_count[MATCH]++;
  }
}

void* paper_smoker(void *a) {
  VERBOSE_PRINT("Paper smoker running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(&(agent->mutex));
  while(1) {
    VERBOSE_PRINT("Paper smoker ready\n");
    pthread_cond_wait(&wakeup_paper, &(agent->mutex));
    VERBOSE_PRINT("Paper smoker SMOKING\n");
    pthread_cond_signal(&(agent->smoke));
    smoke_count[PAPER]++;
  }
}

void* tobacco_smoker(void *a) {
  VERBOSE_PRINT("Tobacco smoker running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(&(agent->mutex));
  while(1) {
    VERBOSE_PRINT("Tobacco smoker ready\n");
    pthread_cond_wait(&wakeup_tobacco, &(agent->mutex));
    VERBOSE_PRINT("Tobacco smoker SMOKING\n");
    pthread_cond_signal(&(agent->smoke));
    smoke_count[PAPER]++;
  }
}

int main (int argc, char** argv) {  
  struct Agent*  a = createAgent();
  flag = 0;
  
  VERBOSE_PRINT("MAIN STARTED\n");

  // Init wakeup cond vars
  pthread_cond_init(&wakeup_match, NULL);
  pthread_cond_init(&wakeup_paper, NULL);
  pthread_cond_init(&wakeup_tobacco, NULL);

  VERBOSE_PRINT("WAKEUPS INITIALIZED\n");
  
  pthread_t agent_pt;
  pthread_t match_smoker_pt, paper_smoker_pt, tobacco_smoker_pt; 
  pthread_t match_listener_pt, paper_listener_pt, tobacco_listener_pt;

  // Create smoker threads
  pthread_create(&match_smoker_pt, NULL, match_smoker, a);
  pthread_create(&paper_smoker_pt, NULL, paper_smoker, a);
  pthread_create(&tobacco_smoker_pt, NULL, tobacco_smoker, a);
  
  VERBOSE_PRINT("SMOKER THREADS CREATED\n");

  // TODO ensure smokers have started the loop (i.e. are waiting)


  // Create listeners
  pthread_create(&match_listener_pt, NULL, match_listener, a);
  pthread_create(&paper_listener_pt, NULL, paper_listener, a);
  pthread_create(&tobacco_listener_pt, NULL, tobacco_listener, a);
  
  VERBOSE_PRINT("LISTENER THREADS CREATED\n");

  // TODO ensure listeners have started the loop (i.e. are waiting)


  // Create agent and join it
  VERBOSE_PRINT("CREATING AGENT THREAD\n");
  pthread_create(&agent_pt, NULL, agent, a);
  VERBOSE_PRINT("AGENT CREATED, JOINING");
  pthread_join(agent_pt, NULL);

  // Teardown
  pthread_mutex_destroy(&(a->mutex));
  pthread_cond_destroy(&(a->match));  
  pthread_cond_destroy(&(a->tobacco));
  pthread_cond_destroy(&(a->paper));
  pthread_cond_destroy(&(a->smoke));  
  free(a);
  pthread_cond_destroy(&wakeup_match);
  pthread_cond_destroy(&wakeup_paper);
  pthread_cond_destroy(&wakeup_tobacco);

  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);

}
