#!/bin/bash

make -j4 sgrid_example_7


nx=41
ny=51
# nx=4
# ny=5
# nx=2
# ny=2
nz=11
block_size=1

# comm_size=1
# nx_debug=$(($nx + 2))
# ny_debug=$(($ny + 2))
# nz_debug=$(($nz + 2))

# comm_size=4
# nx_debug=$(($nx + 4))
# ny_debug=$(($ny + 4))
# nz_debug=$(($nz + 2))


comm_size=6
nx_debug=$(($nx + 4))
ny_debug=$(($ny + 4))
nz_debug=$(($nz + 4))

rm ../build/example_7/*.raw

echo './sgrid_example_7 '$nx' '$ny' '$nz' '$block_size
OMP_PROC_BIND=true mpiexec -np $comm_size ./sgrid_example_7 $nx $ny $nz $block_size && \
    python3 ../scripts/transpose_data.py --nx=$nx_debug --ny=$ny_debug --nz=$nz_debug --block_size=$block_size --path=../build/example_7/x_debug.raw --output=../build/example_7/x_t && \
    ls -lah x*.raw


echo 'Image size='$nx_debug' '$ny_debug' '$nz_debug
# ls -lah *.raw

# python3 ../scripts/generate_xdmf.py example_7
