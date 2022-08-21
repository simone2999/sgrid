import sys
import os
import string

def main(example_name, file_name):
    nx = 0
    ny = 0
    nz = 0
    endianess = ""
    block_size = 0
    path = "../build/" + example_name + '/'
    filename = file_name
    with open('../build/' + example_name + '/'+ 'metadata.yml','r') as f:
        Lines = f.readlines()
        for i in Lines:
            if i[:4] == "nx: ":
                nx = int(i[4:])
            elif i[:4] == "ny: ":
                ny = int(i[4:])
            elif i[:4] == "nz: ":
                nz = int(i[4:])
            elif i[:11] == "endianess: ":
                endianess = i[11:]
            elif i[:12] == "block_size: ":
                block_size = int(i[12:])

    xdmf_string = """<!DOCTYPE Xdmf SYSTEM "Xdmf.dtd" []>
    <Xdmf xmlns:xi="http://www.w3.org/2001/XInclude" Version="2.0">
    <Domain>
    <Topology name="topo" TopologyType="3DRectMesh"
    Dimensions="{dim}">
    </Topology>
    <Geometry name="geo" Type="ORIGIN_DXDYDZ">
    <!-- Origin -->
    <DataItem Format="XML" Dimensions="3">
    0.0 0.0 0.0
    </DataItem>
    <!-- DxDyDz -->
    <DataItem Format="XML" Dimensions="3">
    1 1 1
    </DataItem>
    </Geometry>
    <Time TimeType="HyperSlab">
             <DataItem Format="XML" NumberType="Float" Dimensions="3">
             0.0 1.0 1
             </DataItem>
         </Time>
    <Grid Name="T1" GridType="Uniform">
    <Topology Reference="/Xdmf/Domain/Topology[1]"/>
    <Geometry Reference="/Xdmf/Domain/Geometry[1]"/>
    <Attribute Name="U" Center="Node">
    <DataItem Format="Binary"
    DataType="Float" Endian="{endianess}" Dimensions="{dim}">
    <!-- data_t0.raw -->
    {filename}.raw
    </DataItem>
    </Attribute>
    </Grid>
    </Domain>
    </Xdmf>""".format(dim= "" + str(nx) + " " + str(ny) + " " + str(nz) + "", endianess=endianess, filename=filename, block_size=block_size)

    textfile = open(path + filename+".xdmf", "w")
    a = textfile.write(xdmf_string)
    textfile.close()
if __name__ == '__main__':
	main(sys.argv[1], sys.argv[2])