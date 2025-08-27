# High-Performance Mesh-Quality Assessment for Diamond-Lattice Metamaterials

This repository contains the code developed for my COMPE 596 final project: an end-to-end pipeline that generates a tetrahedral mesh of a diamond-lattice metamaterial and computes geometric quality metrics on every element, using serial C++, MPI, and CUDA implementations to compare performance.

## Features

1. **Point-Cloud Generation**  
   - Defines an N x N x N diamond lattice (FCC sites + two-atom basis) with OpenMP-parallelized C++ code to emit a TetGen `.node` file.

2. **Mesh Construction**  
   - Invokes TetGen on the generated `.node` file to produce `.ele` (tet connectivity) and `.face` (surface triangles).

3. **Quality Metric Computation**  
   - **Serial C++**: Loads `.node`/`.ele`, computes per-tet aspect-ratio, aggregates min/avg/max.  
   - **MPI**: Distributes tets across ranks, times local computations with `MPI_Wtime()`, then reduces global metrics and timing.  
   - **CUDA**: Launches one thread per tet to compute aspect-ratio on the GPU, times the kernel with CUDA events, then aggregates results on the host.

4. **Benchmarking Suite**  
   - Shell scripts to sweep lattice size \(N\) and number of MPI ranks, generating timing tables and speed-up plots.

## Prerequisites

- C++17 compiler with OpenMP support (e.g., `g++`)  
- MPI library & wrapper (e.g., `mpicxx`)  
- CUDA Toolkit (`nvcc`)  
- TetGen installed for mesh generation  
- `make` or `cmake`
