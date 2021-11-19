#!/bin/bash
#SBATCH -J Example_2_OpenMP
#SBATCH --output=Example_2_OpenMP-%j.out
#SBATCH --nodes=48
#SBATCH --tasks-per-node=2
#SBATCH --cpus-per-task=18
#SBATCH --time=00:10:00
#SBATCH --exclusive
#SBATCH -C mc
#SBATCH --account=u2


unset KOKKOS_PROFILE_LIBRARY

source /apps/daint/UES/anfink/cpu/environment

rm data*.raw

n=100
nx=$n
ny=$n
nz=$n
block_size=40000
write_output=0

# Dangerous!
# MPICH_MPIIO_HINTS="*:romio_cb_write=disable:romio_cb_read=enable:cb_nodes=64"

export OMP_NUM_THREADS=18
export OMP_PROC_BIND=true
export MPICH_RDMA_ENABLED_CUDA=0

echo './example_2 '$nx' '$ny' '$nz' '$block_size' '$write_output
srun ./example_2 $nx $ny $nz $block_size $write_output

# python3  ../scripts/plot.py --path=data.raw --nx=$nx --ny=$ny --output=out.pdf