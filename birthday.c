// gcc -o birthday birthday.c -Ofast && ./birthday
// g++ -o birthday birthday.c -Ofast && ./birthday
// zig cc -o birthday birthday.c -Ofast && ./birthday
// zig c++ -o birthday birthday.c -Ofast && ./birthday
// clang -o birthday birthday.c -O3 -ffast-math && ./birthday
// clang++ -o birthday birthday.c -O3 -ffast-math && ./birthday
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
enum {
	DAYS_IN_YEAR = 365,
	NUM_THREADS = 768,
	PEOPLE = 24,
	TOTAL_SIMULATIONS = 1000000,
	MULTIPLIER = 1664525,
	INCREMENT = 1013904223
};
typedef struct {
	uint32_t simulations;
	uint16_t threadId;
	uint32_t* successCount;
} ThreadData;
void* simulate(void* arg) {
	ThreadData* data = (ThreadData*)arg;
	uint32_t simulations_per_thread = data->simulations / NUM_THREADS;
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	uint64_t seed = time.tv_sec * 1e9 + time.tv_nsec;
	uint32_t state = seed ^ data->threadId;
	uint32_t local_success_count = 0;
	for (uint32_t sim = 0; sim < simulations_per_thread; sim++) {
		uint8_t birthdays[DAYS_IN_YEAR] = {0};
		for (uint8_t i = 0; i < PEOPLE; i++) {
			state = state * MULTIPLIER + INCREMENT;
			uint16_t birthday = state % DAYS_IN_YEAR;
			birthdays[birthday]++;
		}
		uint8_t exactly_two_count = 0;
		for (uint16_t i = 0; i < DAYS_IN_YEAR; i++) {
			if (birthdays[i] == 2) {
				exactly_two_count++;
			}
		}
		if (exactly_two_count == 1) {
			local_success_count++;
		}
	}
	data->successCount[data->threadId] = local_success_count;
	pthread_exit(NULL);
}
int main() {
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	uint32_t* success_count = (uint32_t*)malloc(NUM_THREADS * sizeof(uint32_t));
	pthread_t threads[NUM_THREADS];
	ThreadData thread_data[NUM_THREADS];
	for (uint16_t t = 0; t < NUM_THREADS; t++) {
		thread_data[t].simulations = TOTAL_SIMULATIONS;
		thread_data[t].threadId = t;
		thread_data[t].successCount = success_count;
		pthread_create(&threads[t], NULL, simulate, (void*)&thread_data[t]);
	}
	for (uint16_t t = 0; t < NUM_THREADS; t++) {
		pthread_join(threads[t], NULL);
	}
	uint32_t total_success_count = 0;
	for (uint16_t t = 0; t < NUM_THREADS; t++) {
		total_success_count += success_count[t];
	}
	double probability = (double)total_success_count / TOTAL_SIMULATIONS;
	printf("Probability: %.9f\n", probability);
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
						  1e-9 * (end_time.tv_nsec - start_time.tv_nsec);
	printf("Execution Time: %.3f s\n", elapsed_time);
	free(success_count);
	return 0;
}