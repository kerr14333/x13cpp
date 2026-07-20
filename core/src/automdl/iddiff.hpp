// iddiff.hpp -- iddiff.f: TRAMO/Gomez-Maravall unit-root identification of the
// regular and seasonal differencing orders (d, D). Iteratively builds trial
// (p 0 0)(P 0 0) models, HR-estimates them (amdest), inspects the AR roots
// (chkrt1), and accumulates difference orders until the roots move off the unit
// circle -- then optionally tests the mean for significance. The chosen orders
// return in idr/ids.
#ifndef X13_AUTOMDL_IDDIFF_HPP
#define X13_AUTOMDL_IDDIFF_HPP

#include "common/x13context.hpp"

namespace x13 {

// prterr.f: report an estimation error. All message printing is deferred with
// the rest of the print engine; the only non-print effect (kept here) is the
// unknown-error branch zeroing the fit so the caller's !Convrg guard fires.
void prterr(X13Context& ctx, int nefobs, bool lauto);

// iddiff.f: identify the differencing orders. idr/ids are the maxdiff limits on
// input and the chosen regular/seasonal orders on output. trnsrs is the
// transformed series; a/na the ARMA residual scratch; frstry the first Xy
// element (regvar output); imu/lmu the mean-check control (lmu returns whether a
// mean is warranted); svldif/lsumm gate deferred summary prints.
void iddiff(X13Context& ctx, int& idr, int& ids, double* trnsrs, int& nefobs,
            int& frstry, double* a, int& na, int imu, bool& lmu, bool svldif,
            int lsumm);

}  // namespace x13
#endif  // X13_AUTOMDL_IDDIFF_HPP
