// farray.hpp -- Fortran-semantics array wrappers for the X13cpp port.
//
// Provides 1-based, column-major array types that mirror Fortran storage so a
// faithful port can index exactly as the original .f code does.
//
//   farray1<T,N>        fixed-size 1-D, default lower bound 1
//   farray1lb<T,LB,N>   fixed-size 1-D with explicit lower bound (e.g. 0:PTF)
//   farray2<T,M,N>      fixed-size 2-D, column-major, 1-based
//   farray1d<T>         dynamic 1-D (lower bound configurable at construction)
//   farray2d<T>         dynamic 2-D, column-major
//
// All expose .data() returning a contiguous pointer for BLAS/LAPACK-style calls
// and raw C interop. operator() uses Fortran (i) / (i,j) 1-based indexing.
#ifndef X13_FARRAY_HPP
#define X13_FARRAY_HPP

#include <array>
#include <cstddef>
#include <vector>
#include <stdexcept>
#include <string>

namespace x13 {

// ---------------------------------------------------------------------------
// Bounds checking: enabled unless X13_NO_BOUNDS_CHECK is defined.
// ---------------------------------------------------------------------------
namespace detail {
[[noreturn]] inline void farray_oob(long idx, long lo, long hi, const char* what) {
    throw std::out_of_range(std::string("farray ") + what + " index " +
                            std::to_string(idx) + " out of [" +
                            std::to_string(lo) + "," + std::to_string(hi) + "]");
}
inline void check(long idx, long lo, long hi, const char* what) {
#ifndef X13_NO_BOUNDS_CHECK
    if (idx < lo || idx > hi) farray_oob(idx, lo, hi, what);
#else
    (void)idx; (void)lo; (void)hi; (void)what;
#endif
}
} // namespace detail

// ---------------------------------------------------------------------------
// Fixed-size 1-D array with explicit lower bound.
// ---------------------------------------------------------------------------
template <class T, long LB, std::size_t N>
class farray1lb {
public:
    using value_type = T;
    static constexpr long lbound = LB;
    static constexpr long ubound = LB + static_cast<long>(N) - 1;
    static constexpr std::size_t size_v = N;

    farray1lb() : d_{} {}

    T& operator()(long i) {
        detail::check(i, LB, ubound, "1d");
        return d_[static_cast<std::size_t>(i - LB)];
    }
    const T& operator()(long i) const {
        detail::check(i, LB, ubound, "1d");
        return d_[static_cast<std::size_t>(i - LB)];
    }

    T* data() noexcept { return d_.data(); }
    const T* data() const noexcept { return d_.data(); }
    constexpr std::size_t size() const noexcept { return N; }
    void fill(const T& v) { d_.fill(v); }

    // 0-based raw access (storage order), for interop convenience.
    T& raw(std::size_t k) { return d_[k]; }
    const T& raw(std::size_t k) const { return d_[k]; }

private:
    std::array<T, N> d_;
};

// Common case: lower bound 1.
template <class T, std::size_t N>
using farray1 = farray1lb<T, 1, N>;

// ---------------------------------------------------------------------------
// Fixed-size 2-D array, column-major (Fortran), 1-based on both dims.
// Storage: element (i,j) at (i-1) + (j-1)*M.
// ---------------------------------------------------------------------------
template <class T, std::size_t M, std::size_t N>
class farray2 {
public:
    using value_type = T;
    static constexpr std::size_t rows = M;
    static constexpr std::size_t cols = N;

    farray2() : d_{} {}

    T& operator()(long i, long j) {
        detail::check(i, 1, static_cast<long>(M), "2d row");
        detail::check(j, 1, static_cast<long>(N), "2d col");
        return d_[static_cast<std::size_t>((i - 1) + (j - 1) * static_cast<long>(M))];
    }
    const T& operator()(long i, long j) const {
        detail::check(i, 1, static_cast<long>(M), "2d row");
        detail::check(j, 1, static_cast<long>(N), "2d col");
        return d_[static_cast<std::size_t>((i - 1) + (j - 1) * static_cast<long>(M))];
    }

