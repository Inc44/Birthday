# python -OO birthday_numpy.py
import threading
import time
import numpy as np

DAYS_IN_YEAR = 365
NUM_THREADS = 768
PEOPLE = 24
TOTAL_SIMULATIONS = 1_000_000
MULTIPLIER = 1664525
INCREMENT = 1013904223


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
	birthdays = np.zeros(DAYS_IN_YEAR, dtype=np.uint8)
	for _ in range(simulations_per_thread):
		birthdays.fill(0)
		for _ in range(PEOPLE):
			state = (state * MULTIPLIER + INCREMENT) & 0xFFFFFFFF
			birthday = state % DAYS_IN_YEAR
			birthdays[birthday] += 1
		exactly_two_count = np.count_nonzero(birthdays == 2)
		if exactly_two_count == 1:
			data.local_success_count += 1
	with data.success_count_lock:
		data.success_count[0] += data.local_success_count


def main():
	start_time = time.time()
	success_count = [0]
	success_count_lock = threading.Lock()
	threads = []
	for t in range(NUM_THREADS):
		data = ThreadData(TOTAL_SIMULATIONS, t, success_count)
		data.success_count_lock = success_count_lock
		thread = threading.Thread(target=simulate, args=(data,))
		threads.append(thread)
		thread.start()
	for thread in threads:
		thread.join()
	probability = success_count[0] / TOTAL_SIMULATIONS
	print(f"Probability: {probability:.9f}")
	elapsed_time = time.time() - start_time
	print(f"Execution Time: {elapsed_time:.3f} s")


if __name__ == "__main__":
	main()
