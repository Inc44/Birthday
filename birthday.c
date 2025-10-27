// gcc -o birthday birthday.c -Ofast && ./birthday
// g++ -o birthday birthday.c -Ofast && ./birthday
// zig cc -o birthday birthday.c -Ofast && ./birthday
// zig c++ -o birthday birthday.c -Ofast && ./birthday
#define DAYS_IN_YEAR 365
#define NUM_THREADS 768
#define PEOPLE 24
#define TOTAL_SIMULATIONS 1000000
#define MULTIPLIER 1664525
#define INCREMENT 1013904223
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef struct {
	int simulations;
	int threadId;
	int* successCount;
} ThreadData;
void* simulate(void* arg) {
	ThreadData* data = (ThreadData*)arg;
	int simulationsPerThread = data->simulations / NUM_THREADS;
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	unsigned long long seed = time.tv_sec * 1e9 + time.tv_nsec;
	unsigned int state = seed ^ data->threadId;
	int localSuccessCount = 0;
	for (int sim = 0; sim < simulationsPerThread; sim++) {
		int birthdays[DAYS_IN_YEAR] = {0};
		for (int i = 0; i < PEOPLE; i++) {
			state = state * MULTIPLIER + INCREMENT;
			int birthday = state % DAYS_IN_YEAR;
			birthdays[birthday]++;
		}
		int exactlyTwoCount = 0;
		for (int i = 0; i < DAYS_IN_YEAR; i++) {
			if (birthdays[i] == 2) {
				exactlyTwoCount++;
			}
		}
		if (exactlyTwoCount == 1) {
			localSuccessCount++;
		}
	}
	data->successCount[data->threadId] = localSuccessCount;
	pthread_exit(NULL);
}
int main() {
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	int totalSimulations = TOTAL_SIMULATIONS;
	int* successCount = (int*)malloc(NUM_THREADS * sizeof(int));
	pthread_t threads[NUM_THREADS];
	ThreadData threadData[NUM_THREADS];
	for (int t = 0; t < NUM_THREADS; t++) {
		threadData[t].simulations = totalSimulations;
		threadData[t].threadId = t;
		threadData[t].successCount = successCount;
		pthread_create(&threads[t], NULL, simulate, (void*)&threadData[t]);
	}
	for (int t = 0; t < NUM_THREADS; t++) {
		pthread_join(threads[t], NULL);
	}
	int totalSuccessCount = 0;
	for (int t = 0; t < NUM_THREADS; t++) {
		totalSuccessCount += successCount[t];
	}
	double probability = (double)totalSuccessCount / totalSimulations;
	printf("Probability: %.9f\n", probability);
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
						  1e-9 * (end_time.tv_nsec - start_time.tv_nsec);
	printf("Execution Time: %.3f s\n", elapsed_time);
	return 0;
}