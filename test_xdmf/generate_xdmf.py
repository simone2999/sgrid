import sys
import os
import string

import numpy as np
import getopt


def main(argv):
    nx = -1
    ny = -1
    nz = 1
    block_size = 1
    path = "out"
    endianess="Little"

    help_message = 'python generate_xdmf.py --nx=<n nodes in x direction> --ny=<n nodes in y direction> --nz=<n nodes in z direction>'

    try:
        opts, args = getopt.getopt(
            argv,"hx:y:p:o:b:",
            ["help", "nx=", "ny=", "nz=","path=","block_size=", "endian="])
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
        elif opt in ("-p", "--path"):
            path = arg
            path = path.replace(".raw","")
        elif opt in ("-b", "--block_size"):
            block_size = int(arg)
        elif endian in ("-e", "-endian"):
            endianess = arg



    xdmf_string = """<!DOCTYPE Xdmf SYSTEM "Xdmf.dtd" []>
    <Xdmf xmlns:xi="http://www.w3.org/2001/XInclude" Version="2.0">
    <Domain>
    <Topology name="topo" TopologyType="3DCoRectMesh"
    Dimensions="{dim}">
    </Topology>
    <Geometry name="geo" Type="ORIGIN_DXDYDZ">
    <!-- Origin -->
    <DataItem Format="XML" Dimensions="3">
    0.0 0.0 0.0
    </DataItem>
    <!-- DxDyDz -->
    <DataItem Format="XML" Dimensions="3">
    {block_size} {block_size} {block_size}
    </DataItem>
    </Geometry>
    <Grid Name="TimeSeries" GridType="Collection" CollectionType="Temporal">
    <!--  <Time TimeType="HyperSlab">
    <DataItem Format="XML" NumberType="Float" Dimensions="3">
    0.0 1.0 1
    </DataItem>
    </Time> -->
    <Grid Name="T1" GridType="Uniform">
    <Topology Reference="/Xdmf/Domain/Topology[1]"/>
    <Geometry Reference="/Xdmf/Domain/Geometry[1]"/>
    <Attribute Name="T" Center="Node">
    <DataItem Format="Binary" 
    DataType="Float" Precision="8" Endian="{endianess}"
    Dimensions="{dim}">
    <!-- data_t0.raw -->
    {filename}.raw
    </DataItem>
    </Attribute>
    </Grid>
    </Grid>
    </Domain>
    </Xdmf>""".format(dim= "" + str(nx) + " " + str(ny) + " " + str(nz) + "", endianess=endianess, filename=path, block_size=block_size)

    textfile = open(path + ".xdmf", "w")
    a = textfile.write(xdmf_string)
    textfile.close()
if __name__ == '__main__':

	main(sys.argv[1:])