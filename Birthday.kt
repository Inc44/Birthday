// kotlinc Birthday.kt -include-runtime -d Birthday.jar && java -jar Birthday.jar
import kotlin.concurrent.thread

const val DAYS_IN_YEAR: Int = 365
const val NUM_THREADS: Int = 768
const val PEOPLE: Int = 24
const val TOTAL_SIMULATIONS: Int = 1_000_000
const val SIMULATIONS_PER_THREAD: Int = TOTAL_SIMULATIONS / NUM_THREADS
const val MULTIPLIER: Int = 1664525
const val INCREMENT: Int = 1013904223

fun simulate(threadId: Int, successCount: IntArray): Unit {
  val seed: Long = System.nanoTime()
  var state: Int = (seed xor threadId.toLong()).toInt()
  var localSuccessCount: Int = 0
  for (sim: Int in 0 until SIMULATIONS_PER_THREAD) {
    val birthdays: IntArray = IntArray(DAYS_IN_YEAR)
    for (i: Int in 0 until PEOPLE) {
      state = state * MULTIPLIER + INCREMENT
      val birthday: Int = (state.toUInt() % DAYS_IN_YEAR.toUInt()).toInt()
      birthdays[birthday]++
    }
    var exactlyTwoCount: Int = 0
    for (i: Int in 0 until DAYS_IN_YEAR) {
      if (birthdays[i] == 2) {
        exactlyTwoCount++
      }
    }
    if (exactlyTwoCount == 1) {
      localSuccessCount++
    }
  }
  successCount[threadId] = localSuccessCount
}

fun main(): Unit {
  val startTime: Long = System.nanoTime()
  val successCount: IntArray = IntArray(NUM_THREADS)
  val threads: Array<Thread?> = arrayOfNulls<Thread>(NUM_THREADS)
  for (t: Int in 0 until NUM_THREADS) threads[t] = thread { simulate(t, successCount) }
  for (t: Thread? in threads) t?.join()
  var totalSuccessCount: Int = 0
  for (t: Int in successCount) totalSuccessCount += t
  val probability: Double = totalSuccessCount.toDouble() / TOTAL_SIMULATIONS
  System.out.printf("Probability: %.9f\n", probability)
  val endTime: Long = System.nanoTime()
  val elapsedTime: Double = (endTime - startTime) / 1e9
  System.out.printf("Execution Time: %.3f s\n", elapsedTime)
}
