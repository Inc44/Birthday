// gcc -o birthday birthday.c -Ofast && ./birthday
// g++ -o birthday birthday.c -Ofast && ./birthday
// zig cc -o birthday birthday.c -Ofast && ./birthday
// zig c++ -o birthday birthday.c -Ofast && ./birthday
// clang -o birthday birthday.c -O3 -ffast-math && ./birthday
// clang++ -o birthday birthday.c -O3 -ffast-math && ./birthday
#define DAYS_IN_YEAR 365
#define NUM_THREADS 768
#define PEOPLE 24
#define TOTAL_SIMULATIONS 1000000
#define MULTIPLIER 1664525
#define INCREMENT 1013904223
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef struct {
	uint32_t simulations;
	uint16_t threadId;
	uint32_t* successCount;
} ThreadData;
void* simulate(void* arg) {
	ThreadData* data = (ThreadData*)arg;
	uint32_t simulationsPerThread = data->simulations / NUM_THREADS;
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	uint64_t seed = time.tv_sec * 1e9 + time.tv_nsec;
	uint32_t state = seed ^ data->threadId;
	uint32_t localSuccessCount = 0;
	for (uint32_t sim = 0; sim < simulationsPerThread; sim++) {
		uint8_t birthdays[DAYS_IN_YEAR] = {0};
		for (uint8_t i = 0; i < PEOPLE; i++) {
			state = state * MULTIPLIER + INCREMENT;
			uint16_t birthday = state % DAYS_IN_YEAR;
			birthdays[birthday]++;
		}
		uint8_t exactlyTwoCount = 0;
		for (uint16_t i = 0; i < DAYS_IN_YEAR; i++) {
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
	uint32_t* successCount = (uint32_t*)malloc(NUM_THREADS * sizeof(uint32_t));
	pthread_t threads[NUM_THREADS];
	ThreadData threadData[NUM_THREADS];
	for (uint16_t t = 0; t < NUM_THREADS; t++) {
		threadData[t].simulations = TOTAL_SIMULATIONS;
		threadData[t].threadId = t;
		threadData[t].successCount = successCount;
		pthread_create(&threads[t], NULL, simulate, (void*)&threadData[t]);
	}
	for (uint16_t t = 0; t < NUM_THREADS; t++) {
		pthread_join(threads[t], NULL);
	}
	uint32_t totalSuccessCount = 0;
	for (uint16_t t = 0; t < NUM_THREADS; t++) {
		totalSuccessCount += successCount[t];
	}
	double probability = (double)totalSuccessCount / TOTAL_SIMULATIONS;
	printf("Probability: %.9f\n", probability);
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
						  1e-9 * (end_time.tv_nsec - start_time.tv_nsec);
	printf("Execution Time: %.3f s\n", elapsed_time);
	free(successCount);
	return 0;
}