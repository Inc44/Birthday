# python -OO birthday.py
from typing import Final
import threading
import time

DAYS_IN_YEAR: Final[int] = 365
NUM_THREADS: Final[int] = 768
PEOPLE: Final[int] = 24
TOTAL_SIMULATIONS: Final[int] = 1_000_000
MULTIPLIER: Final[int] = 1664525
INCREMENT: Final[int] = 1013904223


class ThreadData:
	def __init__(self, simulations, thread_id, success_count):
		self.simulations = simulations
		self.thread_id = thread_id
		self.success_count = success_count
		self.local_success_count = 0


def simulate(data):
	simulations_per_thread = data.simulations // NUM_THREADS
	seed = int(time.time_ns())
	state = (seed ^ data.thread_id) & 0xFFFFFFFF
	for _ in range(simulations_per_thread):
		birthdays = [0] * DAYS_IN_YEAR
		for _ in range(PEOPLE):
			state = (state * MULTIPLIER + INCREMENT) & 0xFFFFFFFF
			birthday = state % DAYS_IN_YEAR
			birthdays[birthday] += 1
		exactly_two_count = sum(1 for day in birthdays if day == 2)
		if exactly_two_count == 1:
			data.local_success_count += 1
	data.success_count[data.thread_id] = data.local_success_count


def main():
	start_time = time.time()
	success_count = [0] * NUM_THREADS
	threads = []
	for t in range(NUM_THREADS):
		data = ThreadData(TOTAL_SIMULATIONS, t, success_count)
		thread = threading.Thread(target=simulate, args=(data,))
		threads.append(thread)
		thread.start()
	for thread in threads:
		thread.join()
	total_success_count = sum(success_count)
	probability = total_success_count / TOTAL_SIMULATIONS
	print(f"Probability: {probability:.9f}")
	end_time = time.time()
	elapsed_time = end_time - start_time
	print(f"Execution Time: {elapsed_time:.3f} s")


if __name__ == "__main__":
	main()
