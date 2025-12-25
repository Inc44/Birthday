<?php
// php birthday.php
const DAYS_IN_YEAR = 365;
const NUM_THREADS = 768;
const PEOPLE = 24;
const TOTAL_SIMULATIONS = 1_000_000;
const MULTIPLIER = 1664525;
const INCREMENT = 1013904223;
function simulate(int $simulations, int $thread_id, array &$success_count): void
{
	$simulations_per_thread = intdiv($simulations, NUM_THREADS);
	$seed = hrtime(true);
	$state = ($seed ^ $thread_id) & 0xffffffff;
	$local_success_count = 0;
	for ($sim = 0; $sim < $simulations_per_thread; $sim++) {
		$birthdays = array_fill(0, DAYS_IN_YEAR, 0);
		for ($i = 0; $i < PEOPLE; $i++) {
			$state = ($state * MULTIPLIER + INCREMENT) & 0xffffffff;
			$birthday = $state % DAYS_IN_YEAR;
			$birthdays[$birthday]++;
		}
		$exactly_two_count = 0;
		for ($i = 0; $i < DAYS_IN_YEAR; $i++) {
			if ($birthdays[$i] === 2) {
				$exactly_two_count++;
			}
		}
		if ($exactly_two_count === 1) {
			$local_success_count++;
		}
	}
	$success_count[$thread_id] = $local_success_count;
}
function main(): void
{
	$start_time = hrtime(true);
	$sockets = stream_socket_pair(
		STREAM_PF_UNIX,
		STREAM_SOCK_STREAM,
		STREAM_IPPROTO_IP
	);
	for ($t = 0; $t < NUM_THREADS; $t++) {
		if (pcntl_fork() == 0) {
			fclose($sockets[1]);
			$success_count = [];
			simulate(TOTAL_SIMULATIONS, $t, $success_count);
			fwrite($sockets[0], pack("N", $success_count[$t]));
			exit(0);
		}
	}
	fclose($sockets[0]);
	$total_success_count = 0;
	for ($t = 0; $t < NUM_THREADS; $t++) {
		$total_success_count += unpack("N", fread($sockets[1], 4))[1];
	}
	for ($t = 0; $t < NUM_THREADS; $t++) {
		pcntl_wait($status);
	}
	$probability = $total_success_count / TOTAL_SIMULATIONS;
	printf("Probability: %.9f\n", $probability);
	$end_time = hrtime(true);
	$elapsed_time = ($end_time - $start_time) / 1e9;
	printf("Execution Time: %.3f s\n", $elapsed_time);
}
main();
?>
