[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![CodeFactor](https://www.codefactor.io/repository/bitbucket/zulianp/sgrid/badge/main)](https://www.codefactor.io/repository/bitbucket/zulianp/sgrid/overview/main)
[![Build status](https://ci.appveyor.com/api/projects/status/x8ddkg5jw11dv9a8/branch/master?svg=true)](https://ci.appveyor.com/project/zulianp/sgrid/branch/master)

# README #

## Main contributors ##

- Dr. Patrick Zulian (Lead developer, Euler institute)
- Dylan Ramelli (CI, USI)

Developed at the Euler institute, USI, Lugano, Switzerland (https://www.euler.usi.ch/).

## License ##
The software is realized with NO WARRANTY and it is licenzed under BSD 3-Clause license

## Copyright ##

Copyright (c) 2020 Euler institute - USI Università della Svizzera Italiana

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

cd sgrid
mkdir build
cmake .. -DKokkosKernels_DIR=$TRILINOS_DIR/lib/cmake/KokkosKernels -DCMAKE_CXX_FLAGS=-fopenmp \
-DCMAKE_INSTALL_PREFIX=$HOME/sgrid_installation \
&& make && make install

```


## Example runs on Pitz-Daint ##

From the build folder

```bash
sbatch ../scripts/run_example_2.sbatch

```


## Citing sgrid ##

```bibtex
@misc{sgridgit,
	author = {Patrick Zulian and Dylan Ramelli},
	title = {{SGRID}: a structured grid manager for supercomputing},
	url = {https://bitbucket.org/zulianp/sgrid},
	howpublished = {https://bitbucket.org/zulianp/grid},
	year = {2020}
}
```
