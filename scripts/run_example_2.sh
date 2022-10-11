#!/bin/bash
# run_example_2.sh

nx=31
ny=32
nz=33
block_size=3

rm ../build/example_2/data.raw
rm ../build/example_2/data_t*.raw

# mpiexec -np 8 ./sgrid_example_2 $nx $ny $nz $block_size && \
#     ls -lah data*.raw

mpiexec -np 6 ./sgrid_example_2 $nx $ny $nz $block_size && \
    python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size --path=../build/example_2/data.raw&& \
    ls -lah ../build/example_2/data*.raw


python3 ../scripts/generate_xdmf.py example_2 data
python3 ../scripts/generate_xdmf.py example_2 idx
python3 ../scripts/generate_xdmf.py example_2 c_field
