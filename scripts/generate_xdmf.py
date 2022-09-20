import sys

#TODO: modify if it's scalar or vector.
def main(example_name, file_name, time_steps):
    nx = 0
    ny = 0
    nz = 0
    endianess = ""
    block_size = 0
    tp = ""
    precision = ""
    number_type = ""
    path = "../build/" + example_name + '/'
    filename = file_name
    with open('../build/' + example_name + '/' + 'metadata.yml', 'r') as f:
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
            elif i[:6] == "type: ":
                tp = i[6:]
                if tp == "long\n":
                    precision = "8"
                    number_type = "Int"
                elif tp == "double\n":
                    precision = "8"
                    number_type = "Float"

    if int(time_steps) > 0:

        n_grids = int(time_steps)
        time_string_header = """<!DOCTYPE Xdmf SYSTEM "Xdmf.dtd" []>
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
                                1 1 1
                                </DataItem>
                                </Geometry>""".format(dim="" + str(nx) + " " + str(ny) + " " + str(nz) + "")
        time_string_global = """<Grid Name="TimeSeries" GridType="Collection" CollectionType="Temporal">
                                <Time TimeType="HyperSlab">
                                    <DataItem Format="XML" NumberType="Float" Dimensions="3">
                                    <!-- start stride count-->
                                    0.0 1.0 {n_grids}
                                    </DataItem>
                                </Time>""".format(n_grids=n_grids)
        time_string_local_final = ""
        for i in range(0,int(time_steps)):
            time_string_local_final =  time_string_local_final + """<Grid Name="T1" GridType="Uniform">
                            <Topology Reference="/Xdmf/Domain/Topology[1]"/>
                            <Geometry Reference="/Xdmf/Domain/Geometry[1]"/>
                            <Attribute Name="{filename}" Center="Node">
                                <DataItem Format="Binary" 
                                 DataType="Float" Precision="{precision}" Endian="{endianess}"
                                 Dimensions="{dim} {block_size}" NumberType="{number_type}">
                                    {filename}
                                </DataItem>
                            </Attribute>
                        </Grid>""".format(dim="" + str(nx) + " " + str(ny) + " " + str(nz) + "",endianess=endianess, filename=filename + "_t" + str(i) + ".raw",precision=precision, number_type=number_type, block_size=block_size) + "\n"
        end_grid = "\n</Grid>"
        time_footer = """\n</Domain>\n</Xdmf>"""
        time_string = time_string_header + time_string_global + time_string_local_final + end_grid + time_footer



                    
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
    1 1 1
    </DataItem>
    </Geometry>
    <Grid Name="T1" GridType="Uniform">
    <Topology Reference="/Xdmf/Domain/Topology[1]"/>
    <Geometry Reference="/Xdmf/Domain/Geometry[1]"/>
    <Attribute Name="U" Center="Node" AttributeType="Vector">
    <DataItem Format="Binary" Dimensions="{dim} {block_size}" Endian="{endianess}" Precision="{precision}" NumberType="{number_type}">
    <!-- data_t0.raw -->
    {filename}.raw
    </DataItem>
    </Attribute>
    </Grid>
    </Domain>
    </Xdmf>""".format(dim="" + str(nx) + " " + str(ny) + " " + str(nz) + "", endianess=endianess, filename=filename,
                      block_size=block_size, precision=precision, number_type=number_type)

    if int(time_steps) == 0:
        textfile = open(path + filename + ".xdmf", "w")
        textfile.write(xdmf_string)
        textfile.close()
    elif int(time_steps) > 0:
        textfile = open(path + filename + ".xdmf", "w")
        textfile.write(time_string)
        textfile.close()


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2],sys.argv[3])
