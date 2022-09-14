#!/bin/bash
# sgrid_example_4.sh

make -j4 sgrid_example_4

# nx=3
# ny=3
# nz=70
# block_size=3
# comm_size=8

nx=30
ny=40
nz=20
block_size=3
comm_size=4

stride_y=$(($nz + 2))
echo 'stride_y='$stride_y
stride_x=$(( ($ny + 2) * $stride_y))
echo 'stride_x='$stride_x

rm ../build/example_4/x.raw
rm ../build/example_4/x_t*.raw

mpiexec -np $comm_size ./sgrid_example_4 $nx $ny $nz && \
    python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size --path=../build/example_4/x.raw --output=../build/example_4/x_t && \
    ls -lah ../build/example_4/x*.raw

python3 ../scripts/generate_xdmf.py example_4 x 3

