 #!/bin/bash
# sgrid_example_9.sh

n_procs=$1
nz=$(($n_procs * 2 + 6))

echo "nz="$nz
rm ex9_debug*

make -j4 sgrid_example_9

 make -j4 sgrid_example_9 \
  && mpiexec -np $n_procs ./sgrid_example_9 0 \
  && python3 ../scripts/transpose_data.py --nx=8 --ny=8 --nz=$nz --block_size=3 --path=ex9_debug.raw --output=ex9_debug_t \
  && ls -lah ex9_debug*.raw 
 


# mpiexec -np $n_procs ./sgrid_example_9 1 > dump1.txt \
#     && mpiexec -np $n_procs ./sgrid_example_9 0 > dump0.txt \
#     && diff dump1.txt dump0.txt > diff_dump.txt; cat diff_dump.txt
