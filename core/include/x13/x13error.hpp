// x13error.hpp -- error state + exception type for the X13cpp port.
//
// The original Fortran signals fatal conditions by setting the LOGICAL Lfatal
// in COMMON /fcnerr/ (see error.cmn) and unwinding via RETURN chains. The port
// keeps that flag in ErrorState (part of X13Context) and, where a hard stop is
// warranted, throws X13Error.
#ifndef X13_X13ERROR_HPP
#define X13_X13ERROR_HPP

#include <stdexcept>
#include <string>

namespace x13 {

// Mirrors COMMON /fcnerr/ Lfatal.
struct ErrorState {
    bool lfatal = false;
};

class X13Error : public std::runtime_error {
public:
    explicit X13Error(const std::string& msg) : std::runtime_error(msg) {}
    explicit X13Error(const char* msg) : std::runtime_error(msg) {}
};

} // namespace x13

#endif // X13_X13ERROR_HPP
