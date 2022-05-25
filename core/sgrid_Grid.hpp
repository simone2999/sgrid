#ifndef SGRID_GRID_HPP
#define SGRID_GRID_HPP

#include "sgrid_Base.hpp"
#include "sgrid_Utils.hpp"
#include "sgrid_View.hpp"

#include <mpi.h>
#include <cstdio>
#include <type_traits>
#include <vector>

// https://wgropp.cs.illinois.edu/courses/cs598-s16/lectures/lecture32.pdf

namespace sgrid {

    template <typename Real_, int Dim_>
    class Grid {
    public:
        using GlobalOrdinal = ptrdiff_t;
        using LocalOrdinal = int;
        using Real = Real_;
        static constexpr int Dim = Dim_;

        using ViewHost = sgrid::View<Real *, HostMemorySpace>;
        using ViewDevice = sgrid::View<Real *, DeviceMemorySpace>;
        using IntD = sgrid::View<LocalOrdinal[Dim], HostMemorySpace>;

        using MDRangeDevice = sgrid::MDRangePolicy<sgrid::Rank<Dim>, DeviceExecutionSpace>;

        using MDRangeHost = sgrid::MDRangePolicy<sgrid::Rank<Dim>, HostExecutionSpace>;

        static constexpr int dim() { return Dim; }

        template <class MemorySpace, class ExecutionSpace>
        class LocalGrid {
        public:
            using GView = sgrid::View<GlobalOrdinal[Dim], MemorySpace>;
            using LView = sgrid::View<LocalOrdinal[Dim], MemorySpace>;
            using MDRange = sgrid::MDRangePolicy<sgrid::Rank<Dim>, ExecutionSpace>;
            using MDRangeSlice = sgrid::MDRangePolicy<sgrid::Rank<Dim - 1>, ExecutionSpace>;

            using Range = sgrid::RangePolicy<ExecutionSpace>;

            template <typename FromMemSpace, typename FromExecutionSpace>
            void deep_copy(const LocalGrid<FromMemSpace, FromExecutionSpace> &from) {
                sgrid::deep_copy(global_dim, from.global_dim);
                sgrid::deep_copy(start, from.start);
                sgrid::deep_copy(margin, from.margin);
                sgrid::deep_copy(dim, from.dim);
                sgrid::deep_copy(dim_with_margin, from.dim_with_margin);
            }

            MDRange md_range_with_ghosts() {
                typename MDRange::point_type start, end;

                for (int d = 0; d < Dim; ++d) {
                    start[d] = 0;
                    end[d] = dim[d] + 2 * margin[d];
                }

                return MDRange(start, end);
            }

            MDRange md_range() {
                typename MDRange::point_type start, end;

                for (int d = 0; d < Dim; ++d) {
                    start[d] = margin[d];
                    end[d] = dim[d] + margin[d];
                }

                return MDRange(start, end);
            }

            MDRangeSlice md_range_slice(int plane) const {
                typename MDRangeSlice::point_type start, end;

                for (int d = 0, k = 0; d < Dim; ++d) {
                    if (d == plane) continue;

                    start[k] = margin[d];
                    end[k] = dim[d] + margin[d];

                    ++k;
                }

                return MDRangeSlice(start, end);
            }

            MDRangeSlice md_range_slice_with_ghosts(int plane) const {
                typename MDRangeSlice::point_type start, end;

                for (int d = 0, k = 0; d < Dim; ++d) {
                    if (d == plane) continue;

                    start[k] = 0;
                    end[k] = dim[d] + 2 * margin[d];

                    ++k;
                }

                return MDRangeSlice(start, end);
            }

            void init(const bool use_margins) {
                global_dim = GView("global_dim");
                start = GView("start");

                margin = LView("margin");
                dim = LView("dim");

                if (use_margins) {
                    sgrid::deep_copy(margin, 1);
                } else {
                    sgrid::deep_copy(margin, 0);
                }

                dim_with_margin = LView("dim_with_margin");
            }

            SGRID_INLINE_FUNCTION LocalOrdinal data_size() const {
                LocalOrdinal ret = 1;

                for (int d = 0; d < Dim; ++d) {
                    ret *= dim_with_margin[d];
                }

                return ret;
            }

