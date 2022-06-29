export TMPDIR=/tmp

n_procs=$1

nx=2
ny=2
nz=2

make -j4 sgrid_example_14 \
  && mpiexec -np $n_procs ./sgrid_example_14