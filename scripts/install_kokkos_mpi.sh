#!/bin/bash

module purge
module load cmake git openmpi python
module list

mkdir $HOME/kokkos_mpi_deps
cd $HOME/kokkos_mpi_deps
git clone https://github.com/kokkos/kokkos.git
git clone https://github.com/kokkos/kokkos-kernels.git

export DEPS_DIR=$HOME/kokkos_mpi_deps
export KOKKOS_DIR=$DEPS_DIR/kokkos_mpi_install

cd $DEPS_DIR/kokkos && git submodule update --init --recursive
cd $DEPS_DIR/kokkos-kernels && git submodule update --init --recursive


cd $DEPS_DIR/kokkos && mkdir build
cd build \
&& cmake .. -DKokkos_ENABLE_CUDA=OFF \
            -DKokkos_ENABLE_DEBUG=ON \
            -DKokkos_ENABLE_OPENMP=ON  \
            -DKokkos_ENABLE_SERIAL=ON \
            -DKokkos_ENABLE_EXAMPLES=OFF \
            -DCMAKE_INSTALL_PREFIX=$KOKKOS_DIR \
            -DCMAKE_BUILD_TYPE=Release \
&& make -j && make install

cd $DEPS_DIR/kokkos-kernels
mkdir build

cd build \
&& cmake .. -DKokkos_DIR=$KOKKOS_DIR/lib64/cmake/Kokkos \
            -DKokkosKernels_ENABLE_EXAMPLES=OFF \
            -DCMAKE_INSTALL_PREFIX=$KOKKOS_DIR \
            -DCMAKE_BUILD_TYPE=Release \
&& make -j && make install
