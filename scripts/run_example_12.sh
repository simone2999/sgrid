export TMPDIR=/tmp

n_procs=$1

# Args
# nx=5
# ny=4
# nz=$(($n_procs * 3))
# tile_size=2
# n_tiles=2
# block_size=$(($n_procs * $tile_size * $n_tiles))


nx=20
ny=20
nz=20
tile_size=100
n_tiles=50
block_size=$(($n_procs * $tile_size * $n_tiles))


n_elements=$(($nx * $ny * $nz * $block_size))
echo 'n_entries='$n_elements

# rm ex12*

make -j4 sgrid_example_12 \
  && mpiexec -np $n_procs ./sgrid_example_12 $nx $ny $nz $tile_size $n_tiles $block_size 

  # \
  # && python3 ../scripts/transpose_data.py --nx=$nx --ny=$ny --nz=$nz --block_size=$block_size --path=ex12.raw --output=ex12_t \
  # && ls -lah ex12*.raw 
 