#! /bin/bash
mkdir -p bin
#
# Use clang with Homebrew libomp for OpenMP support on macOS
clang -O3 -Xpreprocessor -fopenmp \
      -I/opt/homebrew/opt/libomp/include \
      -c heated_plate_openmp.c
if [ $? -ne 0 ]; then
  echo "Compile error."
  exit
fi
#
clang -O3 -Xpreprocessor -fopenmp \
      heated_plate_openmp.o \
      -L/opt/homebrew/opt/libomp/lib -lomp -lm -o heated_plate_openmp.out
if [ $? -ne 0 ]; then
  echo "Load error."
  exit
fi
rm heated_plate_openmp.o
mv heated_plate_openmp.out bin/heated_plate_openmp
echo "===== Comparing OpenMP schedules ====="
for sched in static dynamic guided; do
    export OMP_SCHEDULE="$sched,1"
    echo ">>> Schedule: $sched"
    bin/heated_plate_openmp
done

# -------------------------------------------------------------------
#  Compile and run the parallel_for variant
# -------------------------------------------------------------------
echo "===== Building Pthreads executable ====="
# link with parallel_for implementation
clang -O3 -pthread heated_plate_pthreads.c parallel_for.c -lm -o heated_plate_pthreads.out
if [ $? -ne 0 ]; then
  echo "Pthreads compile error."
  exit
fi
mv heated_plate_pthreads.out bin/heated_plate_pthreads

echo "===== Running Pthreads reference ====="
bin/heated_plate_pthreads

#
echo "Normal end of execution."
