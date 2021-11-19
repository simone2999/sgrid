#!/bin/bash
#SBATCH -J Example_1
#SBATCH --output=Example_1-%j.out
#SBATCH --nodes=2
#SBATCH --tasks-per-node=36
#SBATCH --cpus-per-task=1
#SBATCH --time=00:10:00
#SBATCH --mem=12020
#SBATCH --exclusive
#SBATCH -C mc
#SBATCH --account=u2


module load openmpi python

nx=300
ny=300

# Dangerous!
# MPICH_MPIIO_HINTS="*:romio_cb_write=disable:romio_cb_read=enable:cb_nodes=64"

export OMP_NUM_THREADS=1
export OMP_PROC_BIND=false

echo './example_1 '$nx' '$ny
srun ./example_1 $nx $ny

# python3  ../scripts/plot.py --path=data.raw --nx=$nx --ny=$ny --output=out.pdf