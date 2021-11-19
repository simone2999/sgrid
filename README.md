# README #

Installation instructions

```bash

git clone https://zulianp@bitbucket.org/zulianp/sgrid.git
cd sgrid
mkdir build
cd build && cmake .. -DCMAKE_INSTALL_PREFIX=<Install dir> -DKokkosKernels_DIR=<Path to cmake config> && make && make install
```