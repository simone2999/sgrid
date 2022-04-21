export TMPDIR=/tmp

n_tiles=2
n_procs=$1
block_size=$(($n_procs * 2 * $n_tiles))
nx=5
ny=4
nz=$(($n_procs * 3))

rm ex12*

 make -j4 sgrid_example_12 \
  && mpiexec -np $n_procs ./sgrid_example_12 \
  && python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size --path=ex12.raw --output=ex12_t \
  && ls -lah ex12*.raw 
 