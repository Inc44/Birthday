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
__global__ void simulate(int simulations,
						 int* d_successCount,
						 unsigned int seed) {
	int tid = blockIdx.x * blockDim.x + threadIdx.x;
	int simulationsPerThread = simulations / NUM_THREADS;
	int successCount = 0;
	unsigned int state = seed ^ tid;
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
			successCount++;
		}
	}
	d_successCount[tid] = successCount;
}
int main() {
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	unsigned long long seed = time.tv_sec * 1e9 + time.tv_nsec;
	int totalSimulations = TOTAL_SIMULATIONS;
	int *d_successCount, *h_successCount;
	cudaMalloc((void**)&d_successCount, NUM_THREADS * sizeof(int));
	h_successCount = (int*)malloc(NUM_THREADS * sizeof(int));
	simulate<<<NUM_BLOCKS, BLOCK_SIZE>>>(totalSimulations, d_successCount,
										 seed);
	cudaMemcpy(h_successCount, d_successCount, NUM_THREADS * sizeof(int),
			   cudaMemcpyDeviceToHost);
	int totalSuccessCount = 0;
	for (int t = 0; t < NUM_THREADS; t++) {
		totalSuccessCount += h_successCount[t];
	}
	double probability = (double)totalSuccessCount / totalSimulations;
	printf("Probability: %.9f\n", probability);
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed =
		(end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec);
	printf("Execution Time: %.3f s\n", elapsed);
	return 0;
}