// node birthday.js
"use strict";
const
{
	Worker,
	isMainThread,
	parentPort,
	workerData
} = require("worker_threads");
const os = require("os");
const DAYS_IN_YEAR = 365;
const PEOPLE = 24;
const TOTAL_SIMULATIONS = 1_000_000;
const NUM_THREADS = 768;
if (!isMainThread)
{
	const tid = workerData.threadId;
	const simulationsPerThread = workerData.simulations;
	let successCount = 0;
	const seed = Number(process.hrtime.bigint());
	let state = (seed ^ tid) >>> 0;
	const birthdays = new Uint8Array(DAYS_IN_YEAR);
	for (let s = 0; s < simulationsPerThread; s++)
	{
		for (let i = 0; i < DAYS_IN_YEAR; i++) birthdays[i] = 0;
		for (let i = 0; i < PEOPLE; i++)
		{
			state = (state * 1664525 + 1013904223) >>> 0;
			const birthday = state % DAYS_IN_YEAR;
			birthdays[birthday]++;
		}
		let exactlyTwoCount = 0;
		for (let i = 0; i < DAYS_IN_YEAR; i++)
		{
			if (birthdays[i] === 2) exactlyTwoCount++;
		}
		if (exactlyTwoCount === 1) successCount++;
	}
	parentPort.postMessage(successCount);
}
else
{
	const start_time = process.hrtime.bigint();
	const threads = Math.min(NUM_THREADS, os.cpus()
		.length);
	const simulationsPerThread = Math.floor(TOTAL_SIMULATIONS / threads);
	const unassignedSimulations = TOTAL_SIMULATIONS % threads;
	let completedThreads = 0;
	let totalSuccessCount = 0;
	for (let t = 0; t < threads; t++)
	{
		const simulations = simulationsPerThread + (t < unassignedSimulations ? 1 : 0);
		const thread = new Worker(__filename,
		{
			workerData:
			{
				simulations: simulations,
				threadId: t
			}
		});
		thread.on("message", (count) =>
		{
			totalSuccessCount += count;
			completedThreads++;
			thread.terminate();
			if (completedThreads === threads)
			{
				const probability = totalSuccessCount / TOTAL_SIMULATIONS;
				console.log(`Probability: ${probability.toFixed(9)}`);
				const elapsed_time = Number(process.hrtime.bigint() - start_time) / 1e9;
				console.log(`Execution Time: ${elapsed_time.toFixed(3)} s`);
			}
		});
	}
}