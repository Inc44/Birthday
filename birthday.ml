(* ocamlopt -I +unix -thread unix.cmxa threads.cmxa -o birthday birthday.ml && ./birthday *)
open Thread
open Unix
let days_in_year = 365
let num_threads = 768
let people = 24
let total_simulations = 1_000_000
let multiplier = 1664525
let increment = 1013904223
let simulations_per_thread = total_simulations / num_threads
let simulate thread_id data_success_count =
	let seed = Int64.of_float (Unix.gettimeofday () *. 1_000_000_000.) in
	let state = ref (Int64.to_int (Int64.logxor seed (Int64.of_int thread_id))) in
	let success_count = ref 0 in
	for _ = 1 to simulations_per_thread do
		let birthdays = Array.make days_in_year 0 in
		for _ = 1 to people do
			state := (!state * multiplier) + increment;
			let birthday = !state land 0x7FFFFFFF mod days_in_year in
			birthdays.(birthday) <- birthdays.(birthday) + 1
		done;
		let exactly_two_count = ref 0 in
		for i = 0 to days_in_year - 1 do
			if birthdays.(i) = 2 then incr exactly_two_count
		done;
		if !exactly_two_count = 1 then incr success_count
	done;
	data_success_count.(thread_id) <- !success_count
let () =
	let start = gettimeofday () in
	let data_success_count = Array.make num_threads 0 in
	let threads = Array.init num_threads (fun t -> Thread.create (fun id -> simulate id data_success_count) t) in
	Array.iter Thread.join threads;
	let total_success_count = Array.fold_left ( + ) 0 data_success_count in
	let probability = float_of_int total_success_count /. float_of_int total_simulations in
	Printf.printf "Probability: %.9f\n" probability;
	let elapsed = gettimeofday () -. start in
	Printf.printf "Execution Time: %.3f s\n" elapsed