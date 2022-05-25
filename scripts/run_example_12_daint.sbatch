#!/bin/bash
#SBATCH -J Example_12
#SBATCH --output=Example_12-%j.out
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=12
#SBATCH --time=00:10:00
#SBATCH -C gpu
#SBATCH --account=c24
#SBATCH -p debug

source /apps/daint/UES/anfink/cpu/environment

export OMP_NUM_THREADS=1
export OMP_PROC_BIND=false

n_procs=12

nx=10
ny=10
nz=10
tile_size=40
n_tiles=100
block_size=$(($n_procs * $tile_size * $n_tiles))

n_elements=$(($nx * $ny * $nz * $block_size))


echo 'tasks: '$SLURM_NTASKS
echo 'tasks-per-node: '$SLURM_NTASKS_PER_NODE
echo 'cpus-per-task: '$SLURM_CPUS_PER_TASK


echo 'n_procs='$n_procs
echo 'n_entries='$n_elements
srun ./sgrid_example_12 $nx $ny $nz $tile_size $n_tiles $block_size 
