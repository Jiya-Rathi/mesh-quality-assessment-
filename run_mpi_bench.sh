#!/usr/bin/env bash
#
# run_mpi_bench.sh
# Benchmark compute_quality_mpi over different MPI process counts,
# storing full terminal output (including elapsed times) in a .txt log.

# Path to your MPI-enabled binary
EXEC=./compute_quality_mpi

# Output log file (plain text)
OUTFILE=mpi_benchmark.txt

# List of MPI process counts to try
PROCS=(1 2 4 8 16)

# Initialize log
echo "MPI Benchmark Results" > "$OUTFILE"
echo "====================" >> "$OUTFILE"
\# Loop over process counts
for p in "${PROCS[@]}"; do
  echo "\n--- Processes: $p ---" >> "$OUTFILE"
  echo "Running with $p processes..." | tee -a "$OUTFILE"
  # Run and append full program output
  mpirun -np $p $EXEC 2>&1 | tee -a "$OUTFILE"
done

echo "\nBenchmark complete. Results written to $OUTFILE" | tee -a "$OUTFILE"
