// fstring.hpp -- fixed-length, blank-padded CHARACTER*N string (Fortran semantics).
//
//  - Storage is exactly N chars, space-padded on the right.
//  - Assignment from a shorter source pads with blanks; from a longer source
//    truncates to N (Fortran assignment rules).
//  - Comparison ignores trailing blanks and compares as if both operands were
//    blank-extended to equal length (Fortran CHARACTER relational semantics).
//  - str() returns an std::string with trailing blanks removed.
#ifndef X13_FSTRING_HPP
#define X13_FSTRING_HPP

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <array>

namespace x13 {

template <std::size_t N>
class fstring {
public:
    static constexpr std::size_t length = N;

    fstring() { d_.fill(' '); }
    fstring(const char* s) { assign(std::string_view(s)); }               // NOLINT
    fstring(std::string_view s) { assign(s); }                            // NOLINT
    fstring(const std::string& s) { assign(std::string_view(s)); }        // NOLINT

    fstring& operator=(std::string_view s) { assign(s); return *this; }
    fstring& operator=(const char* s) { assign(std::string_view(s)); return *this; }
    fstring& operator=(const std::string& s) { assign(std::string_view(s)); return *this; }

    // Fortran assignment: pad with blanks or truncate to N.
    void assign(std::string_view s) {
        std::size_t n = s.size() < N ? s.size() : N;
        for (std::size_t i = 0; i < n; ++i) d_[i] = s[i];
        for (std::size_t i = n; i < N; ++i) d_[i] = ' ';
    }

    // Fortran-trimmed value (trailing blanks removed).
    std::string str() const {
        std::size_t end = N;
        while (end > 0 && d_[end - 1] == ' ') --end;
        return std::string(d_.data(), end);
    }

    // Raw view including trailing blanks (all N chars).
    std::string_view raw() const { return std::string_view(d_.data(), N); }

    char* data() noexcept { return d_.data(); }
    const char* data() const noexcept { return d_.data(); }
    constexpr std::size_t size() const noexcept { return N; }

    char& operator()(std::size_t i) { return d_[i - 1]; }        // 1-based like Fortran
    const char& operator()(std::size_t i) const { return d_[i - 1]; }

    // Length ignoring trailing blanks (Fortran LEN_TRIM).
    std::size_t len_trim() const {
        std::size_t end = N;
        while (end > 0 && d_[end - 1] == ' ') --end;
        return end;
    }

private:
    std::array<char, N> d_;
};

// --- Fortran relational comparison: ignore trailing blanks, blank-extend -----
namespace detail {
inline int fstr_cmp(std::string_view a, std::string_view b) {
    std::size_t n = a.size() > b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        char ca = i < a.size() ? a[i] : ' ';
        char cb = i < b.size() ? b[i] : ' ';
        if (ca != cb) return (static_cast<unsigned char>(ca) <
                              static_cast<unsigned char>(cb)) ? -1 : 1;
    }
    return 0;
}
} // namespace detail

template <std::size_t N, std::size_t M>
bool operator==(const fstring<N>& a, const fstring<M>& b) {
    return detail::fstr_cmp(a.raw(), b.raw()) == 0;
}
template <std::size_t N, std::size_t M>
bool operator!=(const fstring<N>& a, const fstring<M>& b) { return !(a == b); }
template <std::size_t N, std::size_t M>
bool operator<(const fstring<N>& a, const fstring<M>& b) {
    return detail::fstr_cmp(a.raw(), b.raw()) < 0;
}
template <std::size_t N, std::size_t M>
bool operator>(const fstring<N>& a, const fstring<M>& b) { return b < a; }
template <std::size_t N, std::size_t M>
bool operator<=(const fstring<N>& a, const fstring<M>& b) { return !(b < a); }
template <std::size_t N, std::size_t M>
bool operator>=(const fstring<N>& a, const fstring<M>& b) { return !(a < b); }

template <std::size_t N>
bool operator==(const fstring<N>& a, std::string_view b) {
    return detail::fstr_cmp(a.raw(), b) == 0;
}
template <std::size_t N>
bool operator==(std::string_view a, const fstring<N>& b) { return b == a; }
template <std::size_t N>
bool operator!=(const fstring<N>& a, std::string_view b) { return !(a == b); }
template <std::size_t N>
bool operator!=(std::string_view a, const fstring<N>& b) { return !(b == a); }

} // namespace x13

#endif // X13_FSTRING_HPP
