#!/bin/bash
# bash birthday.sh
DAYS_IN_YEAR=365
NUM_THREADS=768
PEOPLE=24
TOTAL_SIMULATIONS=1000000
MULTIPLIER=1664525
INCREMENT=1013904223
simulate() {
	simulations=$1
	thread_id=$2
	simulations_per_thread=$((simulations / NUM_THREADS))
	seed=$(date +%s%N)
	state=$(((seed ^ thread_id) & 0xFFFFFFFF))
	local_success_count=0
	for ((sim = 0; sim < simulations_per_thread; sim++)); do
		birthdays=()
		for ((i = 0; i < PEOPLE; i++)); do
			state=$(((state * MULTIPLIER + INCREMENT) & 0xFFFFFFFF))
			birthday=$((state % DAYS_IN_YEAR))
			((birthdays[birthday]++))
		done
		exactly_two_count=0
		for day in "${birthdays[@]}"; do
			if ((day == 2)); then
				((exactly_two_count++))
			fi
		done
		# 5 times slower alternative
		# for ((i = 0; i < DAYS_IN_YEAR; i++)); do
		# 	if ((birthdays[i] == 2)); then
		# 		((exactly_two_count++))
		# 	fi
		# done
		if ((exactly_two_count == 1)); then
			((local_success_count++))
		fi
	done
	echo "$local_success_count"
}
main() {
	start_time=$(date +%s.%N)
	threads=$(mktemp -d)
	for ((t = 0; t < NUM_THREADS; t++)); do
		simulate "$TOTAL_SIMULATIONS" "$t" >"$threads/$t" &
	done
	wait
	total_success_count=0
	for ((t = 0; t < NUM_THREADS; t++)); do
		if [[ -f "$threads/$t" ]]; then
			read -r success_count <"$threads/$t"
			((total_success_count += success_count))
		fi
	done
	rm -rf "$threads"
	probability=$(awk "BEGIN {printf \"%.9f\", $total_success_count / $TOTAL_SIMULATIONS}")
	echo "Probability: $probability"
	end_time=$(date +%s.%N)
	elapsed_time=$(awk "BEGIN {printf \"%.3f\", $end_time - $start_time}")
	echo "Execution Time: $elapsed_time s"
}
main
