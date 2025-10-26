// zig build-exe birthday.zig -O ReleaseFast && ./birthday
const std = @import("std");
const DAYS_IN_YEAR = 365;
const NUM_THREADS = 768;
const PEOPLE = 24;
const TOTAL_SIMULATIONS = 1_000_000;
const ThreadData = struct {
    simulations: u32,
    thread_id: u64,
    success_count: []u32,
};
fn simulate(data: ThreadData) void {
    const simulations_per_thread = data.simulations / NUM_THREADS;
    const seed: u64 = @bitCast(std.time.timestamp());
    var state: u32 = @truncate(seed ^ data.thread_id);
    var success_count: u32 = 0;
    for (0..simulations_per_thread) |_| {
        var birthdays = [_]u8{0} ** DAYS_IN_YEAR;
        for (0..PEOPLE) |_| {
            state = state *% 1664525 +% 1013904223;
            const birthday: u32 = state % DAYS_IN_YEAR;
            birthdays[birthday] += 1;
        }
        var exactly_two_count: u8 = 0;
        for (birthdays) |day| {
            if (day == 2) {
                exactly_two_count += 1;
            }
        }
        if (exactly_two_count == 1) {
            success_count += 1;
        }
    }
    data.success_count[data.thread_id] = success_count;
}
pub fn main() !void {
    var start_time = try std.time.Timer.start();
    var success_count: [NUM_THREADS]u32 = undefined;
    var threads: [NUM_THREADS]std.Thread = undefined;
    var data: [NUM_THREADS]ThreadData = undefined;
    for (0..NUM_THREADS) |t| {
        data[t] = .{
            .simulations = TOTAL_SIMULATIONS,
            .thread_id = t,
            .success_count = &success_count,
        };
        threads[t] = try std.Thread.spawn(.{}, simulate, .{data[t]});
    }
    for (threads) |thread| {
        thread.join();
    }
    var total_success_count: u64 = 0;
    for (success_count) |count| {
        total_success_count += count;
    }
    const probability = @as(f64, @floatFromInt(total_success_count)) / TOTAL_SIMULATIONS;
    std.debug.print("Probability: {d:.9}\n", .{probability});
    const elapsed_time = @as(f64, @floatFromInt(start_time.read())) / std.time.ns_per_s;
    std.debug.print("Execution Time: {d:.3} s\n", .{elapsed_time});
}
