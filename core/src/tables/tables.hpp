// tables.hpp -- table-output subsystem (M2).
//
// One free function per ported Fortran routine; every routine that touches
// ex-COMMON state takes X13Context& ctx first. This covers the save-file path
// (savtbl.f / punch.f / dtoc.f) that produces the library's numeric data
// surface and its byte-identical /rdb save-file text.
#ifndef X13_TABLES_TABLES_HPP
#define X13_TABLES_TABLES_HPP

#include <string>

#include "common/x13context.hpp"

namespace x13 {

// dtoc.f: format a double into Str at 1-based Ipos using the run's save format
// (savcmn: Svfmt/Svsize), advancing Ipos past the field.
void dtoc(X13Context& ctx, double dnum, std::string& str, int& ipos);

// savtbl.f: capture a table (positions I1..Na of Avec, dated from Begdat/Sp) as
// a save table -- both numerically (SaveState) and as byte-identical /rdb text.
//   Itbl   1-based table pointer (mdltbl.i); its 3-char extension is tbxdic(Itbl)
//   base   output base name (Cursrs/Serno) used for the <base>.<ext> filename
//   label  series name (Serno) used in the header label; nser = its length
void savtbl(X13Context& ctx, int itbl, const int* begdat, int i1, int na, int sp,
            const double* avec, const std::string& base, const std::string& label,
            int nser);

// punch.f: percentage/plain copy then savtbl (Lgraf graphics dir path omitted).
void punch(X13Context& ctx, const double* x, int mfda, int mlda, int itbl,
           const int* begdat, int sp, const std::string& base,
           const std::string& label, int nser, bool lpct);

// savmtx.f: capture a matrix table (Date + one column per title) as a save
// file. xy is the flat row-major [X:y] matrix (element (row,col) at
// ncxy*(row-1)+col); columns 1..nttl are written (the y column is excluded).
void savmtx(X13Context& ctx, int itbl, const int* begxy, int sp, const double* xy,
            int nrxy, int ncxy, const char* ttlstr, const int* ttlptr, int nttl,
            const std::string& base);

}  // namespace x13

#endif  // X13_TABLES_TABLES_HPP
