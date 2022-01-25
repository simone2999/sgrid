#!/bin/bash

make -j4 sgrid_example_7

nx=3
ny=4
nz=5
block_size=1

comm_size=4

# rm x.raw
# rm x_debug.raw

echo './sgrid_example_7 '$nx' '$ny' '$nz' '$block_size
OMP_PROC_BIND=true mpiexec -np $comm_size ./sgrid_example_7 $nx $ny $nz $block_size

# ls -lah *.raw
