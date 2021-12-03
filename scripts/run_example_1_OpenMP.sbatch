#!/bin/bash
#SBATCH -J Example_1_OpenMP
#SBATCH --output=Example_1_OpenMP-%j.out
#SBATCH --nodes=2
#SBATCH --tasks-per-node=2
#SBATCH --cpus-per-task=18
#SBATCH --time=00:10:00
#SBATCH --exclusive
#SBATCH -C mc
#SBATCH --account=u2


unset KOKKOS_PROFILE_LIBRARY

source /apps/daint/UES/anfink/cpu/environment

nx=300
ny=300

# Dangerous!
# MPICH_MPIIO_HINTS="*:romio_cb_write=disable:romio_cb_read=enable:cb_nodes=64"

export OMP_NUM_THREADS=18
export OMP_PROC_BIND=true
export MPICH_RDMA_ENABLED_CUDA=0

echo './sgrid_example_1 '$nx' '$ny
srun ./sgrid_example_1 $nx $ny

# python3  ../scripts/plot.py --path=data.raw --nx=$nx --ny=$ny --output=out.pdf