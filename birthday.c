// gcc -o birthday birthday.c -Ofast && ./birthday
// g++ -o birthday birthday.c -Ofast && ./birthday
// zig cc -o birthday birthday.c -Ofast && ./birthday
// zig c++ -o birthday birthday.c -Ofast && ./birthday
#include <pthread.h>

#include <stdio.h>

#include <stdlib.h>

#include <time.h>

#define DAYS_IN_YEAR 365
#define NUM_THREADS 768
#define PEOPLE 24
#define TOTAL_SIMULATIONS 1000000
typedef struct {
  int simulations;
  int threadId;
  int * successCount;
}
ThreadData;
void * simulate(void * arg) {
  ThreadData * data = (ThreadData * ) arg;
  int simulationsPerThread = data -> simulations / NUM_THREADS;
  unsigned int seed = time(NULL) ^ data -> threadId;
  int successCount = 0;
  for (int sim = 0; sim < simulationsPerThread; sim++) {
    int birthdays[365] = {
      0
    };
    for (int i = 0; i < 24; i++) {
      int birthday = rand_r( & seed) % 365;
      birthdays[birthday]++;
    }
    int exactlyTwoCount = 0;
    for (int i = 0; i < 365; i++) {
      if (birthdays[i] == 2) {
        exactlyTwoCount++;
      }
    }
    if (exactlyTwoCount == 1) {
      successCount++;
    }
  }
  data -> successCount[data -> threadId] = successCount;
  pthread_exit(NULL);
}
int main() {
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, & start);
  int totalSimulations = TOTAL_SIMULATIONS;
  int * successCount = (int * ) malloc(NUM_THREADS * sizeof(int));
  pthread_t threads[NUM_THREADS];
  ThreadData threadData[NUM_THREADS];
  for (int t = 0; t < NUM_THREADS; t++) {
    threadData[t].simulations = totalSimulations;
    threadData[t].threadId = t;
    threadData[t].successCount = successCount;
    pthread_create( & threads[t], NULL, simulate, (void * ) & threadData[t]);
  }
  for (int t = 0; t < NUM_THREADS; t++) {
    pthread_join(threads[t], NULL);
  }
  int totalSuccessCount = 0;
  for (int t = 0; t < NUM_THREADS; t++) {
    totalSuccessCount += successCount[t];
  }
  double probability = (double) totalSuccessCount / totalSimulations;
  printf("Probability: %.9f\n", probability);
  clock_gettime(CLOCK_MONOTONIC, & end);
  double elapsed = (end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec);
  printf("Execution Time: %.3f s\n", elapsed);
  return 0;
}