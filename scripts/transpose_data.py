import numpy as np
import sys, getopt
import string
import os


def main(argv):
    nx = -1
    ny = -1
    nz = -1
    block_size = 1

    path = "data.raw"
    output_path = "data_t"

    help_message = 'python transpose_data.py --nx=<n nodes in x direction> --ny=<n nodes in y direction> --nz=<n nodes in z direction>'

    try:
        opts, args = getopt.getopt(
            argv,"hx:y:z:p:b:o:",
            ["help", "nx=", "ny=", "nz=", "block_size=", "path=", "output="])
    except getopt.GetoptError as err:
        print(err)
        print(help_message)
        sys.exit(2)

    for opt, arg in opts:
        if opt in ('-h', '--help'):
            print(help_message)
            sys.exit()
        elif opt in ("-x", "--nx"):
            nx = int(arg)
        elif opt in ("-y", "--ny"):
            ny = int(arg)
        elif opt in ("-z", "--nz"):
            nz = int(arg)
        elif opt in ("-b", "--block_size"):
            block_size = int(arg)
        elif opt in ("-p", "--path"):
            path = arg
        elif opt in ("-o", "--output"):
            output_path = arg

    if nx == -1 or ny == -1 or nz == -1:
        print("Error %d %d %d" % (nx, ny, nz))
        print(help_message)
        sys.exit(2)

    binary_file = open(path, "r")
    a = np.fromfile(binary_file, dtype=np.float64)
    a = np.reshape(a, (nx, ny, nz, block_size))

    for b in range(0, block_size):
        a_b = np.transpose(a[:, :, :, b])
        binary_file_out = open(output_path + str(b) + ".raw", "wb")
        binary_file_out.write(a_b.tobytes())


if __name__ == '__main__':
    main(sys.argv[1:])
