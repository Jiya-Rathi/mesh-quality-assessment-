#!/usr/bin/env bash
#
# run_omp_bench.sh
# Benchmark gen_diamond_omp for different OMP_NUM_THREADS

EXEC=./gen_diamond_omp
N=1000
THREADS=(1 2 4 8 16 32 64 128)
LOG=omp_benchmark.txt

echo "OMP Benchmark: N=$N cells" > $LOG
echo "Threads,Creation_s" >> $LOG

for t in "${THREADS[@]}"; do
  echo "Running with $t threads..." | tee -a $LOG
  export OMP_NUM_THREADS=$t
  # capture only the creation time line
  line=$($EXEC $N 2>&1 | grep 'Creation time')
  # parse the seconds value
  secs=$(echo $line | awk '{print $3}')
  echo "$t,$secs" >> $LOG
done

echo "Done. Results in $LOG"