            // Comodity, but prioritize p_node_idx for more generic codes
            template <typename... Args>
            SGRID_INLINE_FUNCTION int node_idx(Args... args) const {
                static_assert(sizeof...(Args) == Dim, "Number of arguments must be the same as Dim of grid!");
                return tensor_idx(dim_with_margin.data(), args...);
            }

            SGRID_INLINE_FUNCTION GlobalOrdinal global_coord(const int d, const LocalOrdinal local_coord) const {
                return start[d] + local_coord - margin[d];
            }

            SGRID_INLINE_FUNCTION LocalOrdinal p_node_idx(const LocalOrdinal *idx) const {
                int stride = 1;
                int ret = 0;
                for (int d = Dim - 1; d >= 0; d--) {
                    ret += idx[d] * stride;
                    stride *= dim_with_margin[d];
                }

                return ret;
            }

            // Global indexing
            GView global_dim;
            GView start;

            // Local indexing
            LView margin;
            LView dim;
            LView dim_with_margin;
        };

        using LocalGridHost = LocalGrid<HostMemorySpace, HostExecutionSpace>;
        using LocalGridDevice = LocalGrid<DeviceMemorySpace, DeviceExecutionSpace>;

        MDRangeHost md_range_with_ghosts() { return grid_host_.md_range_with_ghosts(); }

        MDRangeHost md_range() { return grid_host_.md_range(); }

        inline GlobalOrdinal n_nodes() const {
            GlobalOrdinal ret = 1;
            for (int d = 0; d < Dim; ++d) {
                ret *= grid_host_.global_dim[d];
            }

            return ret;
        }

        inline LocalOrdinal n_owned_nodes() const {
            LocalOrdinal ret = 1;
            for (int d = 0; d < Dim; ++d) {
                ret *= grid_host_.dim[d];
            }

            return ret;
        }

        auto md_range_slice(int plane) const { return grid_host_.md_range_slice(plane); }

        auto md_range_slice_with_ghosts(int plane) const { return grid_host_.md_range_slice_with_ghosts(plane); }

        void check_dims() const {
            if (comm_rank() == 0) {
                bool valid = true;
                for (int d = 0; d < Dim; ++d) {
                    if (grid_host_.global_dim[d] < proc_dims_[d]) {
                        valid = false;

                        fprintf(stderr,
                                "Invalid problem dimensions for coordinate (%d). Condition "
                                "%ld (dim) >= %d (proc dim) not respected!\n",
                                d,
                                grid_host_.global_dim[d],
                                proc_dims_[d]);
                    }
                }

                if (!valid) {
                    MPI_Abort(raw_comm(), -1);
                }
            }
        }

        void init(MPI_Comm standard_comm,
                  const std::vector<GlobalOrdinal> &dim,
                  std::vector<int> periods = {},
                  std::vector<int> proc_dims = {},
                  const bool use_margins = true) {
            if (periods.empty()) {
                periods.resize(Dim, 0);
            }

            if (proc_dims.empty()) {
                proc_dims.resize(Dim, 0);
            }

            grid_host_.init(use_margins);
            grid_device_.init(use_margins);

            coords_ = IntD("coords");
            proc_dims_ = IntD("proc_dims");
            periods_ = IntD("periods");

            for (int d = 0; d < Dim; ++d) {
                grid_host_.global_dim[d] = dim[d];
            }

            int size;
            MPI_Comm_size(standard_comm, &size);

            CATCH_MPI_ERROR(MPI_Dims_create(size, Dim, proc_dims.data()));

            CATCH_MPI_ERROR(MPI_Cart_create(standard_comm, Dim, proc_dims.data(), periods.data(), 1, &comm_));

            CATCH_MPI_ERROR(MPI_Cart_get(comm_, Dim, proc_dims.data(), periods.data(), coords_.data()));

            int cart_rank;
            MPI_Comm_rank(comm_, &cart_rank);

            for (int d = 0; d < Dim; ++d) {
                LocalOrdinal temp = grid_host_.global_dim[d] / proc_dims[d];
                LocalOrdinal modulo = grid_host_.global_dim[d] % proc_dims[d];
                grid_host_.dim[d] = temp + (coords_[d] < modulo);
                grid_host_.start[d] = temp * coords_[d] + std::min(modulo, LocalOrdinal(coords_[d]));

                grid_host_.dim_with_margin[d] = grid_host_.dim[d] + 2 * grid_host_.margin[d];
            }

            grid_device_.deep_copy(grid_host_);

            for (int d = 0; d < Dim; ++d) {
                proc_dims_[d] = proc_dims[d];
                periods_[d] = periods[d];
            }

            check_dims();
        }

