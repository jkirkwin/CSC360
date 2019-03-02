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

struct Agent {
  pthread_mutex_t *mutex;
  pthread_cond_t *match;
  pthread_cond_t *paper;
  pthread_cond_t *tobacco;
  pthread_cond_t *smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  pthread_mutex_init(agent->mutex, NULL);
  pthread_cond_init(agent->paper, NULL);
  pthread_cond_init(agent->match, NULL);
  pthread_cond_init(agent->tobacco, NULL);
  pthread_cond_init(agent->smoke, NULL);
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

pthread_cond_t *wakeup_match, *wakeup_paper, *wakeup_tobacco;

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
  
  pthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        pthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        pthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        pthread_cond_signal (a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      pthread_cond_wait (a->smoke, a->mutex);
    }
  pthread_mutex_unlock (a->mutex);
  return NULL;
}

void check_flag() {
  switch(flag) {
    case MATCH + PAPER:
      pthread_cond_signal(wakeup_tobacco);
      flag = 0;
      break;

    case MATCH + TOBACCO:
      pthread_cond_signal(wakeup_paper);
      flag = 0;
      break;

    case PAPER + TOBACCO:
      pthread_cond_signal(wakeup_match);
      flag = 0;
      break;
  }
}

void* match_listener(void *a) {
  VERBOSE_PRINT("Match listener running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(agent->mutex);
  while(1) {
    VERBOSE_PRINT("Match listener ready\n");
    pthread_cond_wait(agent->match, agent->mutex);
    VERBOSE_PRINT("Match listener woken up\n");
    flag += MATCH;
    check_flag();
  }
}

void* paper_listener(void *a) {
  VERBOSE_PRINT("Paper listener running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(agent->mutex);
  while(1) {
    VERBOSE_PRINT("Paper listener ready\n");
    pthread_cond_wait(agent->paper, agent->mutex);
    VERBOSE_PRINT("Paper listener woken up\n");
    flag += PAPER;
    check_flag();
  }
}

void* tobacco_listener(void *a) {
  VERBOSE_PRINT("Tobacco listener running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(agent->mutex);
  while(1) {
    VERBOSE_PRINT("Tobacco listener ready\n");
    pthread_cond_wait(agent->tobacco, agent->mutex);
    VERBOSE_PRINT("Tobacco listener woken up\n");
    flag += TOBACCO;
    check_flag();
  }
}

void* match_smoker(void *a) {
  VERBOSE_PRINT("Match smoker running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(agent->mutex);
  while(1) {
    pthread_cond_wait(wakeup_match, agent->mutex);
    pthread_cond_signal(agent->smoke);
    smoke_count[MATCH]++;
  }
}

void* paper_smoker(void *a) {
  VERBOSE_PRINT("Paper smoker running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(agent->mutex);
  while(1) {
    pthread_cond_wait(wakeup_paper, agent->mutex);
    pthread_cond_signal(agent->smoke);
    smoke_count[PAPER]++;
  }
}

void* tobacco_smoker(void *a) {
  VERBOSE_PRINT("Tobacco smoker running\n");
  struct Agent *agent = (struct Agent *) a;
  pthread_mutex_lock(agent->mutex);
  while(1) {
    pthread_cond_wait(wakeup_tobacco, agent->mutex);
    pthread_cond_signal(agent->smoke);
    smoke_count[PAPER]++;
  }
}

int main (int argc, char** argv) {
  struct Agent*  a = createAgent();
  int ret; // used to validate init/creation of pthread stuff
  flag = 0;
  
  VERBOSE_PRINT("Main started\n");

  // Init wakeup cond vars
  ret = pthread_cond_init(wakeup_match, NULL);
  if(!ret) {
    perror("match wakeup condition variable creation failed\n");
    exit(1);
  }
  ret = pthread_cond_init(wakeup_paper, NULL);
  if(!ret) {
    perror("paper wakeup condition variable creation failed\n");
    exit(1);
  }
  ret = pthread_cond_init(wakeup_tobacco, NULL);
  if(!ret) {
    perror("tobacco wakeup condition variable creation failed\n");
    exit(1);
  }

  VERBOSE_PRINT("Wakeups initialized\n");
  
  pthread_t agent_pt;
  pthread_t match_smoker_pt, paper_smoker_pt, tobacco_smoker_pt; 
  pthread_t match_listener_pt, paper_listener_pt, tobacco_listener_pt;

  // Create smoker threads
  ret = pthread_create(&match_smoker_pt, NULL, match_smoker, a);
  if(!ret) {
    perror("match smoker thread creation failed\n");
    exit(1);
  }
  ret = pthread_create(&paper_smoker_pt, NULL, paper_smoker, a);
  if(!ret) {
    perror("paper smoker thread creation failed\n");
    exit(1);
  }
  ret = pthread_create(&tobacco_smoker_pt, NULL, tobacco_smoker, a);
  if(!ret) {
    perror("tobacco smoker thread creation failed\n");
    exit(1);
  }
  VERBOSE_PRINT("Smoker threads created\n");
  // TODO ensure smokers have started the loop (i.e. are waiting)


  // Create listeners
  ret = pthread_create(&match_listener_pt, NULL, match_listener, a);
  if(!ret) {
    perror(" listener thread creation failed\n");
    exit(1);
  }  
  ret = pthread_create(&paper_listener_pt, NULL, paper_listener, a);
  if(!ret) {
    perror(" listener thread creation failed\n");
    exit(1);
  }  
  ret = pthread_create(&tobacco_listener_pt, NULL, tobacco_listener, a);
  if(!ret) {
    perror(" listener thread creation failed\n");
    exit(1);
  }
  VERBOSE_PRINT("Listener threads created\n");
  // TODO ensure listeners have started the loop (i.e. are waiting)

  // Create agent and join it
  VERBOSE_PRINT("Creating agent thread\n");
  ret = pthread_create(&agent_pt, NULL, agent, NULL);
  if(!ret) {
    perror("failed to create agent thread");
    exit(1);
  }
  pthread_join(agent_pt, NULL);

  // Teardown
  pthread_mutex_destroy(a->mutex);
  pthread_cond_destroy(a->match);  
  pthread_cond_destroy(a->tobacco);
  pthread_cond_destroy(a->paper);
  pthread_cond_destroy(a->smoke);  
  free(a);
  pthread_cond_destroy(wakeup_match);
  pthread_cond_destroy(wakeup_paper);
  pthread_cond_destroy(wakeup_tobacco);

  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);

}
