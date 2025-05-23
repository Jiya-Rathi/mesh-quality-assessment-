#!/usr/bin/env bash
#
# benchmark_sizes.sh
# Automate mesh generation with quality flag and timing for N=1000,1500,2000,2500,
# running MPI for multiple process counts, and record full output to a text log.

# Binaries & tools
GEN=./gen_diamond
TET=/mnt/beegfs/dgx/jrathi/COMPE596/project/software/tetgen/tetgen
SERIAL=./compute_quality
MPIBIN=./compute_quality_mpi
CUDABIN=./compute_quality_cuda

# Quality flag for TetGen (radius-edge ratio bound)
QBOUND=1.4

# Mesh sizes to try
SIZES=(1000 1500 2000 2500)
# MPI process counts to benchmark
PROCS=(1 2 4 8 16)

# Output log file
OUTFILE=benchmark_sizes.txt

echo "Benchmark run started: $(date)" > "$OUTFILE"
echo "Quality bound: $QBOUND" | tee -a "$OUTFILE"
echo "Mesh sizes: ${SIZES[*]}" | tee -a "$OUTFILE"
echo "MPI procs: ${PROCS[*]}" | tee -a "$OUTFILE"

for N in "${SIZES[@]}"; do
  echo -e "\n=== N = $N ===" | tee -a "$OUTFILE"

  echo "1) Generating point cloud (diamond N=$N)" | tee -a "$OUTFILE"
  $GEN $N > diamond.node 2>&1 | tee -a "$OUTFILE"

  echo "2) Tetrahedralizing with TetGen -q$QBOUND" | tee -a "$OUTFILE"
  $TET -q$QBOUND diamond.node 2>&1 | tee -a "$OUTFILE"
  mv diamond.1.node diamond_${N}.node
  mv diamond.1.ele  diamond_${N}.ele

  echo "3) Serial execution" | tee -a "$OUTFILE"
  $SERIAL 2>&1 | tee -a "$OUTFILE"

  echo "4) MPI execution" | tee -a "$OUTFILE"
  for p in "${PROCS[@]}"; do
    echo "-- MPI ranks: $p --" | tee -a "$OUTFILE"
    mpirun -np $p $MPIBIN 2>&1 | tee -a "$OUTFILE"
  done

  echo "5) CUDA execution" | tee -a "$OUTFILE"
  $CUDABIN 2>&1 | tee -a "$OUTFILE"

done

echo -e "\nBenchmark run completed: $(date)" | tee -a "$OUTFILE"
