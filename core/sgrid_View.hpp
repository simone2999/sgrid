#ifndef SGRID_VIEW_HPP
#define SGRID_VIEW_HPP

#include <algorithm>
#include <complex>
#include <initializer_list>
#include <string>
#include <type_traits>
#include "sgrid_Base.hpp"

#ifdef SGRID_WITH_KOKKOS
////////////////////////////////////////////////////////////////////////////

#ifndef PDELAB_CPU_ONLY
#ifdef KOKKOS_ENABLE_CUDA
#define PDELAB_USE_GPU
#endif
#endif  // PDELAB_CPU_ONLY

#include "sgrid_Utils.hpp"

#include <Kokkos_Complex.hpp>
#include <Kokkos_Core.hpp>

#define SGRID_INLINE_FUNCTION KOKKOS_INLINE_FUNCTION
#define SGRID_FUNCTION KOKKOS_FUNCTION
#define SGRID_LAMBDA KOKKOS_LAMBDA

namespace sgrid {

    using Kokkos::create_mirror_view;
    using Kokkos::deep_copy;
    using Kokkos::fence;
    using Kokkos::finalize;
    using Kokkos::initialize;
    using Kokkos::parallel_for;
    using Kokkos::parallel_reduce;

    using Kokkos::MDRangePolicy;
    using Kokkos::RangePolicy;
    using Kokkos::Rank;
    using Kokkos::View;

    using Kokkos::complex;

    using complex_double_t = Kokkos::complex<double>;
    using complex_float_t = Kokkos::complex<float>;

    SGRID_DEFINE_MPI_TYPE(complex_double_t, MPI_DOUBLE_COMPLEX)
    SGRID_DEFINE_MPI_TYPE(complex_float_t, MPI_COMPLEX)

    /////////////////////////////////////////

#ifdef KOKKOS_ENABLE_OPENMP
    using HostMemorySpace = Kokkos::HostSpace;
    using HostExecutionSpace = Kokkos::OpenMP;
#else  // Serial
    using HostMemorySpace = Kokkos::HostSpace;
    using HostExecutionSpace = Kokkos::Serial;
#endif

#ifdef PDELAB_USE_GPU
    using DeviceMemorySpace = Kokkos::CudaSpace;
    using DeviceExecutionSpace = Kokkos::Cuda;
#else
    using DeviceMemorySpace = HostMemorySpace;
    using DeviceExecutionSpace = HostExecutionSpace;
#endif

    /////////////////////////////////////////

}  // namespace sgrid

////////////////////////////////////////////////////////////////////////////
#else  // SGRID_WITH_KOKKOS
////////////////////////////////////////////////////////////////////////////
// Mock protoype. TODO reproduce simplified functionalites of Kokkos
// #error "Not supported yet!"

#define SGRID_INLINE_FUNCTION inline
#define SGRID_FUNCTION
#define SGRID_LAMBDA [=]

namespace sgrid {

    class HostMemorySpace {};
    class DeviceMemorySpace {};

    class HostExecutionSpace {};
    class DeviceExecutionSpace {};

    using complex_double_t = std::complex<double>;
    using complex_float_t = std::complex<float>;

    template <typename T>
    using complex = std::complex<T>;

#ifdef SGRID_WITH_KOKKOS
    template <typename T, typename MemorySpace, typename... Args>
    using View = ::Kokkos::View<T, MemorySpace, Args...>;
#else

    template <typename T, typename MemorySpace, typename... Args>
    class View {
    public:
        using HostMirror = View;
        
        // Handle both pointer types (int*) and array types (int[N])
        using value_type = typename std::conditional<
            std::is_array_v<T>,
            typename std::remove_extent_t<T>,
            typename std::remove_pointer_t<T>
        >::type;
        
        using pointer_type = value_type*;

        View() : ptr_(nullptr), size_(0), owns_memory_(false) {}

        // Constructor with name - no size parameter needed for array types
        View(const std::string &name) : ptr_(nullptr), size_(1), owns_memory_(true) {
            if constexpr (std::is_array_v<T>) {
                // For array types like int[3], allocate a single instance
                ptr_ = new value_type[std::extent_v<T>]();
                size_ = std::extent_v<T>;
            }
        }

