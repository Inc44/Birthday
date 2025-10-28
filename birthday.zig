// zig build-exe birthday.zig -O ReleaseFast && ./birthday
const std = @import("std");
const DAYS_IN_YEAR: u16 = 365;
const NUM_THREADS: u16 = 768;
const PEOPLE: u8 = 24;
const TOTAL_SIMULATIONS: u32 = 1_000_000;
const MULTIPLIER: u32 = 1664525;
const INCREMENT: u32 = 1013904223;
const ThreadData = struct {
    simulations: u32,
    thread_id: u16,
    success_count: []u32,
};
fn simulate(data: ThreadData) void {
    const simulations_per_thread = data.simulations / NUM_THREADS;
    const seed: u64 = @intCast(std.time.nanoTimestamp());
    var state: u32 = @truncate(seed ^ data.thread_id);
    var local_success_count: u32 = 0;
    for (0..simulations_per_thread) |_| {
        var birthdays = [_]u8{0} ** DAYS_IN_YEAR;
        for (0..PEOPLE) |_| {
            state = state *% MULTIPLIER +% INCREMENT;
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
            local_success_count += 1;
        }
    }
    data.success_count[data.thread_id] = local_success_count;
}
pub fn main() !void {
    const start_time: u64 = @intCast(std.time.nanoTimestamp());
    var success_count: [NUM_THREADS]u32 = undefined;
    var threads: [NUM_THREADS]std.Thread = undefined;
    var data: [NUM_THREADS]ThreadData = undefined;
    for (0..NUM_THREADS) |t| {
        data[t] = .{
            .simulations = TOTAL_SIMULATIONS,
            .thread_id = @truncate(t),
            .success_count = &success_count,
        };
        threads[t] = try std.Thread.spawn(.{}, simulate, .{data[t]});
    }
    for (threads) |thread| {
        thread.join();
    }
    var total_success_count: u32 = 0;
    for (success_count) |count| {
        total_success_count += count;
    }
    const probability = @as(f32, @floatFromInt(total_success_count)) / TOTAL_SIMULATIONS;
    std.debug.print("Probability: {d:.9}\n", .{probability});
    const end_time: u64 = @intCast(std.time.nanoTimestamp());
    const elapsed_time: f64 = @as(f64, @floatFromInt(end_time - start_time)) / std.time.ns_per_s;
    std.debug.print("Execution Time: {d:.3} s\n", .{elapsed_time});
}
