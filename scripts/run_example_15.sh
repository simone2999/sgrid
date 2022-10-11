#!/bin/bash
# run_example_15.sh

nx=1000
ny=1000
nz=1
block_size=3

rm ../build/example_15/x.raw
rm ../build/example_15/x_t*.raw

mpiexec -np 6 ./sgrid_example_15 $nx $ny $nz $block_size 
python3 ../scripts/generate_xdmf.py example_15 x_t