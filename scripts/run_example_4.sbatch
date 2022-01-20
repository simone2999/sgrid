#!/bin/bash
#SBATCH -J Example_4
#SBATCH --output=Example_4-%j.out
#SBATCH --nodes=1
#SBATCH --tasks-per-node=36
#SBATCH --cpus-per-task=1
#SBATCH --time=00:10:00
#SBATCH --exclusive
#SBATCH -C mc
#SBATCH --account=u2

source /apps/daint/UES/anfink/cpu/environment


export OMP_NUM_THREADS=1
export OMP_PROC_BIND=false

rm data*.raw

nx=30
ny=40
nz=20
block_size=3
comm_size=20

stride_y=$(($nz + 2))
echo 'stride_y='$stride_y
stride_x=$(( ($ny + 2) * $stride_y))
echo 'stride_x='$stride_x

rm x.raw
rm x_t*.raw


echo './sgrid_example_4 '$nx' '$ny' '$nz

srun ./sgrid_example_4 $nx $ny $nz && \
    python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size --path=x.raw --output=x_t && \
    ls -lah x*.raw

