// srslen.hpp -- hand-authored port of oracle/fortran/srslen.prm.
//
// Maximum series-length parameters for the X13cpp port. These mirror the Fortran
// PARAMETER values exactly; the generated core/prm/gen/srslen.hpp is produced by
// tools/prm2hpp.py and must agree with this file.
//
//   PSP    maximum length of a seasonal period               = 12
//   PFCST  maximum number of forecasts            (10*PSP)    = 120
//   PYR1   maximum number of years in the series              = 65
//   POBS   maximum length of the series           (PYR1*PSP)  = 780
//   PYRS   max years in series + back/forecasts   (PYR1+20)   = 85
//   PLEN   max series length + back & forecasts   (POBS+2*PFCST) = 1020
//   PSRSCR (series title character count)                     = 79
//   PTD    number of trading-day factor types                 = 28
#ifndef X13_PRM_SRSLEN_HPP
#define X13_PRM_SRSLEN_HPP

namespace x13 {
namespace prm {

inline constexpr int PSP    = 12;
inline constexpr int PFCST  = 10 * PSP;          // 120
inline constexpr int PYR1   = 65;
inline constexpr int POBS   = PYR1 * PSP;        // 780
inline constexpr int PYRS   = PYR1 + 20;         // 85
inline constexpr int PLEN   = POBS + (2 * PFCST);// 1020
inline constexpr int PSRSCR = 79;
inline constexpr int PTD    = 28;

}  // namespace prm
}  // namespace x13

#endif  // X13_PRM_SRSLEN_HPP
