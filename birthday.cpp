// g++ -o birthday birthday.cpp -Ofast && ./birthday
// zig c++ -o birthday birthday.cpp -Ofast && ./birthday
// clang++ -o birthday birthday.cpp -O3 -ffast-math && ./birthday
#define DAYS_IN_YEAR 365
#define NUM_THREADS 768
#define PEOPLE 24
#define TOTAL_SIMULATIONS 1000000
#define MULTIPLIER 1664525
#define INCREMENT 1013904223
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>
using namespace std;
void simulate(uint32_t simulations, uint16_t threadId, uint32_t* successCount) {
	uint32_t simulationsPerThread = simulations / NUM_THREADS;
	uint64_t seed = (uint64_t)chrono::duration_cast<chrono::nanoseconds>(
						chrono::steady_clock::now().time_since_epoch())
						.count();
	uint32_t state = seed ^ threadId;
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
			if (birthdays[i] == 2)
				exactlyTwoCount++;
		}
		if (exactlyTwoCount == 1)
			localSuccessCount++;
	}
	successCount[threadId] = localSuccessCount;
}
int main() {
	chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
	vector<uint32_t> successCount(NUM_THREADS);
	vector<thread> threads;
	for (uint16_t t = 0; t < NUM_THREADS; t++) {
		threads.emplace_back(simulate, TOTAL_SIMULATIONS, t,
							 successCount.data());
	}
	for (thread& t : threads)
		t.join();
	uint32_t totalSuccessCount = 0;
	for (uint16_t t = 0; t < NUM_THREADS; t++)
		totalSuccessCount += successCount[t];
	double probability = (double)totalSuccessCount / TOTAL_SIMULATIONS;
	printf("Probability: %.9f\n", probability);
	chrono::steady_clock::time_point end_time = chrono::steady_clock::now();
	double elapsed_time =
		chrono::duration<double>(end_time - start_time).count();
	printf("Execution Time: %.3f s\n", elapsed_time);
	return 0;
}