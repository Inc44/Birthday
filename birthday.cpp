// g++ -o birthday birthday.cpp -Ofast && ./birthday
// zig c++ -o birthday birthday.cpp -Ofast && ./birthday
// clang++ -o birthday birthday.cpp -O3 -ffast-math && ./birthday
#define DAYS_IN_YEAR 365
#define NUM_THREADS 768
#define PEOPLE 24
#define TOTAL_SIMULATIONS 1000000000
#define MULTIPLIER 1664525
#define INCREMENT 1013904223
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>
using namespace std;
void simulate(int simulations, int threadId, int* successCount) {
	int simulationsPerThread = simulations / NUM_THREADS;
	unsigned long long seed =
		chrono::duration_cast<chrono::nanoseconds>(
			chrono::steady_clock::now().time_since_epoch())
			.count();
	unsigned int state = seed ^ threadId;
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
			if (birthdays[i] == 2)
				exactlyTwoCount++;
		}
		if (exactlyTwoCount == 1)
			localSuccessCount++;
	}
	successCount[threadId] = localSuccessCount;
}
int main() {
	chrono::steady_clock::time_point start = chrono::steady_clock::now();
	int totalSimulations = TOTAL_SIMULATIONS;
	vector<int> successCount(NUM_THREADS);
	vector<thread> threads;
	for (int t = 0; t < NUM_THREADS; t++) {
		threads.emplace_back(simulate, totalSimulations, t,
							 successCount.data());
	}
	for (thread& t : threads)
		t.join();
	int totalSuccessCount = 0;
	for (int t = 0; t < NUM_THREADS; t++)
		totalSuccessCount += successCount[t];
	double probability = (double)totalSuccessCount / totalSimulations;
	printf("Probability: %.9f\n", probability);
	chrono::steady_clock::time_point end = chrono::steady_clock::now();
	double elapsed = chrono::duration<double>(end - start).count();
	printf("Execution Time: %.3f s\n", elapsed);
	return 0;
}