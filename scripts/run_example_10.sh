 #!/bin/bash
# sgrid_example_10.sh

n_procs=$1
ny=$(($n_procs * 2 + 6))
echo 'ny='$ny

rm ex10_debug*

make -j4 sgrid_example_10

 make -j4 sgrid_example_10 \
  && mpiexec -np $n_procs ./sgrid_example_10 0 1\
  && python3 ../scripts/transpose_data.py --nx=8 --ny=$ny --nz=1 --block_size=2 --path=ex10_debug.raw --output=ex10_debug_t \
  && ls -lah ex10_debug*.raw 
 


# mpiexec -np $n_procs ./sgrid_example_10 1 > dump1.txt \
#     && mpiexec -np $n_procs ./sgrid_example_10 0 > dump0.txt \
#     && diff dump1.txt dump0.txt > diff_dump.txt; cat diff_dump.txt
