#!/bin/bash
# run_example_2.sh

nx=31
ny=32
nz=33
block_size=3

rm data.raw
rm data_t*.raw

# mpiexec -np 8 ./sgrid_example_2 $nx $ny $nz $block_size && \
#     ls -lah data*.raw

mpiexec -np 16 ./sgrid_example_2 $nx $ny $nz $block_size && \
    python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size && \
    ls -lah data*.raw
