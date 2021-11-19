# README #

## General installation instructions ##

```bash

git clone https://zulianp@bitbucket.org/zulianp/sgrid.git
cd sgrid
mkdir build
cd build && cmake .. -DCMAKE_INSTALL_PREFIX=<Install dir> -DKokkosKernels_DIR=<Path to cmake config> \
&& make && make install
```


## Configuration on Pitz-Daint ##

```bash

source /apps/daint/UES/anfink/cpu/environment
export LD_LIBRARY_PATH=$TRILINOS_DIR/lib:$LD_LIBRARY_PATH

cmake -DKokkosKernels_DIR=$TRILINOS_DIR/lib/cmake/KokkosKernels -DCMAKE_CXX_FLAGS=-fopenmp \
&& make && make install

```


## Example runs on Pitz-Daint ##

From the build folder

```bash
sbatch ../scripts/run_example_2.sbatch

```