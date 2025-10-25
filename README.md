# Programming Languages Benchmark

This repository conducts a simulation of classes consisting of 24 people to search for matches where exactly two people share the same birthday.

## Performance Comparison

### Simulation with 1,000,000 Iterations
![Simulation with 1,000,000 Iterations](1_000_000.png)

**First Conclusion:** Python demonstrates slow performance.

### Simulation with 1,000,000,000 Iterations
![Simulation with 1,000,000,000 Iterations](1_000_000_000.png)

**Second Conclusion:** CUDA exhibits super-fast performance.

## Description

This benchmark was conducted on various hardware configurations and programming languages:

### Google Colab Tesla T4 16GB

#### CUDA
- `birthday.ipynb`

### Ryzen 5 1600 + 1660 Super

#### CUDA
- Compilation: `nvcc -o birthday birthday.cu -O3 -arch=sm_75`
- Execution: `./birthday`

#### Rust
- Optimized Run: `cargo run --release`

#### C Compiled as C++ using GNU Compiler
- Compilation: `g++ -o birthday birthday.c -Ofast`
- Execution: `./birthday`

#### C Compiled as C using GNU Compiler
- Compilation: `gcc -o birthday birthday.c -Ofast`
- Execution: `./birthday`

#### Go
- Compilation: `go build -o birthday birthday.go`
- Execution: `./birthday`

#### Python
- Optimized Execution: `python -OO birthday.py`

#### Python using NumPy
- Optimized Execution: `python -OO birthday_numpy.py`

#### C Compiled as C++ using Zig Compiler
- Compilation: `zig c++ -o birthday birthday.c -Ofast`
- Execution: `./birthday`

#### C Compiled as C using Zig Compiler
- Compilation: `zig cc -o birthday birthday.c -Ofast`
- Execution: `./birthday`

#### Java
- Compilation: `javac Birthday.java`
- Execution: `java Birthday.java`

#### OCaml
- Compilation: `ocamlopt -I +unix -thread unix.cmxa threads.cmxa -o birthday birthday.ml`
- Execution: `./birthday`

#### Zig
- Compilation: `zig build-exe birthday.zig -O ReleaseFast`
- Execution: `./birthday`

#### OpenCL
- Compilation: `cl birthday_opencl.c /I"C:\Program Files (x86)\Intel\oneAPI\compiler\latest\include" /link "C:\Program Files (x86)\Intel\oneAPI\compiler\latest\lib\OpenCL.lib"`
- Execution: `./birthday_opencl`

#### JavaScript
- Execution: `node birthday.js`

## License

MIT