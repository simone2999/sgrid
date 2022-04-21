#!/bin/bash
#SBATCH -J Example_12
#SBATCH --output=Example_12-%j.out
#SBATCH --nodes=4
#SBATCH --tasks-per-node=20
#SBATCH --cpus-per-task=1
#SBATCH --time=00:10:00
#SBATCH --exclusive
#SBATCH --partition=fat

n_procs=80

nx=100
ny=100
nz=100
tile_size=4
n_tiles=10
block_size=$(($n_procs * $tile_size * $n_tiles))

n_elements=$(($nx * $ny * $nz * $block_size))

echo 'n_procs='$n_procs
echo 'n_entries='$n_elements
mpirun ./sgrid_example_12 $nx $ny $nz $tile_size $n_tiles $block_size 