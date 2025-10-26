// cargo run --release
use std::sync::mpsc;
use std::thread;
use std::time::Instant;
const DAYS_IN_YEAR: usize = 365;
const NUM_THREADS: usize = 768;
const PEOPLE: usize = 24;
const TOTAL_SIMULATIONS: usize = 1_000_000;
fn simulate(simulations: usize, sender: mpsc::Sender<usize>, thread_id: usize) {
    let seed = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_secs();
    let mut state = seed ^ thread_id as u64;
    let simulations_per_thread = simulations / NUM_THREADS;
    let mut local_success_count = 0;
    for _ in 0..simulations_per_thread {
        let mut birthdays = [0; DAYS_IN_YEAR];
        for _ in 0..PEOPLE {
            state = state.wrapping_mul(1664525).wrapping_add(1013904223);
            let birthday = (state as usize) % DAYS_IN_YEAR;
            birthdays[birthday] += 1;
        }
        let exactly_two_count = birthdays.iter().filter(|&&x| x == 2).count();
        if exactly_two_count == 1 {
            local_success_count += 1;
        }
    }
    sender.send(local_success_count).unwrap();
}
fn main() {
    let start = Instant::now();
    let (sender, receiver) = mpsc::channel();
    for thread_id in 0..NUM_THREADS {
        let sender = sender.clone();
        thread::spawn(move || {
            simulate(TOTAL_SIMULATIONS, sender, thread_id);
        });
    }
    let mut total_success_count = 0;
    for _ in 0..NUM_THREADS {
        total_success_count += receiver.recv().unwrap();
    }
    let probability = total_success_count as f64 / TOTAL_SIMULATIONS as f64;
    println!("Probability: {:.9}", probability);
    let elapsed = start.elapsed();
    println!("Execution Time: {:.3} s", elapsed.as_secs_f64());
}