        // Constructor with name and size (for pointer types like double*)
        template <typename... CArgs>
        View(const std::string &name, size_t size) : ptr_(nullptr), size_(size), owns_memory_(true) {
            if constexpr (std::is_pointer_v<T> || std::is_array_v<T>) {
                if (size > 0) {
                    ptr_ = new value_type[size]();  // Zero-initialize
                }
            }
        }

        template <typename... CArgs>
        View(CArgs &&...) : ptr_(nullptr), size_(0), owns_memory_(false) {}

        ~View() {
            if (owns_memory_ && ptr_ != nullptr) {
                delete[] ptr_;
            }
        }

        // Copy constructor - shares ownership semantics like Kokkos
        View(const View &other) : ptr_(other.ptr_), size_(other.size_), owns_memory_(false) {
            // Shallow copy - views share data
        }

        // Assignment operator - shares ownership semantics like Kokkos
        View &operator=(const View &other) {
            if (this != &other) {
                // Clean up if we own the memory
                if (owns_memory_ && ptr_ != nullptr) {
                    delete[] ptr_;
                }
                // Shallow copy - share the data
                ptr_ = other.ptr_;
                size_ = other.size_;
                owns_memory_ = false;  // Don't take ownership of copied data
            }
            return *this;
        }

        // Move constructor
        View(View &&other) noexcept 
            : ptr_(other.ptr_), size_(other.size_), owns_memory_(other.owns_memory_) {
            other.ptr_ = nullptr;
            other.size_ = 0;
            other.owns_memory_ = false;
        }

        // Move assignment operator
        View &operator=(View &&other) noexcept {
            if (this != &other) {
                // Clean up our own memory
                if (owns_memory_ && ptr_ != nullptr) {
                    delete[] ptr_;
                }
                // Take over the other's resources
                ptr_ = other.ptr_;
                size_ = other.size_;
                owns_memory_ = other.owns_memory_;
                // Leave other in valid state
                other.ptr_ = nullptr;
                other.size_ = 0;
                other.owns_memory_ = false;
            }
            return *this;
        }

        inline value_type &operator[](const int i) { return ptr_[i]; }
        inline value_type &operator[](const int i) const { return ptr_[i]; }  // Return mutable reference (Kokkos behavior)

        inline value_type &operator()(const int i) { return ptr_[i]; }
        inline value_type &operator()(const int i) const { return ptr_[i]; }  // Return mutable reference (Kokkos behavior)

        pointer_type data() { return ptr_; }
        pointer_type data() const { return ptr_; }  // Return non-const pointer even from const method (Kokkos behavior)

        size_t size() const { return size_; }
        size_t extent(int) const { return size_; }
        size_t span() const { return size_; }

        bool is_allocated() const { return ptr_ != nullptr; }

    private:
        pointer_type ptr_;
        size_t size_;
        bool owns_memory_;
    };

    template <typename... Args>
    class RangePolicy {};
    template <typename... Args>
    class MDRangePolicy {
    public:
        MDRangePolicy(std::initializer_list<int> start, std::initializer_list<int> end) {}
        
        template <typename... CArgs>
        MDRangePolicy(CArgs &&...) {}
        using point_type = int[3];
    };
    template <int Dim>
    class Rank {};

    template <typename Dest, typename Src>
    void deep_copy(Dest &&dest, Src &&src) {
        if constexpr (std::is_arithmetic_v<std::remove_reference_t<Src>>) {
            // Fill destination with scalar value
            auto size = dest.size();
            auto ptr = dest.data();
            for (size_t i = 0; i < size; ++i) {
                ptr[i] = src;
            }
        } else {
            // Copy between views
            auto size = dest.size() < src.size() ? dest.size() : src.size();
            std::copy(src.data(), src.data() + size, dest.data());
        }
    }

    template <class T>
    inline T create_mirror_view(T &&obj) {
        return std::forward<T>(obj);
    }

    template <typename... Args>
    void parallel_for(Args...) {}
    template <typename... Args>
    void parallel_reduce(Args...) {}

    inline void initialize(int, char **) {}
    inline void finalize() {}
    inline void fence() {}

////////////////////////////////////////////////////////////////////////////
#endif
}  // namespace sgrid

#endif  // SGRID_WITH_KOKKOS

#endif  // SGRID_VIEW_HPP