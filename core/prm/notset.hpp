// notset.hpp -- hand-authored port of oracle/fortran/notset.prm.
//
// Sentinel "value not set" constants used throughout X-13ARIMA-SEATS to mark
// namelist/spec inputs the user did not supply.
//
//   CNOTST '?'         character variable not input to namelist
//   NOTSET -32767      integer variable not input
//   DNOTST -999.0d0    double-precision value not input
//
// (The single-precision SNOTST = -999.0 is commented out in the Fortran and is
// intentionally omitted here.)
#ifndef X13_PRM_NOTSET_HPP
#define X13_PRM_NOTSET_HPP

namespace x13 {
namespace prm {

inline constexpr char   CNOTST = '?';
inline constexpr int    NOTSET = -32767;
inline constexpr double DNOTST = -999.0;

}  // namespace prm
}  // namespace x13

#endif  // X13_PRM_NOTSET_HPP
