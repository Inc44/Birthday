% swilgt -O -q -t halt birthday.lgt
:- object(birthday).
	:- threaded.
	:- public(main/0).
	days_in_year(365).
	num_threads(768).
	people(24).
	total_simulations(1000000).
	multiplier(1664525).
	increment(1013904223).
	uint32_t(4294967296).
	next_state(State, NextState) :-
		multiplier(Multiplier),
		increment(Increment),
		uint32_t(UInt32T),
		NextState is (State * Multiplier + Increment) mod UInt32T.
	loop_people(0, State, State, _) :- !.
	loop_people(I, State, FinalState, Birthdays) :-
		days_in_year(DaysInYear),
		next_state(State, NextState),
		Birthday is (NextState mod DaysInYear) + 1,
		arg(Birthday, Birthdays, Count),
		NextCount is Count + 1,
		{nb_setarg(Birthday, Birthdays, NextCount)},
		NextI is I - 1,
		loop_people(NextI, NextState, FinalState, Birthdays).
	loop_count(0, _, Count, Count) :- !.
	loop_count(I, Birthdays, LocalSuccessCount, Count) :-
		arg(I, Birthdays, ExactlyTwoCount),
		(ExactlyTwoCount =:= 2 -> NextLocalSuccessCount is LocalSuccessCount + 1 ; NextLocalSuccessCount = LocalSuccessCount),
		NextI is I - 1,
		loop_count(NextI, Birthdays, NextLocalSuccessCount, Count).
	loop_sim(State, NextState, LocalSuccessCount, Birthdays, EmptyBirthdays) :-
		days_in_year(DaysInYear),
		people(I),
		{duplicate_term(EmptyBirthdays, Birthdays)},
		loop_people(I, State, NextState, Birthdays),
		loop_count(DaysInYear, Birthdays, 0, ExactlyTwoCount),
		(ExactlyTwoCount =:= 1 -> LocalSuccessCount = 1 ; LocalSuccessCount = 0).
	simulate(Simulations, Seed, SuccessCount) :-
		days_in_year(DaysInYear),
		functor(EmptyBirthdays, birthdays, DaysInYear),
		{forall(between(1, DaysInYear, I), nb_setarg(I, EmptyBirthdays, 0))},
		simulate_loop(Simulations, Seed, 0, SuccessCount, EmptyBirthdays).
	simulate_loop(0, _, SuccessCount, SuccessCount, _) :- !.
	simulate_loop(Sim, State, LocalSuccessCount, SuccessCount, EmptyBirthdays) :-
		loop_sim(State, NextState, Count, _, EmptyBirthdays),
		NextLocalSuccessCount is LocalSuccessCount + Count,
		NextSim is Sim - 1,
		simulate_loop(NextSim, NextState, NextLocalSuccessCount, SuccessCount, EmptyBirthdays).
	worker(Simulations, ThreadId, SuccessCount) :-
		uint32_t(UInt32T),
		{get_time(SeedTime)},
		Seed is (floor(SeedTime * 1e9) xor ThreadId) mod UInt32T,
		simulate(Simulations, Seed, SuccessCount).
	pthread_call(Simulations, ThreadId, Thread) :-
		threaded_call(worker(Simulations, ThreadId, _), Thread).
	pthread_exit(Thread, SuccessCount) :-
		threaded_exit(worker(_, _, SuccessCount), Thread).
	pthread_create(_, [], []).
	pthread_create(Simulations, [ThreadId|ThreadIds], [Thread|Threads]) :-
		pthread_call(Simulations, ThreadId, Thread),
		pthread_create(Simulations, ThreadIds, Threads).
	pthread_join([], []).
	pthread_join([Thread|Threads], [SuccessCount|SuccessCounts]) :-
		pthread_exit(Thread, SuccessCount),
		pthread_join(Threads, SuccessCounts).
	main :-
		{get_time(StartTime)},
		num_threads(NumThreads),
		total_simulations(TotalSimulations),
		SimulationsPerThread is TotalSimulations // NumThreads,
		MaxThreadId is NumThreads - 1,
		{numlist(0, MaxThreadId, ThreadIds)},
		pthread_create(SimulationsPerThread, ThreadIds, Threads),
		pthread_join(Threads, SuccessCount),
		{sum_list(SuccessCount, TotalSuccessCount)},
		Probability is TotalSuccessCount / TotalSimulations,
		{format('Probability: ~9f~n', [Probability])},
		{get_time(EndTime)},
		ElapsedTime is EndTime - StartTime,
		{format('Execution Time: ~3f s~n', [ElapsedTime])}.
:- end_object.
:- initialization(birthday::main).