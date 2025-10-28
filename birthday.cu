// nvcc -o birthday birthday.cu -O3 -arch=sm_75 && ./birthday for GTX 1660 SUPER
// nvcc -o birthday birthday.cu -O3 -arch=sm_89 && ./birthday for RTX 4060 TI
#define BLOCK_SIZE 32
#define DAYS_IN_YEAR 365
#define NUM_BLOCKS (NUM_THREADS / BLOCK_SIZE)
#define NUM_THREADS 768	 // for GTX 1660 SUPER
// #define NUM_THREADS 2176 for RTX 4060 TI
#define PEOPLE 24
#define TOTAL_SIMULATIONS 1000000
#define MULTIPLIER 1664525
#define INCREMENT 1013904223
#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
__global__ void simulate(uint32_t simulations,
						 uint32_t* deviceSuccessCount,
						 uint64_t seed) {
	uint16_t threadId = blockIdx.x * blockDim.x + threadIdx.x;
	uint32_t simulationsPerThread = simulations / NUM_THREADS;
	uint32_t localSuccessCount = 0;
	uint32_t state = seed ^ threadId;
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
	deviceSuccessCount[threadId] = localSuccessCount;
}
int main() {
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	uint64_t seed = time.tv_sec * 1e9 + time.tv_nsec;
	uint32_t *deviceSuccessCount, *hostSuccessCount;
	cudaMalloc((void**)&deviceSuccessCount, NUM_THREADS * sizeof(uint32_t));
	hostSuccessCount = (uint32_t*)malloc(NUM_THREADS * sizeof(uint32_t));
	simulate<<<NUM_BLOCKS, BLOCK_SIZE>>>(TOTAL_SIMULATIONS, deviceSuccessCount,
										 seed);
	cudaMemcpy(hostSuccessCount, deviceSuccessCount, NUM_THREADS * sizeof(uint32_t),
			   cudaMemcpyDeviceToHost);
	uint32_t totalSuccessCount = 0;
	for (uint16_t t = 0; t < NUM_THREADS; t++) {
		totalSuccessCount += hostSuccessCount[t];
	}
	double probability = (double)totalSuccessCount / TOTAL_SIMULATIONS;
	printf("Probability: %.9f\n", probability);
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	double elapsed_time =
		(end_time.tv_sec - start_time.tv_sec) + 1e-9 * (end_time.tv_nsec - start_time.tv_nsec);
	printf("Execution Time: %.3f s\n", elapsed_time);
	return 0;
}