// go build -o birthday birthday.go && ./birthday
package main

import (
	"fmt"
	"math/rand"
	"sync"
	"time"
)

const (
	days_in_year         = 365
	numThreads           = 768
	people               = 24
	totalSimulations     = 1_000_000
	simulationsPerThread = totalSimulations / numThreads
)

func simulate(threadId int, simulations int, successCount *[]int, wg *sync.WaitGroup) {
	defer wg.Done()
	seed := rand.New(rand.NewSource(time.Now().UnixNano() ^ int64(threadId)))
	successCountLocal := 0
	for sim := 0; sim < simulationsPerThread; sim++ {
		var birthdays [days_in_year]int
		for i := 0; i < people; i++ {
			birthday := seed.Intn(days_in_year)
			birthdays[birthday]++
		}
		exactlyTwoCount := 0
		for _, count := range birthdays {
			if count == 2 {
				exactlyTwoCount++
			}
		}
		if exactlyTwoCount == 1 {
			successCountLocal++
		}
	}
	(*successCount)[threadId] = successCountLocal
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
