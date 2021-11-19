import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
import sys, getopt
import glob
import string
import os



def batch_plot(dir, nx, ny, output_dir):
    raw_files = glob.glob(dir + "/*.raw")

    for f in raw_files:
        out_f = os.path.basename(f)
        out_f = out_f.replace(".raw", ".png", 1)
        output_path = output_dir + "/" + out_f
        print("Convert .raw to .png: " + f + " -> " + output_path)
        plot(f, nx, ny, output_path)



def plot(path, nx, ny, output_path):
    image = open(path, "r")
    a = np.fromfile(image, dtype=np.float64)
    a = np.reshape(a, (nx, ny))
    a = np.transpose(a)

    plt.clf()
    imgplot = plt.imshow(a, aspect=((1.0 * (nx-1))/(ny - 1)),
        # cmap=plt.get_cmap('gist_rainbow')
        cmap=plt.get_cmap('jet')
        )
    plt.xlim([0, nx-1])
    plt.ylim([0, ny-1])
    plt.colorbar(label="u")
    plt.xlabel("x")
    plt.ylabel("y")
    plt.savefig(output_path)


def main(argv):
    nx = -1
    ny = -1

    path = "out.raw"
    output_path = "out.png"
    batch = False

    help_message = 'python plot.py --nx=<n nodes in x direction> --ny=<n nodes in y direction>'

    try:
        opts, args = getopt.getopt(
            argv,"hx:y:p:o:",
            ["help", "nx=", "ny=", "path=", "output=", "batch"])
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
        elif opt in ("-p", "--path"):
            path = arg
        elif opt in ("-o", "--output"):
            output_path = arg
        elif opt in ("--batch"):
            batch = True

    if nx == -1 or ny == -1:
        print(help_message)
        sys.exit(2)

    if batch:
        batch_plot(path, nx, ny, output_path)
    else:
        plot(path, nx, ny, output_path)


if __name__ == '__main__':
    main(sys.argv[1:])

