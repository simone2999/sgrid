#!/bin/bash
# run_example_15.sh
# Run a TimeSeries grid of Mandelbort fractal leveraging 
# xdmf xml format for easily reading data into ParaView.

nx=1000
ny=1000
nz=1
block_size=3

rm ../build/example_15/x.raw
rm ../build/example_15/x_t*.raw

mpiexec -np 6 ./sgrid_example_15 $nx $ny $nz $block_size 
# Specify folder and filename that was written. Filename preferably without .raw
python3 ../scripts/generate_xdmf.py example_15