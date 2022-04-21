export TMPDIR=/tmp

n_tiles=2
n_procs=$1
block_size=$(($n_procs * 2 * $n_tiles))

rm ex11*

 make -j4 sgrid_example_11 \
  && mpiexec -np $n_procs ./sgrid_example_11 \
  && python3 ../scripts/transpose_data.py --nx=2 --ny=4 --nz=1 --block_size=$block_size --path=ex11.raw --output=ex11_t \
  && ls -lah ex11*.raw 
 