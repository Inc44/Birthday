// tsc birthday.ts && node birthday.js
const {
	Worker: ThreadWorker,
	isMainThread,
	parentPort,
	workerData,
} = require("worker_threads");
const os = require("os");
const DAYS_IN_YEAR: number = 365;
const PEOPLE: number = 24;
const TOTAL_SIMULATIONS: number = 1_000_000;
const NUM_THREADS: number = 768;
const MULTIPLIER: number = 1664525;
const INCREMENT: number = 1013904223;
if (!isMainThread) {
	const threadId: number = workerData.threadId;
	const simulationsPerThread: number = workerData.simulations;
	let localSuccessCount: number = 0;
	const seed: number = Number(process.hrtime.bigint());
	let state: number = (seed ^ threadId) >>> 0;
	const birthdays = new Uint8Array(DAYS_IN_YEAR);
	for (let sim = 0; sim < simulationsPerThread; sim++) {
		birthdays.fill(0);
		for (let i = 0; i < PEOPLE; i++) {
			state = (state * MULTIPLIER + INCREMENT) >>> 0;
			const birthday: number = state % DAYS_IN_YEAR;
			birthdays[birthday]++;
		}
		let exactlyTwoCount: number = 0;
		for (let i = 0; i < DAYS_IN_YEAR; i++) {
			if (birthdays[i] === 2) exactlyTwoCount++;
		}
		if (exactlyTwoCount === 1) localSuccessCount++;
	}
	parentPort.postMessage(localSuccessCount);
} else {
	const startTime: bigint = process.hrtime.bigint();
	const threads: number = Math.min(NUM_THREADS, os.cpus().length);
	const simulationsPerThread: number = Math.floor(
		TOTAL_SIMULATIONS / threads,
	);
	const unassignedSimulations: number = TOTAL_SIMULATIONS % threads;
	let completedThreads: number = 0;
	let totalSuccessCount: number = 0;
	for (let t = 0; t < threads; t++) {
		const simulations: number =
			simulationsPerThread + (t < unassignedSimulations ? 1 : 0);
		const thread = new ThreadWorker(__filename, {
			workerData: {
				simulations: simulations,
				threadId: t,
			},
		});
		thread.on("message", (count: number) => {
			totalSuccessCount += count;
			completedThreads++;
			thread.terminate();
			if (completedThreads === threads) {
				const probability: number =
					totalSuccessCount / TOTAL_SIMULATIONS;
				console.log(`Probability: ${probability.toFixed(9)}`);
				const endTime: bigint = process.hrtime.bigint();
				const elapsedTime: number = Number(endTime - startTime) / 1e9;
				console.log(`Execution Time: ${elapsedTime.toFixed(3)} s`);
			}
		});
	}
}
