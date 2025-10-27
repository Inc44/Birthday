// csc /o+ Birthday.cs && mono ./Birthday.exe
using System;
using System.Diagnostics;
using System.Threading;
public static class Birthday
{
	const int DAYS_IN_YEAR = 365;
	const int NUM_THREADS = 768;
	const int PEOPLE = 24;
	const int TOTAL_SIMULATIONS = 1_000_000;
	const uint MULTIPLIER = 1664525;
	const uint INCREMENT = 1013904223;
	static void Simulate(int simulations, int threadId, int[] successCount)
	{
		int simulationsPerThread = simulations / NUM_THREADS;
		long seed = Stopwatch.GetTimestamp();
		uint state = (uint)(seed ^ threadId);
		int localSuccessCount = 0;
		int[] birthdays = new int[DAYS_IN_YEAR];
		for (int sim = 0; sim < simulationsPerThread; sim++)
		{
			for (int i = 0; i < DAYS_IN_YEAR; i++)
				birthdays[i] = 0;
			for (int i = 0; i < PEOPLE; i++)
			{
				state = state * MULTIPLIER + INCREMENT;
				int birthday = (int)(state % DAYS_IN_YEAR);
				birthdays[birthday]++;
			}
			int exactlyTwoCount = 0;
			for (int i = 0; i < DAYS_IN_YEAR; i++)
			{
				if (birthdays[i] == 2)
					exactlyTwoCount++;
			}
			if (exactlyTwoCount == 1)
				localSuccessCount++;
		}
		successCount[threadId] = localSuccessCount;
	}
	public static void Main()
	{
		var startTime = Stopwatch.StartNew();
		int[] successCount = new int[NUM_THREADS];
		Thread[] threads = new Thread[NUM_THREADS];
		for (int t = 0; t < NUM_THREADS; t++)
		{
			int threadId = t;
			threads[t] = new Thread(() => Simulate(TOTAL_SIMULATIONS, threadId, successCount));
			threads[t].Start();
		}
		for (int t = 0; t < NUM_THREADS; t++)
			threads[t].Join();
		long totalSuccessCount = 0;
		for (int t = 0; t < NUM_THREADS; t++)
			totalSuccessCount += successCount[t];
		double probability = (double)totalSuccessCount / TOTAL_SIMULATIONS;
		Console.WriteLine("Probability: {0:F9}", probability);
		var endTime = startTime.Elapsed;
		var elapsedTime = endTime.TotalSeconds;
		Console.WriteLine("Execution Time: {0:F3} s", elapsedTime);
	}
}