    T* data() noexcept { return d_.data(); }
    const T* data() const noexcept { return d_.data(); }
    constexpr std::size_t size() const noexcept { return M * N; }
    void fill(const T& v) { d_.fill(v); }

private:
    std::array<T, M * N> d_;
};

// ---------------------------------------------------------------------------
// Fixed-size 2-D array with explicit per-dimension lower bounds (Fortran
// LB1:UB1, LB2:UB2), column-major, e.g. a COMMON array declared (0:N, M).
// ---------------------------------------------------------------------------
template <class T, long LB1, std::size_t M, long LB2, std::size_t N>
class farray2lb {
public:
    using value_type = T;
    static constexpr std::size_t rows = M;
    static constexpr std::size_t cols = N;

    farray2lb() : d_{} {}

    T& operator()(long i, long j) {
        detail::check(i, LB1, LB1 + (long)M - 1, "2dlb row");
        detail::check(j, LB2, LB2 + (long)N - 1, "2dlb col");
        return d_[static_cast<std::size_t>((i - LB1) + (j - LB2) * static_cast<long>(M))];
    }
    const T& operator()(long i, long j) const {
        detail::check(i, LB1, LB1 + (long)M - 1, "2dlb row");
        detail::check(j, LB2, LB2 + (long)N - 1, "2dlb col");
        return d_[static_cast<std::size_t>((i - LB1) + (j - LB2) * static_cast<long>(M))];
    }

    T* data() noexcept { return d_.data(); }
    const T* data() const noexcept { return d_.data(); }
    constexpr std::size_t size() const noexcept { return M * N; }
    void fill(const T& v) { d_.fill(v); }

private:
    std::array<T, M * N> d_;
};

// ---------------------------------------------------------------------------
// Dynamic 1-D array with configurable lower bound.
// ---------------------------------------------------------------------------
template <class T>
class farray1d {
public:
    using value_type = T;

    farray1d() : lb_(1) {}
    explicit farray1d(std::size_t n, long lb = 1) : d_(n), lb_(lb) {}

    void resize(std::size_t n, long lb = 1) { d_.assign(n, T{}); lb_ = lb; }

    T& operator()(long i) {
        detail::check(i, lb_, ubound(), "dyn1d");
        return d_[static_cast<std::size_t>(i - lb_)];
    }
    const T& operator()(long i) const {
        detail::check(i, lb_, ubound(), "dyn1d");
        return d_[static_cast<std::size_t>(i - lb_)];
    }

    T* data() noexcept { return d_.data(); }
    const T* data() const noexcept { return d_.data(); }
    std::size_t size() const noexcept { return d_.size(); }
    long lbound() const noexcept { return lb_; }
    long ubound() const noexcept { return lb_ + static_cast<long>(d_.size()) - 1; }
    void fill(const T& v) { std::fill(d_.begin(), d_.end(), v); }

private:
    std::vector<T> d_;
    long lb_;
};

// ---------------------------------------------------------------------------
// Dynamic 2-D array, column-major, 1-based.
// ---------------------------------------------------------------------------
template <class T>
class farray2d {
public:
    using value_type = T;

    farray2d() : m_(0), n_(0) {}
    farray2d(std::size_t m, std::size_t n) : d_(m * n), m_(m), n_(n) {}

    void resize(std::size_t m, std::size_t n) { d_.assign(m * n, T{}); m_ = m; n_ = n; }

    T& operator()(long i, long j) {
        detail::check(i, 1, static_cast<long>(m_), "dyn2d row");
        detail::check(j, 1, static_cast<long>(n_), "dyn2d col");
        return d_[static_cast<std::size_t>((i - 1) + (j - 1) * static_cast<long>(m_))];
    }
    const T& operator()(long i, long j) const {
        detail::check(i, 1, static_cast<long>(m_), "dyn2d row");
        detail::check(j, 1, static_cast<long>(n_), "dyn2d col");
        return d_[static_cast<std::size_t>((i - 1) + (j - 1) * static_cast<long>(m_))];
    }

    T* data() noexcept { return d_.data(); }
    const T* data() const noexcept { return d_.data(); }
    std::size_t rows() const noexcept { return m_; }
    std::size_t cols() const noexcept { return n_; }
    std::size_t size() const noexcept { return d_.size(); }
    void fill(const T& v) { std::fill(d_.begin(), d_.end(), v); }

private:
    std::vector<T> d_;
    std::size_t m_, n_;
};

} // namespace x13

#endif // X13_FARRAY_HPP
