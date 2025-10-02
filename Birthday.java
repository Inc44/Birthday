// javac Birthday.java && java Birthday
import java.util.concurrent.CountDownLatch;
import java.util.Random;
public class Birthday {
	static int NUM_THREADS = 768;
	static int TOTAL_SIMULATIONS = 1_000_000;
	static int SIMULATIONS_PER_THREAD = TOTAL_SIMULATIONS / NUM_THREADS;
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
			long seed = (System.currentTimeMillis() / 1000L) ^ threadId;
			Random rand = new Random();
			rand.setSeed(seed);
			int successCount = 0;
			for (int sim = 0; sim < SIMULATIONS_PER_THREAD; sim++) {
				int[] birthdays = new int[365];
				for (int i = 0; i < 24; i++) {
					int birthday = rand.nextInt(365);
					birthdays[birthday]++;
				}
				int exactlyTwoCount = 0;
				for (int i = 0; i < 365; i++) {
					if (birthdays[i] == 2) {
						exactlyTwoCount++;
					}
				}
				if (exactlyTwoCount == 1) {
					successCount++;
				}
			}
			dataSuccessCount[threadId] = successCount;
			latch.countDown();
		}
	}
	public static void main(String[] args) {
		long start = System.nanoTime();
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
		double elapsed = (System.nanoTime() - start) / 1e9;
		System.out.printf("Execution Time: %.3f s%n", elapsed);
	}
}