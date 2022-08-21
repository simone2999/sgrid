#!/bin/bash
# run_example_2.sh

nx=6
ny=6
nz=12
block_size=1

rm ../build/example_3/x.raw
rm ../build/example_3/x_t*.raw

mpiexec -np 6 ./sgrid_example_3 $nx $ny $nz $block_size && \
    python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size --path=../build/example_3/x.raw --output=../build/example_3/x_t && \
    ls -lah ../build/example_3/x*.raw
python3 ../scripts/generate_xdmf.py example_3 x