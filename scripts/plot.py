import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
import sys, getopt
import glob
import string
import os

invalid_float=-777

class PlotParams:
    def __init__(self, name, nx, ny, vmin, vmax):
        self.name = name
        self.nx = nx
        self.ny = ny
        self.vmin = vmin
        self.vmax = vmax
        self.detect_bounds_for_time_series = True
        self.gpu = False
        self.Lx = 1
        self.Ly = 1
        self.aspect_ratio = 1
        # self.aspect_ratio = ((1.0 * (nx- 1))/(ny - 1))

def get_min_and_max(path, nx, ny):
    image = open(path, "r")
    a = np.fromfile(image, dtype=np.float64)
    vmin = np.min(a)
    vmax = np.max(a)

    return vmin, vmax

def get_min_and_max_from_series(raw_files, nx, ny):
    first = True
    vmin = invalid_float
    vmax = invalid_float

    for f in raw_files:
        f_vmin, f_vmax = get_min_and_max(f,nx,ny)

        if first:
            vmin = f_vmin
            vmax = f_vmax
            first = False
        else:
            vmin = f_vmin if vmin > f_vmin else vmin
            vmax = f_vmax if vmax < f_vmax else vmax

    return vmin, vmax

def batch_plot(params, dir, output_dir):
    nx = params.nx
    ny = params.ny

    raw_files = glob.glob(dir + "/*.raw")

    if params.detect_bounds_for_time_series and params.vmin == invalid_float and params.vmax == invalid_float:
        vmin, vmax = get_min_and_max_from_series(raw_files, nx, ny)
        params.vmin = vmin
        params.vmax = vmax

    for f in raw_files:
        out_f = os.path.basename(f)
        out_f = out_f.replace(".raw", ".png", 1)
        output_path = output_dir + "/" + out_f
        print("Convert .raw to .png: " + f + " -> " + output_path)
        plot(params, f,output_path)

def plot(params, path, output_path):
    nx = params.nx
    ny = params.ny
    block_size = params.block_size
    vmin = params.vmin
    vmax = params.vmax

    image = open(path, "r")
    a = np.fromfile(image, dtype=np.float64)

    min_a = np.min(a)
    max_a = np.max(a)

    if block_size > 1:
        if params.gpu:
            a = np.reshape(a, (block_size, ny, nx))
            # a = np.transpose(a)
        else:
            a = np.reshape(a, (nx, ny, block_size))
            a = np.transpose(a)
    else:
        if params.gpu:
            a = np.reshape(a, (ny, nx))
            # a = np.transpose(a)
        else:
            a = np.reshape(a, (nx, ny))
            a = np.transpose(a)



    if vmin == invalid_float:
        vmin = min_a

    if vmax == invalid_float:
        vmax = max_a


    if block_size == 1:
        plt.clf()
        imgplot = plt.imshow(a, aspect=params.aspect_ratio,
            extent=[0,params.Lx,params.Ly,0],
            # cmap=plt.get_cmap('gist_rainbow')
            cmap=plt.get_cmap('jet'),
            vmin=vmin, vmax=vmax
            )
        plt.xlim([0, params.Lx])
        plt.ylim([0, params.Ly])
        plt.colorbar(label="u", orientation="horizontal")
        plt.xlabel("x")
        plt.ylabel("y")

        sum_a = float(np.sum(a))
        avg_a = sum_a/(nx * ny)

        title = "%s, min=%.3f, max=%.3f, avg=%.3f" % (params.name, min_a, max_a, avg_a)
        plt.title(title)
        plt.savefig(output_path)
    else:
        plt.clf()
        fig, axs = plt.subplots(block_size)

        sum_a = np.sum(a)
        avg_a = sum_a/(nx * ny * block_size)

        title = "%s, min=%.3f, max=%.3f, avg=%.3f" % (params.name, min_a, max_a, avg_a)
        fig.suptitle(title)


        for i in range(0, block_size):
            vmin=np.min(a[i,:,:])
            vmax=np.max(a[i,:,:])

            imgplot = axs[i].imshow(a[i,:,:], aspect=params.aspect_ratio,
                extent=[0,params.Lx,params.Ly,0],
                cmap=plt.get_cmap('jet'),
                vmin=vmin, vmax=vmax
                )

            axs[i].set(xlim=[0, params.Lx], ylim=[0, params.Ly])
            plt.colorbar(imgplot, label="u", orientation="vertical", ax=axs[i])
            axs[i].set(xlabel="x", ylabel="y")

        fig.savefig(output_path)


def main(argv):
    nx = -1
    ny = -1
    block_size = 1

    path = "out.raw"
    output_path = "out.png"
    batch = False
    vmin=invalid_float
    vmax=invalid_float
    name = "Result"
    gpu = False
    Lx = 1
    Ly = 1

    help_message = 'python plot.py --nx=<n nodes in x direction> --ny=<n nodes in y direction>'

    try:
        opts, args = getopt.getopt(
            argv,"hx:y:p:o:b:",
            ["help", "nx=", "ny=", "path=", "output=", "batch", "vmin=", "vmax=", "name=", "gpu", "Lx=", "Ly=", "block_size="])
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
        elif opt in ("--vmin"):
            vmin = float(arg)
        elif opt in ("--vmax"):
            vmax = float(arg)
        elif opt in ("-p", "--path"):
            path = arg
        elif opt in ("-o", "--output"):
            output_path = arg
        elif opt in ("--batch"):
            batch = True
        elif opt in ("--gpu"):
            gpu = True
        elif opt in ("--name"):
            name = arg
        elif opt in ("--Lx"):
            Lx = float(arg)
        elif opt in ("--Ly"):
            Ly = float(arg)
        elif opt in ("-b", "--block_size"):
            block_size = int(arg)

    if nx == -1 or ny == -1:
        print(help_message)
        sys.exit(2)


    params = PlotParams(name, nx, ny, vmin, vmax)
    params.gpu = gpu
    params.Lx = Lx
    params.Ly = Ly
    params.block_size = block_size

    if batch:
        batch_plot(params, path, output_path)
    else:
        plot(params, path, output_path)


if __name__ == '__main__':
    main(sys.argv[1:])

