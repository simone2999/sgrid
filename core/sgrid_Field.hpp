#ifndef SGRID_FIELD_HPP
#define SGRID_FIELD_HPP

#include "sgrid_Grid.hpp"
#include "sgrid_Halo.hpp"
#include "sgrid_RawIO.hpp"
#include <fstream>
#include <memory>

namespace sgrid {

enum StencilType { CROSS_STENCIL = 0, STAR_STENCIL = 1 };

template <class Grid_> class Field {
public:
  using Grid = Grid_;
  using Real = typename Grid::Real;
  using LocalOrdinal = typename Grid::LocalOrdinal;
  using GlobalOrdinal = typename Grid::GlobalOrdinal;

  using ViewDevice = Kokkos::View<Real *, DeviceMemorySpace>;
  using HostMirror = typename ViewDevice::HostMirror;

  using SideHalo = sgrid::SideHalo<Field>;
  using EdgeHalo = sgrid::EdgeHalo<Field>;
  using NodeHalo = sgrid::NodeHalo<Field>;
  using IO = sgrid::RawIO<Field>;

  explicit Field(const std::string &name, const std::shared_ptr<Grid> &grid,
                 int block_size = 1,
                 const StencilType stencil_type = CROSS_STENCIL)
      : name_(name), grid_(grid), block_size_(block_size),
        stencil_type_(stencil_type) {}

  void allocate_on_device() {
    auto &&grid_host = grid_->view_host();

    field_device_.grid_ = grid_->view_device();

    field_device_.data_ =
        ViewDevice(name_, grid_host.data_size() * block_size_);

    field_device_.block_size_ = block_size_;
  }

  void allocate_on_host() {
    assert(!field_device_.empty());

    field_host_.grid_ = grid_->view_host();
    field_host_.data_ = Kokkos::create_mirror_view(field_device_.data_);
    field_host_.block_size_ = block_size_;
  }

  void allocate() {
    allocate_on_device();
    allocate_on_host();
  }

  void synch_device_to_host() {
    if (field_host_.empty()) {
      allocate_on_host();
    }

    Kokkos::deep_copy(field_host_.data_, field_device_.data_);
  }

  void synch_host_to_device() {
    assert(!field_host_.empty());
    Kokkos::deep_copy(field_device_.data_, field_host_.data_);
  }

  template <class LocalGrid, class View> class LocalField {
  public:
    // Only for scalar fields
    template <typename... Args>
    KOKKOS_INLINE_FUNCTION Real &ref(Args... args) const {
      LocalOrdinal node = grid_.node_idx(args...);
      return data_[node];
    }

    template <typename... Args>
    KOKKOS_INLINE_FUNCTION Real &operator()(Args... args) const {
      return ref(args...);
    }
    template <typename... Args>
    KOKKOS_INLINE_FUNCTION Real *block(Args... args) const {
      LocalOrdinal node = grid_.node_idx(args...);
      return &data_[node * block_size_];
    }

    KOKKOS_INLINE_FUNCTION Real *p_block(const LocalOrdinal *idx) {
      auto node = grid_.p_node_idx(idx);
      return &data_[node * block_size_];
    }

    // Only for scalar fields
    KOKKOS_INLINE_FUNCTION Real &p_ref(const LocalOrdinal *idx) const {
      assert(block_size_ == 1);
      return data_[grid_.p_node_idx(idx)];
    }

    KOKKOS_INLINE_FUNCTION bool empty() const { return block_size_ == -1; }

    void set(const Real value) { Kokkos::deep_copy(data_, value); }

    inline View data() { return data_; }
    inline Real *ptr() { return &data_[0]; }

    LocalGrid grid_;
    View data_;
    int block_size_{-1};
  };

  using LocalFieldDevice =
      LocalField<typename Grid::LocalGridDevice, ViewDevice>;

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
    synch_device_to_host();

    if (halos_.empty()) {
      init_halos();
    }

    for (auto &h : halos_) {
      h->exchange();
    }

    synch_host_to_device();
  }

  std::shared_ptr<Grid> grid() { return grid_; }

  inline int block_size() const { return block_size_; }

private:
  std::string name_;
  std::shared_ptr<Grid> grid_;
  int block_size_{1};
  StencilType stencil_type_{CROSS_STENCIL};

  LocalFieldDevice field_device_;
  LocalFieldHost field_host_;

  std::vector<std::unique_ptr<Halo>> halos_;

  void init_halos() {
    halos_.clear();
    for (int d = 0; d < Grid::Dim; ++d) {
      auto side = std::make_unique<SideHalo>(*this);

      if (side->init(d)) {
        halos_.push_back(std::move(side));
      }
    }

    if (stencil_type_ == STAR_STENCIL) {
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
  }
};

} // namespace sgrid

#endif // SGRID_FIELD_HPP
