#!/bin/bash
#SBATCH -J Example_2
#SBATCH --output=Example_2-%j.out
#SBATCH --nodes=1
#SBATCH --tasks-per-node=36
#SBATCH --cpus-per-task=1
#SBATCH --time=00:10:00
#SBATCH --exclusive
#SBATCH -C mc
#SBATCH --account=u2

source /apps/daint/UES/anfink/cpu/environment

n=100
nx=$n
ny=$n
nz=$n
block_size=4
write_output=0

export OMP_NUM_THREADS=1
export OMP_PROC_BIND=false

rm data*.raw

echo './sgrid_example_2 '$nx' '$ny' '$nz' '$block_size' '$write_output
srun ./sgrid_example_2 $nx $ny $nz $block_size $write_output

# python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size && \
#     ls -lah data*.raw
