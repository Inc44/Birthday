// go build -o birthday birthday.go && ./birthday
package main

import (
	"fmt"
	"sync"
	"time"
)

const (
	days_in_year         = 365
	numThreads           = 768
	people               = 24
	totalSimulations     = 1_000_000
	simulationsPerThread = totalSimulations / numThreads
	multiplier           = uint32(1664525)
	increment            = uint32(1013904223)
)

func simulate(threadId int, simulations int, successCount *[]int, wg *sync.WaitGroup) {
	defer wg.Done()
	seed := time.Now().UnixNano()
	state := uint32(seed ^ int64(threadId))
	localSuccessCount := 0
	for sim := 0; sim < simulationsPerThread; sim++ {
		var birthdays [days_in_year]int
		for i := 0; i < people; i++ {
			state = state*multiplier + increment
			birthday := int(state % days_in_year)
			birthdays[birthday]++
		}
		exactlyTwoCount := 0
		for _, count := range birthdays {
			if count == 2 {
				exactlyTwoCount++
			}
		}
		if exactlyTwoCount == 1 {
			localSuccessCount++
		}
	}
	(*successCount)[threadId] = localSuccessCount
}
func main() {
	start := time.Now()
	successCount := make([]int, numThreads)
	var wg sync.WaitGroup
	for t := 0; t < numThreads; t++ {
		wg.Add(1)
		go simulate(t, totalSimulations, &successCount, &wg)
	}
	wg.Wait()
	totalSuccessCount := 0
	for _, count := range successCount {
		totalSuccessCount += count
	}
	probability := float64(totalSuccessCount) / float64(totalSimulations)
	fmt.Printf("Probability: %.9f\n", probability)
	elapsed := time.Since(start)
	fmt.Printf("Execution Time: %.3f s\n", elapsed.Seconds())
}
