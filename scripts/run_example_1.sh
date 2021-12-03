#!/bin/bash

# run_example_1.sh

nx=200
ny=200

rm data.raw

echo './sgrid_example_1 '$nx' '$ny
OMP_PROC_BIND=true mpiexec -np 8 ./sgrid_example_1 $nx $ny

python3  ../scripts/plot.py --path=data.raw --nx=$nx --ny=$ny --output=out.pdf