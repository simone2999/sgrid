#!/bin/bash
# sgrid_example_4.sh

make -j4 sgrid_example_8

nx=2
ny=2
nz=3
block_size=1
slice_number=1
comm_size=2

rm ex8.raw
rm ex8_t*.raw

mpiexec -np $comm_size ./sgrid_example_8 $nx $ny $nz $block_size $slice_number && \
    python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size --path=ex8.raw --output=ex8_t && \
    ls -lah ex8*.raw

