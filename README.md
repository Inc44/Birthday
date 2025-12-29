# Programming Languages Benchmark

![Stars](https://img.shields.io/github/stars/Inc44/Birthday?style=social)
![Forks](https://img.shields.io/github/forks/Inc44/Birthday?style=social)
![Watchers](https://img.shields.io/github/watchers/Inc44/Birthday?style=social)
![Repo Size](https://img.shields.io/github/repo-size/Inc44/Birthday)
![Language Count](https://img.shields.io/github/languages/count/Inc44/Birthday)
![Top Language](https://img.shields.io/github/languages/top/Inc44/Birthday)
[![Issues](https://img.shields.io/github/issues/Inc44/Birthday)](https://github.com/Inc44/Birthday/issues?q=is%3Aopen+is%3Aissue)
![Last Commit](https://img.shields.io/github/last-commit/Inc44/Birthday?color=red)
[![Release](https://img.shields.io/github/release/Inc44/Birthday.svg)](https://github.com/Inc44/Birthday/releases)
[![Sponsor](https://img.shields.io/static/v1?label=Sponsor&message=%E2%9D%A4&logo=GitHub&color=%23fe8e86)](https://github.com/sponsors/Inc44)

This repository conducts a simulation of classes consisting of 24 people to search for matches where exactly two people share the same birthday.

## 📈 Performance Comparison

In March 2024, the project started with Python, C, CUDA, Rust, and Go implementations and was tested on AMD Ryzen 5 1600 (AF), NVIDIA GTX 1660 Super, and Google Colab's NVIDIA Tesla T4 with the following results:

### Simulation with 1,000,000 Iterations

![Simulation with 1,000,000 Iterations](1_000_000_202403.png)

**First Conclusion:** Python demonstrates slow performance.

### Simulation with 1,000,000,000 Iterations

![Simulation with 1,000,000,000 Iterations](1_000_000_000_202403.png)

**Second Conclusion:** CUDA exhibits super-fast performance.

In October 2025, Java, OCaml, Zig, OpenCL, and JavaScript implementations were added and tested on Intel Core Ultra 7 155H and NVIDIA RTX 4060 Ti 16 GB with the following results:

### Simulation with 1,000,000 Iterations

![Simulation with 1,000,000 Iterations](1_000_000_202510.png)

### Simulation with 1,000,000,000 Iterations

![Simulation with 1,000,000,000 Iterations](1_000_000_000_202510.png)

**Conclusion:** It seems necessary to rewrite existing implementations to make them use the same random function, probably LCG for simplicity, as Xoshiro, despite its potential speed, as well as PCG, despite its statistical goodness, may be too slow to implement in interpreted languages.

## 📖 Usage Examples

This benchmark was conducted on various hardware configurations and programming languages:

### Google Colab Tesla T4 16GB

#### CUDA
- `birthday.ipynb`

### Ryzen 5 1600 + 1660 Super

#### C Compiled as C using GNU Compiler
- Compilation: `gcc -o birthday birthday.c -Ofast`
- Execution: `./birthday`

#### C Compiled as C++ using GNU Compiler
- Compilation: `g++ -o birthday birthday.c -Ofast`
- Execution: `./birthday`

#### C Compiled as C using Zig Compiler
- Compilation: `zig cc -o birthday birthday.c -Ofast`
- Execution: `./birthday`

#### C Compiled as C++ using Zig Compiler
- Compilation: `zig c++ -o birthday birthday.c -Ofast`
- Execution: `./birthday`

#### CUDA
- Compilation: `nvcc -o birthday birthday.cu -O3 -arch=sm_75`
- Execution: `./birthday`

#### Go
- Compilation: `go build -o birthday birthday.go`
- Execution: `./birthday`

#### Python
- Optimized Execution: `python -OO birthday.py`

#### Python using NumPy
- Optimized Execution: `python -OO birthday_numpy.py`

#### Rust
- Optimized Run: `cargo run --release -q`

### Core Ultra 7 155H + 4060 Ti 16 GB

#### C Compiled as C using Clang Compiler
- Compilation: `clang -o birthday birthday.c -O3 -ffast-math`
- Execution: `./birthday`

#### C Compiled as C++ using Clang Compiler
- Compilation: `clang++ -o birthday birthday.c -O3 -ffast-math`
- Execution: `./birthday`

#### OpenCL Compiled as C using GNU Compiler
- Compilation: `gcc -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast`
- Execution: `./birthday_opencl`

#### OpenCL Compiled as C++ using GNU Compiler
- Compilation: `g++ -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast`
- Execution: `./birthday_opencl`

#### OpenCL Compiled as C using Zig Compiler
- Compilation: `zig cc -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast`
- Execution: `./birthday_opencl`

#### OpenCL Compiled as C++ using Zig Compiler
- Compilation: `zig c++ -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast`
- Execution: `./birthday_opencl`

#### OpenCL Compiled as C using Clang Compiler
- Compilation: `clang -o birthday_opencl birthday_opencl.c -lOpenCL -O3 -ffast-math`
- Execution: `./birthday_opencl`

#### OpenCL Compiled as C++ using Clang Compiler
- Compilation: `clang++ -o birthday_opencl birthday_opencl.c -lOpenCL -O3 -ffast-math`
- Execution: `./birthday_opencl`

#### Vulkan Compiled as C using GNU Compiler
- Compilation: `gcc -o birthday_vulkan birthday_vulkan.c -lvulkan -Ofast`
- Execution: `./birthday_vulkan`

#### Vulkan Compiled as C++ using GNU Compiler
- Compilation: `g++ -o birthday_vulkan birthday_vulkan.c -lvulkan -Ofast`
- Execution: `./birthday_vulkan`

#### Vulkan Compiled as C using Zig Compiler
- Compilation: `zig cc -o birthday_vulkan birthday_vulkan.c -lvulkan -Ofast`
- Execution: `./birthday_vulkan`

#### Vulkan Compiled as C++ using Zig Compiler
- Compilation: `zig c++ -o birthday_vulkan birthday_vulkan.c -lvulkan -Ofast`
- Execution: `./birthday_vulkan`

#### Vulkan Compiled as C using Clang Compiler
- Compilation: `clang -o birthday_vulkan birthday_vulkan.c -lvulkan -O3 -ffast-math`
- Execution: `./birthday_vulkan`

#### Vulkan Compiled as C++ using Clang Compiler
- Compilation: `clang++ -o birthday_vulkan birthday_vulkan.c -lvulkan -O3 -ffast-math`
- Execution: `./birthday_vulkan`

#### C++ Compiled using GNU Compiler
- Compilation: `g++ -o birthday birthday.cpp -Ofast`
- Execution: `./birthday`

#### C++ Compiled using Zig Compiler
- Compilation: `zig c++ -o birthday birthday.cpp -Ofast`
- Execution: `./birthday`

#### C++ Compiled using Clang Compiler
- Compilation: `clang++ -o birthday birthday.cpp -O3 -ffast-math`
- Execution: `./birthday`

#### C#
- Compilation: `csc /o+ Birthday.cs`
- Execution: `mono ./Birthday.exe`

#### Java
- Compilation: `javac Birthday.java`
- Execution: `java Birthday.java`

#### JavaScript
- Execution: `node birthday.js`

#### Lua
- Execution: `lua birthday.lua`

#### OCaml
- Compilation: `ocamlopt -I +unix -thread unix.cmxa threads.cmxa -o birthday birthday.ml`
- Execution: `./birthday`

#### Zig
- Compilation: `zig build-exe birthday.zig -O ReleaseFast`
- Execution: `./birthday`

#### Assembly
- Execution: `nasm -f elf64 birthday.asm && gcc -no-pie -o birthday birthday.o -lpthread -Ofast && ./birthday`

#### Assembly with Optimizations
- Execution: `nasm -f elf64 birthday_ofast.asm && gcc -no-pie -o birthday_ofast birthday_ofast.o -lpthread -Ofast && ./birthday_ofast`

#### MySQL
- Execution: `mysql -u root -p < birthday.sql`

#### PHP
- Execution: `php birthday.php`

#### Bash/Shell
- Execution: `bash birthday.sh`

#### TypeScript
- Execution: `tsc birthday.ts && node birthday.js`

#### OpenMP Compiled as C using GNU Compiler
- Compilation: `gcc -o birthday_openmp birthday_openmp.c -fopenmp -Ofast`
- Execution: `./birthday_openmp`

#### OpenMP Compiled as C++ using GNU Compiler
- Compilation: `g++ -o birthday_openmp birthday_openmp.c -fopenmp -Ofast`
- Execution: `./birthday_openmp`

#### OpenMP Compiled as C using Zig Compiler
- Compilation: `zig cc -o birthday_openmp birthday_openmp.c -fopenmp -Ofast`
- Execution: `./birthday_openmp`

#### OpenMP Compiled as C++ using Zig Compiler
- Compilation: `zig c++ -o birthday_openmp birthday_openmp.c -fopenmp -Ofast`
- Execution: `./birthday_openmp`

#### OpenMP Compiled as C using Clang Compiler
- Compilation: `clang -o birthday_openmp birthday_openmp.c -fopenmp -O3 -ffast-math`
- Execution: `./birthday_openmp`

#### OpenMP Compiled as C++ using Clang Compiler
- Compilation: `clang++ -o birthday_openmp birthday_openmp.c -fopenmp -O3 -ffast-math`
- Execution: `./birthday_openmp`

#### Fortran
- Compilation: `gfortran -o birthday birthday.f -fopenmp -Ofast`
- Execution: `./birthday`

#### COBOL
- Compilation: `cobc -x -O3 -o birthday birthday.cob`
- Execution: `./birthday`

#### Kotlin
- Compilation: `kotlinc Birthday.kt -include-runtime -d Birthday.jar`
- Execution: `java -jar Birthday.jar`

#### Haskell
- Compilation: `ghc -O2 -threaded -o birthday Birthday.hs`
- Execution: `./birthday +RTS -N`

### C/C++/OpenCL/OpenMP/Vulkan Checked

- Compilation: `gcc|g++|clang|clang++ -Wall -Wextra -g -fsanitize=address -fsanitize=undefined ... -o birthday birthday...`
- Execution: `./birthday`

## 🚧 TODO

- [ ] **Create separate charts for each tested hardware**
- [ ] **Highlight languages with color**
- [ ] **Run tests on Windows**

## 🤝 Contribution

Contributions, suggestions, and new ideas are heartily welcomed. If you're considering significant modifications, please initiate an issue for discussion before submitting a pull request.

## 📜 License

[![MIT](https://img.shields.io/badge/License-MIT-lightgrey.svg)](https://opensource.org/licenses/MIT)

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## 💖 Support

[![BuyMeACoffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-ffdd00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black)](https://buymeacoffee.com/xamituchido)
[![Ko-Fi](https://img.shields.io/badge/Ko--fi-F16061?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/inc44)
[![Patreon](https://img.shields.io/badge/Patreon-F96854?style=for-the-badge&logo=patreon&logoColor=white)](https://www.patreon.com/Inc44)