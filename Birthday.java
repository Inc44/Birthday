// javac Birthday.java && java Birthday
import java.util.concurrent.CountDownLatch;
public class Birthday {
	static final int DAYS_IN_YEAR = 365;
	static final int NUM_THREADS = 768;
	static final int PEOPLE = 24;
	static final int TOTAL_SIMULATIONS = 1_000_000;
	static final int SIMULATIONS_PER_THREAD = TOTAL_SIMULATIONS / NUM_THREADS;
	static final int MULTIPLIER = 1664525;
	static final int INCREMENT = 1013904223;
	static class Worker implements Runnable {
		private int threadId;
		private int[] dataSuccessCount;
		private CountDownLatch latch;
		Worker(int threadId, int[] dataSuccessCount, CountDownLatch latch) {
			this.threadId = threadId;
			this.dataSuccessCount = dataSuccessCount;
			this.latch = latch;
		}
		@Override
		public void run() {
			long seed = System.nanoTime();
			int state = (int) (seed ^ threadId);
			int localSuccessCount = 0;
			for (int sim = 0; sim < SIMULATIONS_PER_THREAD; sim++) {
				int[] birthdays = new int[DAYS_IN_YEAR];
				for (int i = 0; i < PEOPLE; i++) {
					state = (state * MULTIPLIER + INCREMENT);
					int birthday = Integer.remainderUnsigned(state, DAYS_IN_YEAR);
					birthdays[birthday]++;
				}
				int exactlyTwoCount = 0;
				for (int i = 0; i < DAYS_IN_YEAR; i++) {
					if (birthdays[i] == 2) {
						exactlyTwoCount++;
					}
				}
				if (exactlyTwoCount == 1) {
					localSuccessCount++;
				}
			}
			dataSuccessCount[threadId] = localSuccessCount;
			latch.countDown();
		}
	}
	public static void main(String[] args) {
		long startTime = System.nanoTime();
		int[] dataSuccessCount = new int[NUM_THREADS];
		CountDownLatch latch = new CountDownLatch(NUM_THREADS);
		for (int t = 0; t < NUM_THREADS; t++) {
			new Thread(new Worker(t, dataSuccessCount, latch)).start();
		}
		try {
			latch.await();
		} catch (InterruptedException e) {
			return;
		}
		int totalSuccessCount = 0;
		for (int t : dataSuccessCount) totalSuccessCount += t;
		double probability = (double) totalSuccessCount / TOTAL_SIMULATIONS;
		System.out.printf("Probability: %.9f%n", probability);
		long endTime = System.nanoTime();
		double elapsedTime = (endTime - startTime) / 1e9;
		System.out.printf("Execution Time: %.3f s%n", elapsedTime);
	}
}