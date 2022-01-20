#ifndef SGRID_RAW_IO_HPP
#define SGRID_RAW_IO_HPP

#include <string>

#include <mpi.h>

namespace sgrid {

template <class Field> class RawIO {
public:
  using Grid = typename Field::Grid;
  using ValueType = typename Field::ValueType;
  using LocalOrdinal = typename Field::LocalOrdinal;
  using GlobalOrdinal = typename Field::GlobalOrdinal;
  using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
  using HostMirror = typename ViewDevice::HostMirror;

  void set_output_path(const std::string &output_path) {
    output_path_ = output_path;
  }

  RawIO(Field &field) : field_(field) { init(); }

  ~RawIO() { destroy(); }

  void write() {
    field_.synch_device_to_host();

    auto grid = field_.grid();

    MPI_Comm comm = grid->raw_comm();
    // auto g_host = grid->view_host();

    MPI_Datatype real_type = MPIType<ValueType>();

    MPI_File fout;

    CATCH_MPI_ERROR(MPI_File_open(comm, output_path_.c_str(),
                                  MPI_MODE_WRONLY | MPI_MODE_CREATE,
                                  MPI_INFO_NULL, &fout));

    CATCH_MPI_ERROR(MPI_File_set_view(fout, 0, real_type, view_type_, "native",
                                      MPI_INFO_NULL));
    CATCH_MPI_ERROR(MPI_File_write_all(fout, field_.view_host().data().data(),
                                       1, interior_subarray_type_,
                                       MPI_STATUS_IGNORE));

    CATCH_MPI_ERROR(MPI_File_close(&fout));

    // Clean-up
    // MPI_Type_free(&view_type_);
    // MPI_Type_free(&interior_subarray_type_);
  }

private:
  Field &field_;
  std::string output_path_{"out.raw"};

  MPI_Datatype interior_subarray_type_{MPI_DATATYPE_NULL};
  MPI_Datatype view_type_{MPI_DATATYPE_NULL};

  void destroy() {
    if (interior_subarray_type_ != MPI_DATATYPE_NULL) {
      MPI_Type_free(&interior_subarray_type_);
    }

    if (view_type_ != MPI_DATATYPE_NULL) {
      MPI_Type_free(&view_type_);
    }
  }

  void init() {
    auto grid = field_.grid();

    auto g_host = grid->view_host();

    MPI_Datatype real_type = MPIType<ValueType>();
    int block_size = field_.block_size();

    int tensor_size = Grid::Dim;
    if (block_size > 1) {
      tensor_size++;
    }

    /////////////////////////////////////////////////////////////////////////////
    // Interior grid type

    int array_of_sizes[Grid::Dim + 1];
    int array_of_subsizes[Grid::Dim + 1];
    int array_of_starts[Grid::Dim + 1];

    // Handle blocks!
    array_of_sizes[Grid::Dim] = block_size;
    array_of_subsizes[Grid::Dim] = block_size;
    array_of_starts[Grid::Dim] = 0;

    for (int d = 0; d < Grid::Dim; ++d) {
      array_of_sizes[d] = g_host.dim[d] + 2 * g_host.margin[d];
      array_of_subsizes[d] = g_host.dim[d];
      array_of_starts[d] = g_host.margin[d];
    }

    CATCH_MPI_ERROR(MPI_Type_create_subarray(
        tensor_size, array_of_sizes, array_of_subsizes, array_of_starts,
        MPI_ORDER_C, real_type, &interior_subarray_type_));

    CATCH_MPI_ERROR(MPI_Type_commit(&interior_subarray_type_));

    /////////////////////////////////////////////////////////////////////////////
    // File view type

    for (int d = 0; d < Grid::Dim; ++d) {
      array_of_sizes[d] = g_host.global_dim[d];
      array_of_subsizes[d] = g_host.dim[d];
      array_of_starts[d] = g_host.start[d];
    }

    CATCH_MPI_ERROR(MPI_Type_create_subarray(
        tensor_size, array_of_sizes, array_of_subsizes, array_of_starts,
        MPI_ORDER_C, real_type, &view_type_));

    CATCH_MPI_ERROR(MPI_Type_commit(&view_type_));
  }
};

template <class Field> class RawIODebug {
public:
  using Grid = typename Field::Grid;
  using ValueType = typename Field::ValueType;
  using LocalOrdinal = typename Field::LocalOrdinal;
  using GlobalOrdinal = typename Field::GlobalOrdinal;
  using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
  using HostMirror = typename ViewDevice::HostMirror;

  void set_output_path(const std::string &output_path) {
    output_path_ = output_path;
  }

  RawIODebug(Field &field) : field_(field) { init(); }

  ~RawIODebug() { destroy(); }

  void write() {
    field_.synch_device_to_host();

    auto grid = field_.grid();

    MPI_Comm comm = grid->raw_comm();
    MPI_Datatype real_type = MPIType<ValueType>();

    MPI_File fout;

    CATCH_MPI_ERROR(MPI_File_open(comm, output_path_.c_str(),
                                  MPI_MODE_WRONLY | MPI_MODE_CREATE,
                                  MPI_INFO_NULL, &fout));

    CATCH_MPI_ERROR(MPI_File_set_view(fout, 0, real_type, view_type_, "native",
                                      MPI_INFO_NULL));
    CATCH_MPI_ERROR(MPI_File_write_all(fout, field_.view_host().data().data(),
                                       1, interior_subarray_type_,
                                       MPI_STATUS_IGNORE));

    CATCH_MPI_ERROR(MPI_File_close(&fout));

    // Clean-up
    // MPI_Type_free(&view_type_);
    // MPI_Type_free(&interior_subarray_type_);
  }

private:
  Field &field_;
  std::string output_path_{"out.raw"};

  MPI_Datatype interior_subarray_type_{MPI_DATATYPE_NULL};
  MPI_Datatype view_type_{MPI_DATATYPE_NULL};

  void destroy() {
    if (interior_subarray_type_ != MPI_DATATYPE_NULL) {
      MPI_Type_free(&interior_subarray_type_);
    }

    if (view_type_ != MPI_DATATYPE_NULL) {
      MPI_Type_free(&view_type_);
    }
  }

  void init() {
    auto grid = field_.grid();

    auto g_host = grid->view_host();

    MPI_Datatype real_type = MPIType<ValueType>();
    int block_size = field_.block_size();

    int tensor_size = Grid::Dim;
    if (block_size > 1) {
      tensor_size++;
    }

    /////////////////////////////////////////////////////////////////////////////
    // Interior grid type

    int array_of_sizes[Grid::Dim + 1];
    int array_of_subsizes[Grid::Dim + 1];
    int array_of_starts[Grid::Dim + 1];

    // Handle blocks!
    array_of_sizes[Grid::Dim] = block_size;
    array_of_subsizes[Grid::Dim] = block_size;
    array_of_starts[Grid::Dim] = 0;

    for (int d = 0; d < Grid::Dim; ++d) {
      array_of_sizes[d] = g_host.dim_with_margin[d];
      array_of_subsizes[d] = g_host.dim_with_margin[d];
      array_of_starts[d] = 0;
    }

    CATCH_MPI_ERROR(MPI_Type_create_subarray(
        tensor_size, array_of_sizes, array_of_subsizes, array_of_starts,
        MPI_ORDER_C, real_type, &interior_subarray_type_));

    CATCH_MPI_ERROR(MPI_Type_commit(&interior_subarray_type_));

    /////////////////////////////////////////////////////////////////////////////
    // File view type

    int array_of_global_sizes[Grid::Dim + 1];
    for (int d = 0; d < Grid::Dim; ++d) {
      array_of_global_sizes[d] =
          g_host.global_dim[d] + grid->comm_dim(d) * 2 * g_host.margin[d];
    }

    array_of_global_sizes[Grid::Dim] = block_size;

    for (int d = 0; d < Grid::Dim; ++d) {
      array_of_sizes[d] = array_of_global_sizes[d];
      array_of_subsizes[d] = g_host.dim_with_margin[d];
      array_of_starts[d] =
          g_host.start[d] + grid->comm_coord(d) * 2 * g_host.margin[d];
    }

    CATCH_MPI_ERROR(MPI_Type_create_subarray(
        tensor_size, array_of_sizes, array_of_subsizes, array_of_starts,
        MPI_ORDER_C, real_type, &view_type_));

    // MPI_Type_create_darray(grid->comm_size()
    //                          grid->comm_rank(),
    //                          tensor_size,
    //                          array_of_global_sizes,
    //                          const int array_of_distribs[],
    //                          const int array_of_dargs[],
    //                          const int array_of_psizes[],
    //                          int order,
    //                          MPI_Datatype oldtype,
    //                          MPI_Datatype *newtype)

    CATCH_MPI_ERROR(MPI_Type_commit(&view_type_));

    int size_d[2];
    MPI_Type_size(interior_subarray_type_, &size_d[0]);
    MPI_Type_size(view_type_, &size_d[1]);

    if (grid->comm_rank() == 0) {
      printf("(%d,%d)\n", array_of_global_sizes[0], array_of_global_sizes[1]);
    }

    printf("[%d] (%d, %d), -> size=%d,%d\n", grid->comm_rank(),
           array_of_subsizes[0], array_of_subsizes[1], size_d[0], size_d[1]);
  }
};

} // namespace sgrid

#endif // SGRID_RAW_IO_HPP
