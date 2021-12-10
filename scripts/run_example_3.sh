#!/bin/bash
# run_example_2.sh

nx=4
ny=4
nz=10
block_size=3

rm x.raw
rm x_t*.raw

mpiexec -np 8 ./sgrid_example_3 $nx $ny $nz $block_size && \
    python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size --path=x.raw --output=x_t && \
    ls -lah x*.raw
