#ifndef SGRID_FIELD_HPP
#define SGRID_FIELD_HPP

#include <filesystem>
#include <memory>
#include "sgrid_Grid.hpp"
#include "sgrid_Halo.hpp"
#include "sgrid_RawIO.hpp"
#include "sgrid_SerialPeriodicHalo.hpp"

namespace sgrid {

    enum StencilType { STAR_STENCIL = 0, BOX_STENCIL = 1 };

    template <class Grid_, typename ValueType_ = typename Grid_::Real>
    class Field {
    public:
        using Grid = Grid_;
        using Real = typename Grid::Real;
        using ValueType = ValueType_;
        using LocalOrdinal = typename Grid::LocalOrdinal;
        using GlobalOrdinal = typename Grid::GlobalOrdinal;

        using ViewDevice = sgrid::View<ValueType *, DeviceMemorySpace>;
        using HostMirror = typename ViewDevice::HostMirror;

        using SideHalo = sgrid::SideHalo<Field>;
        using EdgeHalo = sgrid::EdgeHalo<Field>;
        using NodeHalo = sgrid::NodeHalo<Field>;
        using IO = sgrid::RawIO<Field>;

        explicit Field(const std::string &name,
                       const std::shared_ptr<Grid> &grid,
                       int block_size = 1,
                       const StencilType stencil_type = STAR_STENCIL)
            : name_(name), grid_(grid), block_size_(block_size), stencil_type_(stencil_type) {}

        void allocate_on_device() {
            auto &&grid_host = grid_->view_host();

            field_device_.grid_ = grid_->view_device();

            field_device_.data_ = ViewDevice(name_, grid_host.data_size() * block_size_);

            field_device_.block_size_ = block_size_;
        }

        std::string get_value_type() {
            if (std::is_same<ValueType, long>::value) {
                return "long";
            } else if (std::is_same<ValueType, int>::value) {
                return "int";
            } else if (std::is_same<ValueType, double>::value) {
                return "double";
            } else if (std::is_same<ValueType, char>::value) {
                return "char";
            } else if (std::is_same<ValueType, float>::value) {
                return "float";
            } else if (std::is_same<ValueType, sgrid::complex<double>>::value) {
                return "complex";
            } else {
                assert(false);
                return "";
            }
        }

        void allocate_on_host() {
            assert(!field_device_.empty());

            field_host_.grid_ = grid_->view_host();
            field_host_.data_ = sgrid::create_mirror_view(field_device_.data_);
            field_host_.block_size_ = block_size_;
        }

        void ensure_view_host() {
            if (field_host_.empty()) {
                allocate_on_host();
            }
        }

        void allocate() {
            allocate_on_device();
            allocate_on_host();
        }

        void synch_device_to_host() {
            ensure_view_host();

            sgrid::deep_copy(field_host_.data_, field_device_.data_);
        }

        void synch_host_to_device() {
            assert(!field_host_.empty());
            sgrid::deep_copy(field_device_.data_, field_host_.data_);
        }

        template <class LocalGrid, class View>
        class LocalField {
        public:
            // Only for scalar fields
            template <typename... Args>
            SGRID_INLINE_FUNCTION ValueType &ref(Args... args) const {
                LocalOrdinal node = grid_.node_idx(args...);
                return data_[node];
            }

            template <typename... Args>
            SGRID_INLINE_FUNCTION ValueType &operator()(Args... args) const {
                return ref(args...);
            }
            template <typename... Args>
            SGRID_INLINE_FUNCTION ValueType *block(Args... args) const {
                LocalOrdinal node = grid_.node_idx(args...);
                return &data_[node * block_size_];
            }

            SGRID_INLINE_FUNCTION ValueType *p_block(const LocalOrdinal *idx) const {
                auto node = grid_.p_node_idx(idx);
                return &data_[node * block_size_];
            }

            // Only for scalar fields
            SGRID_INLINE_FUNCTION ValueType &p_ref(const LocalOrdinal *idx) const {
                assert(block_size_ == 1);
                return data_[grid_.p_node_idx(idx)];
            }

            SGRID_INLINE_FUNCTION bool empty() const { return block_size_ == -1; }

            void set(const ValueType value) { sgrid::deep_copy(data_, value); }

            inline View data() { return data_; }
            inline ValueType *ptr() { return &data_[0]; }

            LocalGrid grid_;
            View data_;
            int block_size_{-1};
        };

        using LocalFieldDevice = LocalField<typename Grid::LocalGridDevice, ViewDevice>;

        using LocalFieldHost = LocalField<typename Grid::LocalGridHost, HostMirror>;

        LocalFieldHost &view_host() {
            assert(!field_host_.empty());
            return field_host_;
        }

        LocalFieldDevice &view_device() { return field_device_; }

        void write(const std::string &path) {
            IO io(*this);
            io.set_output_path(path);
            io.write();
        }

        // Symplistic synchronization
        void exchange_halos() {
            assert(grid()->has_margins());

            synch_device_to_host();

            if (halos_.empty()) {
                init_halos();
            }

            for (auto &h : halos_) {
                h->exchange();
            }

            synch_host_to_device();
        }

        void init_halos() {
            halos_.clear();
            for (int d = 0; d < Grid::Dim; ++d) {
                auto side = std::make_unique<SideHalo>(*this);

                if (side->init(d)) {
                    halos_.push_back(std::move(side));
                }
            }

            if (stencil_type_ == BOX_STENCIL) {
                if (Grid::Dim == 3) {
                    for (int d = 0; d < Grid::Dim; ++d) {
                        auto side = std::make_unique<EdgeHalo>(*this);

                        if (side->init(d)) {
                            halos_.push_back(std::move(side));
                        }
                    }
                }

                if (Grid::Dim >= 2) {
                    auto node = std::make_unique<NodeHalo>(*this);

                    if (node->init()) {
                        halos_.push_back(std::move(node));
                    }
                }
            }

            for (int d = 0; d < Grid::Dim; ++d) {
                if (grid_->is_periodic(d) && grid_->comm_dim(d) == 1) {
                    halos_.push_back(std::make_unique<SerialPeriodicSideHalo<Field>>(*this, d));
                }
            }
        }

        std::shared_ptr<Grid> grid() { return grid_; }

        inline int block_size() const { return block_size_; }

        inline StencilType stencil_type() const { return stencil_type_; }

        inline size_t n_bytes() const {
            auto g_host = grid_->view_host();

            size_t count = 1;

            for (int d = 0; d < Grid::Dim; ++d) {
                size_t n = g_host.global_dim[d];
                size_t halos = grid_->comm_dim(d) * g_host.margin[d];
                count *= n + halos;
            }

            return sizeof(ValueType) * count * block_size_;
        }

    private:
        std::string name_;
        std::shared_ptr<Grid> grid_;
        int block_size_{1};
        StencilType stencil_type_{STAR_STENCIL};

        LocalFieldDevice field_device_;
        LocalFieldHost field_host_;

        std::vector<std::unique_ptr<Halo>> halos_;
    };

}  // namespace sgrid

#endif  // SGRID_FIELD_HPP
