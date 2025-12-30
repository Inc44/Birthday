% swipl -O -g main -t halt birthday_ofast.pl
:- use_module(library(lists)).
:- use_module(library(thread)).
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
loop_people(0, State, State, []) :- !.
loop_people(I, State, FinalState, [Birthday|Birthdays]) :-
    days_in_year(DaysInYear),
    next_state(State, NextState),
    Birthday is (NextState mod DaysInYear) + 1,
    NextI is I - 1,
    loop_people(NextI, NextState, FinalState, Birthdays).
loop_subcount(_, [], [], 1) :- !.
loop_subcount(Birthday, [Birthday|Birthdays], UnassignedBirthdays, Count) :- !,
    loop_subcount(Birthday, Birthdays, UnassignedBirthdays, NextCount),
    Count is NextCount + 1.
loop_subcount(_, Birthdays, Birthdays, 1).
loop_count([], 0).
loop_count([Birthday|Birthdays], Count) :-
    loop_subcount(Birthday, Birthdays, UnassignedBirthdays, ExactlyTwoCount),
    (ExactlyTwoCount =:= 2 -> LocalSuccessCount = 1 ; LocalSuccessCount = 0),
    loop_count(UnassignedBirthdays, SubCount),
    Count is SubCount + LocalSuccessCount.
loop_sim(State, NextState, LocalSuccessCount, _) :-
    people(I),
    loop_people(I, State, NextState, Birthdays),
    msort(Birthdays, SortedBirthdays),
    loop_count(SortedBirthdays, ExactlyTwoCount),
    (ExactlyTwoCount =:= 1 -> LocalSuccessCount = 1 ; LocalSuccessCount = 0).
simulate_loop(0, _, SuccessCount, SuccessCount, _) :- !.
simulate_loop(Sim, State, LocalSuccessCount, SuccessCount, EmptyBirthdays) :-
    loop_sim(State, NextState, Count, EmptyBirthdays),
    NextLocalSuccessCount is LocalSuccessCount + Count,
    NextSim is Sim - 1,
    simulate_loop(NextSim, NextState, NextLocalSuccessCount, SuccessCount, EmptyBirthdays).
worker(Simulations, ThreadId) :-
    uint32_t(UInt32T),
    get_time(SeedTime),
    Seed is (floor(SeedTime * 1e9) xor ThreadId) mod UInt32T,
    simulate_loop(Simulations, Seed, 0, SuccessCount, _),
    thread_exit(SuccessCount).
pthread_create(Simulations, ThreadId, Thread) :-
    thread_create(worker(Simulations, ThreadId), Thread, []).
pthread_join(Thread, SuccessCount) :-
    thread_join(Thread, exited(SuccessCount)).
main :-
    get_time(StartTime),
    num_threads(NumThreads),
    total_simulations(TotalSimulations),
    SimulationsPerThread is TotalSimulations // NumThreads,
    MaxThreadId is NumThreads - 1,
    numlist(0, MaxThreadId, ThreadIds),
    maplist(pthread_create(SimulationsPerThread), ThreadIds, Threads),
    maplist(pthread_join, Threads, SuccessCount),
    sum_list(SuccessCount, TotalSuccessCount),
    Probability is TotalSuccessCount / TotalSimulations,
    format('Probability: ~9f~n', [Probability]),
    get_time(EndTime),
    ElapsedTime is EndTime - StartTime,
    format('Execution Time: ~3f s~n', [ElapsedTime]).
