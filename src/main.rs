// cargo run --release -q
use std::sync::mpsc;
use std::thread;
use std::time::Instant;
const DAYS_IN_YEAR: usize = 365;
const NUM_THREADS: u32 = 768;
const PEOPLE: u8 = 24;
const TOTAL_SIMULATIONS: u32 = 1_000_000;
const MULTIPLIER: u32 = 1664525;
const INCREMENT: u32 = 1013904223;
fn simulate(simulations: u32, sender: mpsc::Sender<u32>, thread_id: u64) {
    let seed = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_nanos();
    let mut state: u32 = (seed as u64 ^ thread_id) as u32;
    let simulations_per_thread: u32 = simulations / NUM_THREADS;
    let mut local_success_count: u32 = 0;
    for _ in 0..simulations_per_thread {
        let mut birthdays = [0u8; DAYS_IN_YEAR];
        for _ in 0..PEOPLE {
            state = state.wrapping_mul(MULTIPLIER).wrapping_add(INCREMENT);
            let birthday = (state as usize) % DAYS_IN_YEAR;
            birthdays[birthday] = birthdays[birthday].wrapping_add(1);
        }
        let exactly_two_count = birthdays.iter().filter(|&&x| x == 2).count();
        if exactly_two_count == 1 {
            local_success_count += 1;
        }
    }
    sender.send(local_success_count).unwrap();
}
fn main() {
    let start_time = Instant::now();
    let (sender, receiver) = mpsc::channel::<u32>();
    for thread_id in 0..NUM_THREADS {
        let sender = sender.clone();
        thread::spawn(move || {
            simulate(TOTAL_SIMULATIONS, sender, thread_id as u64);
        });
    }
    let mut total_success_count: u32 = 0;
    for _ in 0..NUM_THREADS {
        total_success_count += receiver.recv().unwrap();
    }
    let probability = total_success_count as f32 / TOTAL_SIMULATIONS as f32;
    println!("Probability: {:.9}", probability);
    let elapsed_time = start_time.elapsed();
    println!("Execution Time: {:.3} s", elapsed_time.as_secs_f64());
}
