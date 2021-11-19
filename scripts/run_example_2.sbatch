#!/bin/bash
#SBATCH -J Example_2
#SBATCH --output=Example_2-%j.out
#SBATCH --nodes=12
#SBATCH --tasks-per-node=20
#SBATCH --cpus-per-task=1
#SBATCH --time=00:20:00
#SBATCH --mem=12020
#SBATCH --exclusive
#SBATCH --partition=slim

module load openmpi python

n=1500
nx=$n
ny=$n
nz=$n
block_size=4
write_output=0

export OMP_NUM_THREADS=1
export OMP_PROC_BIND=false

rm data*.raw

echo './example_2 '$nx' '$ny' '$nz' '$block_size' '$write_output
mpirun ./example_2 $nx $ny $nz $block_size $write_output


# python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size && \
#     ls -lah data*.raw
