// armafl.hpp -- the exact ARMA filter cluster (armafl.f and its two stateful
// helpers intgpg.f / exctma.f). Unlike the pointer-only leaves in armafilt.hpp,
// these routines read and write the ex-COMMON model state, so they take
// X13Context& ctx first (model_cmn + mdldat_cmn). They drive the ported
// pointer leaves (ratpos/ratneg/mltpos/arflt, uconv/xpand/euclid, dppfa/logdet/
// dsolve, xprmx, copy/setdp/scrmlt).
#ifndef X13_REGARIMA_ARMAFL_HPP
#define X13_REGARIMA_ARMAFL_HPP

#include "common/x13context.hpp"

namespace x13 {

// intgpg.f: initialize G'G (the MA-innovation cross-product), Cholesky-factor it
// into mdldat.chlgpg, and set mdldat.lndtcv to its log-determinant. nextma is
// the number of pi/psi weights to expand (input); info is the dppfa status
// (output, >0 => not positive definite). No-op with lndtcv=0 when !model.lma.
void intgpg(X13Context& ctx, int nextma, int& info);

// exctma.f: exact MA filter a = [1/theta(B)] z applied in place to the matrix a
// (nc columns, nelta elements on entry -- nelta is grown by the initial-value
// block and returned). Uses chlgpg from a prior intgpg call for the initial
// values w* = -(G'G)^-1 G'Hw. nata is a's declared length (bound only). Writes
// model.nopr as a side effect. No-op unless model.nopr>0.
void exctma(X13Context& ctx, int nc, double* a, int& nelta, int nata);

// armafl.f: the exact ARMA filter. Filters the nr x nc matrix mata in place to
// whitened residuals (returned length na = nelta/nc). When linit is set (and the
// model has AR or MA terms) it (re)initializes G'G via intgpg, the ARMA ACVs
// (uconv/euclid/xpand), the D matrix and chol(var(w_p|z)) in chlvwp, adding the
// AR determinant term to lndtcv. lckrts gates the chkrts invertibility check
// (info=PINVER on failure; PGPGER/PACFER/PVWPER for the init sub-steps). nextma
// is SAVEd across calls in ctx.saved. nata is mata's declared length.
void armafl(X13Context& ctx, int nr, int nc, bool linit, bool lckrts,
            double* mata, int& na, int nata, int& info);

}  // namespace x13

#endif  // X13_REGARIMA_ARMAFL_HPP
