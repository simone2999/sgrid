import sys
import os
import string

# TODO: given file path create folder where we put .raw .xdmf, 
# with folder name as the name of the example 


def main(example_name, file_name):
    nx = 0
    ny = 0
    nz = 0
    endianess = ""
    block_size = 0
    path = "../build/" + example_name + '/'
    filename = file_name
    with open('../build/' + example_name + '/'+ 'xdmf_data.txt','r') as f:
        Lines = f.readlines()
        for i in Lines:
            if i[:3] == "nx:":
                nx = int(i[3:])
            elif i[:3] == "ny:":
                ny = int(i[3:])
            elif i[:3] == "nz:":
                nz = int(i[3:])
            elif i[:10] == "endianess:":
                endianess = i[10:]
            elif i[:11] == "block_size:":
                block_size = int(i[11:])

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
    </Xdmf>""".format(dim= "" + str(nx) + " " + str(ny) + " " + str(nz) + "", endianess=endianess, filename=filename, block_size=block_size)

    textfile = open(path + filename+".xdmf", "w")
    a = textfile.write(xdmf_string)
    textfile.close()
if __name__ == '__main__':
	main(sys.argv[1], sys.argv[2])