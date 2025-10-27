-- lua birthday.lua
local lanes = require("lanes")
local socket = require("socket")
local DAYS_IN_YEAR = 365
local NUM_THREADS = 768
local PEOPLE = 24
local TOTAL_SIMULATIONS = 1000000
local MULTIPLIER = 1664525
local INCREMENT = 1013904223
local function simulate(simulations, threadId)
	local simulationsPerThread = simulations / NUM_THREADS
	local seed = os.time() * 1000000000 + (os.clock() * 1000000000) % 1000000000
	local state = (seed ~ threadId) & 0xFFFFFFFF
	local birthdays = {}
	local localSuccessCount = 0
	for _ = 1, simulationsPerThread do
		for i = 1, DAYS_IN_YEAR do
			birthdays[i] = 0
		end
		for _ = 1, PEOPLE do
			state = (state * MULTIPLIER + INCREMENT) & 0xFFFFFFFF
			local birthday = state % DAYS_IN_YEAR + 1
			birthdays[birthday] = birthdays[birthday] + 1
		end
		local exactlyTwoCount = 0
		for i = 1, DAYS_IN_YEAR do
			if birthdays[i] == 2 then
				exactlyTwoCount = exactlyTwoCount + 1
			end
		end
		if exactlyTwoCount == 1 then
			localSuccessCount = localSuccessCount + 1
		end
	end
	return localSuccessCount
end
local function main()
	local start_time = socket.gettime()
	local thread = lanes.gen("*", simulate)
	local successCount = {}
	for t = 0, NUM_THREADS - 1 do
		successCount[t + 1] = thread(TOTAL_SIMULATIONS, t)
	end
	local totalSuccessCount = 0
	for t = 1, NUM_THREADS do
		totalSuccessCount = totalSuccessCount + successCount[t][1]
	end
	local probability = totalSuccessCount / TOTAL_SIMULATIONS
	io.write(string.format("Probability: %.9f\n", probability))
	local end_time = socket.gettime()
	local elapsed_time = end_time - start_time
	io.write(string.format("Execution Time: %.3f s\n", elapsed_time))
end
main()
