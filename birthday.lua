-- lua birthday.lua
local lanes = require("lanes")
local socket = require("socket")
local DAYS_IN_YEAR = 365
local NUM_THREADS = 768
local PEOPLE = 24
local TOTAL_SIMULATIONS = 1000000
local MULTIPLIER = 1664525
local INCREMENT = 1013904223
local function simulate(simulations, thread_id)
	local simulations_per_thread = simulations / NUM_THREADS
	local seed = os.time() * 1000000000 + (os.clock() * 1000000000) % 1000000000
	local state = (seed ~ thread_id) & 0xFFFFFFFF
	local birthdays = {}
	local local_success_count = 0
	for _ = 1, simulations_per_thread do
		for i = 1, DAYS_IN_YEAR do
			birthdays[i] = 0
		end
		for _ = 1, PEOPLE do
			state = (state * MULTIPLIER + INCREMENT) & 0xFFFFFFFF
			local birthday = state % DAYS_IN_YEAR + 1
			birthdays[birthday] = birthdays[birthday] + 1
		end
		local exactly_two_count = 0
		for i = 1, DAYS_IN_YEAR do
			if birthdays[i] == 2 then
				exactly_two_count = exactly_two_count + 1
			end
		end
		if exactly_two_count == 1 then
			local_success_count = local_success_count + 1
		end
	end
	return local_success_count
end
local function main()
	local start_time = socket.gettime()
	local thread = lanes.gen("*", simulate)
	local success_count = {}
	for t = 0, NUM_THREADS - 1 do
		success_count[t + 1] = thread(TOTAL_SIMULATIONS, t)
	end
	local total_success_count = 0
	for t = 1, NUM_THREADS do
		total_success_count = total_success_count + success_count[t][1]
	end
	local probability = total_success_count / TOTAL_SIMULATIONS
	io.write(string.format("Probability: %.9f\n", probability))
	local end_time = socket.gettime()
	local elapsed_time = end_time - start_time
	io.write(string.format("Execution Time: %.3f s\n", elapsed_time))
end
main()
