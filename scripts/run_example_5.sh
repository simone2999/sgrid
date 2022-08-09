#!/bin/bash

make -j4 sgrid_example_5

nx=61
ny=10
block_size=3
comm_size=4

nx_debug=$(($nx+4))
ny_debug=$(($ny+4))


rm ../build/example_5/x.raw
rm ../build/example_5/x_debug.raw

echo './sgrid_example_5 '$nx' '$ny
OMP_PROC_BIND=true mpiexec -np $comm_size ./sgrid_example_5 $nx $ny

ls -lah ../build/example_5/*.raw

python3  ../scripts/plot.py --path=../build/example_5/x.raw --nx=$nx --ny=$ny --block_size=$block_size --Lx=$nx --Ly=$ny --output=../build/example_5/out.pdf
python3  ../scripts/plot.py --path=../build/example_5/x_debug.raw --nx=$nx_debug --ny=$ny_debug --block_size=$block_size --Lx=$nx_debug --Ly=$ny_debug --output=../build/example_5/out_debug.pdf
python3 ../scripts/generate_xdmf.py example_5 x