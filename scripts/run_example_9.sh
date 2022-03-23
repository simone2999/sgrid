 #!/bin/bash
# sgrid_example_9.sh

 make -j4 sgrid_example_9 &&  ./sgrid_example_9 1 > dump1.txt  &&  ./sgrid_example_9 0 > dump0.txt; diff dump1.txt dump0.txt > diff_dump.txt; cat diff_dump.txt