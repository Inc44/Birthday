// g++ -o birthday birthday.cpp -Ofast && ./birthday
// zig c++ -o birthday birthday.cpp -Ofast && ./birthday
// clang++ -o birthday birthday.cpp -O3 -ffast-math && ./birthday
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>
using namespace std;
enum {
	DAYS_IN_YEAR = 365,
	NUM_THREADS = 768,
	PEOPLE = 24,
	TOTAL_SIMULATIONS = 1000000,
	MULTIPLIER = 1664525,
	INCREMENT = 1013904223
};
void simulate(uint32_t simulations,
			  uint16_t thread_id,
			  uint32_t* success_count) {
	uint32_t simulations_per_thread = simulations / NUM_THREADS;
	uint64_t seed = (uint64_t)chrono::duration_cast<chrono::nanoseconds>(
						chrono::steady_clock::now().time_since_epoch())
						.count();
	uint32_t state = seed ^ thread_id;
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
			if (birthdays[i] == 2)
				exactly_two_count++;
		}
		if (exactly_two_count == 1)
			local_success_count++;
	}
	success_count[thread_id] = local_success_count;
}
int main() {
	chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
	vector<uint32_t> success_count(NUM_THREADS);
	vector<thread> threads;
	for (uint16_t t = 0; t < NUM_THREADS; t++) {
		threads.emplace_back(simulate, TOTAL_SIMULATIONS, t,
							 success_count.data());
	}
	for (thread& t : threads)
		t.join();
	uint32_t total_success_count = 0;
	for (uint16_t t = 0; t < NUM_THREADS; t++)
		total_success_count += success_count[t];
	double probability = (double)total_success_count / TOTAL_SIMULATIONS;
	printf("Probability: %.9f\n", probability);
	chrono::steady_clock::time_point end_time = chrono::steady_clock::now();
	double elapsed_time =
		chrono::duration<double>(end_time - start_time).count();
	printf("Execution Time: %.3f s\n", elapsed_time);
	return 0;
}