        void dims(int rank, LocalOrdinal *dims) const {
            int coords[Dim];
            CATCH_MPI_ERROR(MPI_Cart_coords(comm_, rank, Dim, coords));

            for (int d = 0; d < Dim; ++d) {
                LocalOrdinal temp = grid_host_.global_dim[d] / proc_dims_[d];
                LocalOrdinal modulo = grid_host_.global_dim[d] % proc_dims_[d];
                dims[d] = temp + (coords[d] < modulo);
            }
        }

        void starts_and_dims(int rank, GlobalOrdinal *starts, LocalOrdinal *dims) const {
            int coords[Dim];
            CATCH_MPI_ERROR(MPI_Cart_coords(comm_, rank, Dim, coords));

            for (int d = 0; d < Dim; ++d) {
                LocalOrdinal temp = grid_host_.global_dim[d] / proc_dims_[d];
                LocalOrdinal modulo = grid_host_.global_dim[d] % proc_dims_[d];
                dims[d] = temp + (coords[d] < modulo);
                starts[d] = temp * coords[d] + std::min(modulo, LocalOrdinal(coords[d]));
            }
        }

        template <typename... Args>
        int neigh(Args... args) {
            static_assert(sizeof...(Args) == Dim, "Number of arguments must be the same as Dim of grid!");

            int coords[Dim];
            unpack_to_array(coords, 0, args...);

            return p_neigh(coords);
        }

        int p_neigh(int *coords) const {
            for (int d = 0; d < Dim; ++d) {
                coords[d] += coords_[d];

                if ((coords[d] < 0 || coords[d] >= proc_dims_[d]) && periods_[d] == 0) {
                    return MPI_PROC_NULL;
                }
            }

            int r = 0;
            CATCH_MPI_ERROR(MPI_Cart_rank(comm_, coords, &r));
            return r;
        }

        int comm_rank() const {
            int rank;
            MPI_Comm_rank(comm_, &rank);
            return rank;
        }

        int comm_size() const {
            int size;
            MPI_Comm_size(comm_, &size);
            return size;
        }

        int comm_coord(int d) const { return coords_[d]; }
        int comm_dim(int d) const { return proc_dims_[d]; }
        bool is_periodic(int d) const { return periods_[d]; }

        bool has_margins() const 
        {
            return grid_host_.margin[0] > 0;
        }

        int shift(int direction, int disp) const {
            int rank_source = comm_rank();
            int rank_dest = MPI_PROC_NULL;
            CATCH_MPI_ERROR(MPI_Cart_shift(comm_, direction, disp, &rank_source, &rank_dest));

            return rank_dest;
        }

        bool can_shift(int direction, int disp) const { return (shift(direction, disp) != MPI_PROC_NULL); }

        void describe() {
            int rank, size;

            MPI_Comm_rank(comm_, &rank);
            MPI_Comm_size(comm_, &size);

            for (int r = 0; r < size; ++r) {
                if (r == rank) {
                    printf("=============================\n");
                    printf("[%d]\t", rank);
                    printf("\ncoords=(%d, %d, %d)\n", coords_[0], coords_[1], coords_[2]);

                    if (r == 0) {
                        printf("\nproc_dims\n");
                        for (int d = 0; d < Dim; ++d) {
                            printf("%d\t", proc_dims_[d]);
                        }
                        printf("\n");
                    } else {
                        printf("\n");
                    }

                    printf("\n(start,dim)\n");
                    for (int d = 0; d < Dim; ++d) {
                        printf("(%ld, %d)\t", grid_host_.start[d], grid_host_.dim[d]);
                    }

                    printf("\n");
                    printf("=============================\n");
                }
                MPI_Barrier(comm_);
            }
        }

        LocalGridHost &view_host() { return grid_host_; }
        LocalGridDevice &view_device() { return grid_device_; }

        inline MPI_Comm raw_comm() const { return comm_; }

    private:
        MPI_Comm comm_;
        IntD coords_;
        IntD proc_dims_;
        IntD periods_;
        LocalGridHost grid_host_;
        LocalGridDevice grid_device_;
    };

}  // namespace sgrid

#endif  // SGRID_GRID_HPP