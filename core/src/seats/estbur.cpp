// estbur.cpp -- see estbur.hpp.
#include "seats/estbur.hpp"

#include <algorithm>
#include <cmath>

#include "gen/model.hpp"
#include "gen/srslen.hpp"
#include "regarima/armafl.hpp"
#include "seats/seatsfact.hpp"  // mltsol
#include "seats/seatspoly.hpp"  // conv

namespace x13 {
namespace {

// armafl()-derived TRAILING exact-ML residuals for `series` (forward: z
// itself; backward: bz, the plain reversal -- differencing a reversed
// series naturally reproduces CALCFX's kd=(-1)^(d+bd) sign flip on the
// DATA, session 11).
//
// SESSION 14 FIX #1 (seed count): this used to return only the SINGLE last
// residual (a(Na)). Re-reading FCAST's own end-extension recursion
// (ansub1.f:2183-2201) closely: `sz = za - sum_{j=1}^{Qstar} thstar(j)*
// a(k-j)` at forecast step i (k=na+i) references a(na+i-j) for j=1..Qstar --
// which is HISTORICAL (a real, nonzero residual, not the "future residuals
// are 0" convention) whenever i<=j, i.e. for the first (Qstar-1) extension
// steps. Concretely: step i=1 needs ALL of a(Na), a(Na-1), ...,
// a(Na-Qstar+1) -- not just a(Na). For `unrate_seats` (Qstar_f=q+bq*mq=1)
// this coincides with the old single-residual behavior (nothing changes).
// For `payems_seats` (Qstar_f=2) it does NOT: a(Na-1) was silently treated
// as 0 by the old code instead of its real, nonzero value -- a genuine
// under-seeding bug for any Qstar_f>=2 spec, found while investigating
// session 13's payems s12/s13 "residual-precision floor".
//
// SESSION 14 FIX #2 (the `fac` scaling was WRONG for this use, not merely
// imprecise): the old code applied fcnar()'s own recipe (regarima/
// estimate.cpp:162-192) -- armafl(linit=true) then, for exact-MA
// estimation, scale by fac=exp(lndtcv/2/dnefob) -- copied from the REGULAR
// ARIMA-estimation residual path on the assumption it was the generic
// "exact-ML residual" recipe. Ground-truthed against the oracle directly
// this session (temporary analts.f instrumentation dumping the ACTUAL
// a(Na)/Detpri CALCFX hands to FCAST, reverted after -- see
// tools/seats_scope.md session 14): `Detpri` (ansub1.f:1425/1467,
// `detbnp=det**(0.5/np)`, np=Nw-Pstar) turned out to be EXACTLY
// `exp(lndtcv/2/dnefob)` -- i.e. EXACTLY this function's own `fac` --
// bit-for-bit identical to 17 significant digits (`dnefob==Nw==775`,
// `np==Nw-Pstar==773` for unrate_seats happen to make `fac`/`Detpri`
// numerically indistinguishable here, but they're conceptually the SAME
// quantity, not a coincidence). But CALCFX's OWN internal round trip
// MULTIPLIES its raw STEP8 residual by Detpri (ansub1.f:1469) and then
// analts.f's caller immediately DIVIDES it back out (analts.f:2386/2829,
// `a(i)=a(i)/Detpri`) before ever handing `a` to FCAST -- session 10 already
// found this cancels for CALCFX's OWN path. The value FCAST actually
// consumes is therefore the RAW (unscaled) residual, NOT the
// Detpri/fac-multiplied one. Applying fcnar()'s `fac` scaling here (which
// is the right thing to do for the REGULAR estimation-residual use fcnar()
// itself needs it for) was accidentally reintroducing the exact factor
// CALCFX's own round trip cancels back out -- confirmed decisively by
// substituting the oracle's dumped ground-truth a(Na) into this function's
// output: `s12`/`s13` dropped from a ~2.78e-8 worst-case gap to ~4.8e-15
// (double-precision noise) at EVERY one of unrate_seats' 776 dates, and
// dividing this function's own (formerly scaled) output by its own `fac`
// reproduced that EXACT oracle-dumped value to the last bit (diff==0.0).
// Fix: DROP the `fac` scaling entirely for this SEATS/FCAST-seeding use --
// use armafl()'s raw residual, negated only (session 12's still-unexplained
// but re-confirmed sign convention, unaffected by this fix).
//
// out[0] = a(Na), out[1] = a(Na-1), ..., out[count-1] = a(Na-count+1)
// (all negated, UNSCALED); out[k]=0 if Na-k would be <1 (not enough
// history -- shouldn't happen for any corpus spec's count=Qstar_f, but
// guarded).
// Faithful port of CALCFX (ansub1.f:964-1471), STEP 3B-8, returning the
// TRAILING exact-ML residuals FCAST seeds itself with. This is the
// Morf-Sidhu-Kailath constrained-least-squares residual computation SEATS
// uses -- NOT regARIMA's armafl (Ansley/Kalman). For a well-behaved
// (invertible, single-lag) MA the two agree to the last bit (unrate_seats,
// Qstar=1: armafl matched the oracle exactly, session 14). But for a
// near-NON-invertible multi-lag seasonal MA (payems_fixed: bth ~ -0.978, MA
// root modulus 1.022) the exact-ML startup a(1..Qstar) propagates far enough
// down the series that armafl's residuals diverge ~4.5% from CALCFX's at the
// tail (the FCAST seeds), because armafl's GLS residual convention differs
// from CALCFX's raw STEP-8 innovation in the startup. Reproducing CALCFX
// exactly closes that gap.
//
// The general Pstar>0 (AR present) branch IS ported: STEP 1B/2 build the
// stationary AR filter Phist and u = (1 - sum Phist B^j) Wd (ansub1.f:5006-
// 5007), np = Nw - Pstar. This matters for near-non-invertible seasonal MAs
// combined with AR (e.g. payems_ar2-seats, seasonal MA ~0.985), where armafl's
// startup residuals diverge ~1.6e-5 at the tail; the faithful CALCFX seeds
// close it to double-precision noise. Airline-family (pstar=0) collapses to
// u == Wd exactly as before.
//
// out[k] = -a(n-k) (n=nw+Qstar), matching armafl_last_residuals' negation
// convention that fcast_extend expects.
bool calcfx_last_residuals(const SeatsModelOrders& mo,
                           const SeatsCanonicalDenoms& cd,
                           const std::vector<double>& series, int count,
                           std::vector<double>& out, double center = 0.0) {
    out.assign(std::max(count, 0), 0.0);
    if (count <= 0) return true;
    const int Q = mo.q, Bq = mo.bq, Mq = mo.mq;
    const int Qstar = Q + Bq * Mq;
    if (Qstar <= 0 || cd.qstar < Qstar) return false;

    // ---- Difference series -> Wd. Faithful to the oracle's order
    // (analts.f:1933-1947): SEASONAL (1-B^mq)^bd FIRST, then REGULAR (1-B)^d.
    // The two commute algebraically but the FP subtraction tree differs, so
    // the order must match for bit-exactness. Forward difference keeps the
    // most-recent value last. ----
    std::vector<double> w(series);  // 0-indexed working copy
    int len = static_cast<int>(w.size());
    for (int rep = 0; rep < mo.bd; ++rep) {
        for (int t = 0; t < len - Mq; ++t) w[t] = w[t + Mq] - w[t];
        len -= Mq;
    }
    for (int rep = 0; rep < mo.d; ++rep) {
        for (int t = 0; t < len - 1; ++t) w[t] = w[t + 1] - w[t];
        --len;
    }
    const int nw = len;

    // MEAN-CORRECT the differenced series (analts.f:1972 Wd(j)=Wd(j)-wmDifXL)
    // when imean!=0: the exact-ML residuals CALCFX seeds are computed on the
    // CENTERED differenced series. `center` is wm for the forward series and
    // kd*wm (kd=(-1)^(d+bd)) for the backward -- the oracle reverses the
    // already-centered Wd with sign kd (analts.f:2793-2804), so the backward
    // Wd's mean is kd*wm. 0 when imean=0 (inert).
    if (center != 0.0)
        for (int t = 0; t < nw; ++t) w[t] -= center;

    // ---- STEP 1B/2 (ansub1.f:5006-5007): the STATIONARY AR filter. Pstar
    // here is the STATIONARY AR order (p+bp*mq), NOT cd.pstar (which folds in
    // the differencing). Phist = the stationary AR polynomial's coefficients
    // in the u = (1 - sum Phist B^j) Wd convention -- i.e. Phist(j) = -arp(j)
    // where arp = (1 + sum phi B^i)(1 + sum bphi B^{k*mq}) is the true-sign AR
    // polynomial build_bphist forms (arp(j) == -phist(j), ansub1.f:2116-2121).
    // For pstar=0 (airline family) this collapses to u == Wd, np == nw. ----
    const int Pstar = mo.p + mo.bp * Mq;
    std::vector<double> Phist(Pstar + 1, 0.0);
    if (Pstar > 0) {
        std::vector<double> nn(mo.p + 1, 0.0);
        nn[0] = 1.0;
        for (int i = 0; i < mo.p; ++i) nn[i + 1] = mo.phi[i];
        std::vector<double> ss(mo.bp * Mq + 1, 0.0);
        ss[0] = 1.0;
        for (int k = 0; k < mo.bp; ++k) ss[(k + 1) * Mq] = mo.bphi[k];
        std::vector<double> arp(Pstar + 1, 0.0);
        for (int i = 0; i < static_cast<int>(nn.size()); ++i)
            for (int j = 0; j < static_cast<int>(ss.size()); ++j)
                arp[i + j] += nn[i] * ss[j];
        for (int j = 1; j <= Pstar; ++j) Phist[j] = -arp[j];
    }
    const int np = nw - Pstar;
    if (np <= Qstar) return false;
    const int n = np + Qstar;
    // 1-indexed u[1..np] = (1 - sum Phist B^j) Wd, aligned to end at Wd(nw).
    std::vector<double> u(np + 1, 0.0);
    for (int i = 1; i <= np; ++i) {
        double sum = w[i + Pstar - 1];  // Wd(i+Pstar), 0-indexed
        for (int j = 1; j <= Pstar; ++j)
            sum -= Phist[j] * w[i + Pstar - j - 1];  // Wd(i+Pstar-j)
        u[i] = sum;
    }

    // ---- Thstar (STEP 3B, ansub1.f:1179-1197). The SEATS CALCFX operates on
    // the CANONICAL/approximated model -- the SEATS-CAPPED MA (bth snapped to
    // the xl invertibility bound), NOT the raw estimate. cd.thstar is that
    // capped MA polynomial numerator [1, th1, ..., -0.99@mq, ...]; CALCFX's
    // Thstar is its negation (its recursion uses `a = u + Thstar*a`, i.e.
    // Thstar = -(polynomial coefficients above the leading 1)). Verified vs an
    // oracle CALCFX dump: Thstar(1)=-cd.thstar(1), Thstar(mq)=-cd.thstar(mq)
    // (=+0.99), Thstar(mq+1)=-cd.thstar(mq+1). ----
    std::vector<double> Thstar(Qstar + 1, 0.0);
    for (int j = 1; j <= Qstar; ++j) Thstar[j] = -cd.thstar[j];

    // ---- ith / nith: indices of non-zero Thstar lags (ansub1.f:1036-1055).
    // nith = Q*Bq+Q+Bq can exceed Qstar when Q>=Mq (nith-Qstar = Bq*(Q+1-Mq));
    // the oracle sizes ith(maxTH) independently of Qstar, so size to hold nith
    // even though the corpus never hits that (Q<=1). ----
    const int nith = Q * Bq + Q + Bq;
    std::vector<int> ith(std::max(Qstar, nith) + 1, 0);
    {
        int idx = 0;
        for (int i = 1; i <= Q; ++i) ith[++idx] = i;
        int k = Q;
        for (int i = 1; i <= Bq; ++i) {
            ++k;
            int j = i * Mq;
            ith[k] = j;
            for (int l = 1; l <= Q; ++l) ith[++k] = j + l;
        }
    }

    const double small = 10.0e-10, ceps = 1.0e-12;
    // ---- STEP 4 init (ansub1.f:1234-1247). ----
    std::vector<double> a(n + 1, 0.0), v(Qstar + 1, 0.0);
    std::vector<double> b((Qstar + 1) * (Qstar + 1), 0.0);
    auto B = [&](int i, int j) -> double& { return b[i * (Qstar + 1) + j]; };
    // am is n x Qstar (1-indexed).
    std::vector<double> amv((n + 1) * (Qstar + 1), 0.0);
    auto AM = [&](int i, int j) -> double& { return amv[i * (Qstar + 1) + j]; };
    std::vector<int> jcol(Qstar + 1, 0);
    for (int i = 1; i <= Qstar; ++i) { B(i, i) = 1.0; AM(i, i) = 1.0; }
    const int q1 = Qstar + 1;

    // ---- STEP 5 constrained residuals (ansub1.f:1258-1265). ----
    for (int l = q1; l <= n; ++l) {
        double sum1 = u[l - Qstar];
        for (int i = 1; i <= nith; ++i) {
            int j = ith[i];
            sum1 += Thstar[j] * a[l - j];
        }
        a[l] = sum1;
    }
    // ---- Build am columns (ansub1.f:1269-1325). ----
    int nq2 = Q + 2, nq1 = (Q == 0) ? Bq : Bq + 1;
    bool cols_done = false;
    for (int i1 = 1; i1 <= nq1 && !cols_done; ++i1) {
        int k = (i1 - 1) * Mq;
        if (Q != 0) {
            int i = 0;
            for (int i2 = 1; i2 <= Q; ++i2) {
                i = k + i2;
                jcol[i] = 0;
                for (int l = q1; l <= n; ++l) {
                    double sum2 = 0.0;
                    for (int j = 1; j <= nith; ++j) {
                        int j1 = ith[j];
                        sum2 += Thstar[j1] * AM(l - j1, i);
                    }
                    if (std::abs(sum2) <= small) sum2 = 0.0;
                    AM(l, i) = sum2;
                }
            }
            if (i == Qstar) { cols_done = true; break; }
        }
        // (Q+1)th column: non-zeros every Mq rows.
        int i2 = Q + 1, i = k + i2;
        jcol[i] = 1;
        for (int l = q1; l <= n; l += Mq) {
            double sum2 = 0.0;
            for (int j = 1; j <= Bq; ++j) {
                int j1 = j * Mq;
                sum2 += Thstar[j1] * AM(l - j1, i);
            }
            AM(l, i) = sum2;
        }
        // Remaining columns: shift the (Q+1)th column (ansub1.f:1315-1325).
        for (i2 = nq2; i2 <= Mq; ++i2) {
            i = k + i2;
            jcol[i] = i2 - Q;
            for (int l = q1 + i2 - nq2 + 1; l <= n; l += Mq)
                AM(l, i) = AM(l - 1, i - 1);
        }
    }
    // ---- FORM K'K (b) and vector v (ansub1.f:1329-1365). ----
    for (int i = 1; i <= Qstar; ++i) {
        for (int j = 1; j <= i; ++j) {
            if (jcol[i] + jcol[j] > 0) {
                int l;
                if (jcol[i] * jcol[j] > 0) {
                    if (jcol[i] != jcol[j]) continue;  // goto 15
                    l = Qstar + jcol[i];
                } else {
                    l = Qstar + jcol[i] + jcol[j];
                }
                double sum3 = B(j, i);
                for (; l <= n; l += Mq) sum3 += AM(l, i) * AM(l, j);
                B(j, i) = sum3;
            } else {
                double sum3 = B(j, i);
                for (int l = q1; l <= n; ++l) sum3 += AM(l, i) * AM(l, j);
                B(j, i) = sum3;
            }
        }
        if (jcol[i] > 0) {
            for (int l = Qstar + jcol[i]; l <= n; l += Mq)
                v[i] += AM(l, i) * a[l];
        } else {
            for (int l = q1; l <= n; ++l) v[i] += AM(l, i) * a[l];
        }
    }
    // ---- STEP 6 inverse of b (ansub1.f:1371-1417). ----
    if (Qstar > 1) {
        double det = 1.0, e = std::pow(10.0, -Qstar - 3);
        for (int i = 1; i <= Qstar; ++i) {
            det *= B(i, i);
            if (det < e) return false;  // DETB zero/negative
            double g = 1.0 / B(i, i);
            B(i, i) = g;
            for (int j = 1; j <= Qstar; ++j) {
                if (j < i) {
                    if (std::abs(B(j, i)) >= ceps) {
                        double h = g * B(j, i);
                        for (int k = j; k <= Qstar; ++k) {
                            if (k < i) B(j, k) += h * B(k, i);
                            else if (k != i) B(j, k) -= h * B(i, k);
                        }
                        B(j, i) = -h;
                    }
                } else if (j != i) {
                    if (std::abs(B(i, j)) >= ceps) {
                        double h = g * B(i, j);
                        for (int k = j; k <= Qstar; ++k)
                            B(j, k) -= h * B(i, k);
                        B(i, j) = h;
                    }
                }
            }
        }
        for (int j = 1; j <= Qstar; ++j)
            for (int k = 1; k <= j - 1; ++k) B(j, k) = B(k, j);
    } else {
        B(1, 1) = 1.0 / B(1, 1);
    }
    // ---- STEP 7 ML values of first Qstar a(i) (ansub1.f:1430-1436). ----
    for (int i = 1; i <= Qstar; ++i) {
        double sum = 0.0;
        for (int j = 1; j <= Qstar; ++j) sum -= B(i, j) * v[j];
        a[i] = sum;
    }
    // ---- STEP 8 residuals for i=q1..n (ansub1.f:1451-1461). ----
    for (int i = q1; i <= n; ++i) {
        double sum = u[i - Qstar];
        for (int j = 1; j <= nith; ++j) {
            int j1 = ith[j];
            sum += Thstar[j1] * a[i - j1];
        }
        a[i] = sum;
    }
    // ---- Seeds: out[k] = -a(n-k) (negation convention, STEP 9 Detpri
    // round-trips to a no-op on the forward path, analts.f:2386). ----
    for (int k = 0; k < count; ++k) {
        int idx = n - k;  // 1-indexed a(n-k)
        out[k] = (idx >= 1) ? -a[idx] : 0.0;
    }
    return true;
}

bool armafl_last_residuals(X13Context& ctx, const std::vector<double>& series,
                            int count, std::vector<double>& out) {
    out.assign(std::max(count, 0), 0.0);
    if (count <= 0) return true;
    constexpr int PA = prm::PLEN + 2 * prm::PORDER;
    std::vector<double> a(PA, 0.0);
    int nr = static_cast<int>(series.size());
    if (nr <= 0 || nr > PA) return false;
    for (int i = 0; i < nr; ++i) a[i] = series[i];
    int na = 0, info = 0;
    armafl(ctx, nr, 1, /*linit=*/true, /*lckrts=*/false, a.data(), na, PA,
           info);
    if (info != 0 || na <= 0) return false;
    for (int k = 0; k < count; ++k) {
        int idx = na - 1 - k;  // 0-indexed a(Na-k)
        out[k] = (idx >= 0) ? -a[idx] : 0.0;
    }
    return true;
}

// bphist = phist*(1-B)^d*(1-B^mq)^bd, FCAST's own construction
// (ansub1.f:2113-2148). Returns bpstar (bphist's length). The phist AR
// polynomial fill (P/Bp>0) is ported; a strict no-op for the airline-family
// corpus (p=bp=0). The general-shape (p>0/bp>0) decomposition is bit-exact +
// gated (the *_ar2-seats / *_sar-seats specs); the CALCFX general-branch
// residual seed is ported too (calcfx_last_residuals Pstar>0). Only imean!=0
// remains unported (guarded-fatal in run_seats). See
// tools/seats_general_scope.md.
int build_bphist(const SeatsModelOrders& mo, std::vector<double>& bphist) {
    bphist.assign(64, 0.0);
    bphist[1] = 1.0;
    int pstar_f = mo.p + mo.bp * mo.mq;
    // phist = the full AR polynomial (1 - sum phi B^i)(1 - sum bphi B^{k*mq}),
    // ansub1.f:2116-2121 sets bphist(i+1) = -phist(i). mo.phi/mo.bphi are the
    // standardized AR coeffs in the arp(B) = 1 + sum(mo.phi)B^i factor convention
    // (verified: pure-AR(2) arp=[1, phi0, phi1] == the oracle's -phist), so the
    // full AR polynomial coefficient arp(i) IS -phist(i) = bphist(i+1) directly.
    if (pstar_f > 0) {
        std::vector<double> nn(mo.p + 1, 0.0);
        nn[0] = 1.0;
        for (int i = 0; i < mo.p; ++i) nn[i + 1] = mo.phi[i];
        std::vector<double> ss(mo.bp * mo.mq + 1, 0.0);
        ss[0] = 1.0;
        for (int k = 0; k < mo.bp; ++k) ss[(k + 1) * mo.mq] = mo.bphi[k];
        std::vector<double> arp(pstar_f + 1, 0.0);
        for (int i = 0; i < static_cast<int>(nn.size()); ++i)
            for (int j = 0; j < static_cast<int>(ss.size()); ++j)
                arp[i + j] += nn[i] * ss[j];
        for (int i = 1; i <= pstar_f; ++i) bphist[i + 1] = arp[i];
    }
    int bpstar = pstar_f + 1;
    if (mo.d != 0) {
        for (int rep = 0; rep < mo.d; ++rep) {
            bphist[bpstar + 1] = 0.0;
            for (int j = 1; j <= bpstar; ++j) {
                int k = bpstar - j + 2;
                bphist[k] = bphist[k] - bphist[k - 1];
            }
            bpstar += 1;
        }
    }
    if (mo.bd != 0) {
        for (int rep = 0; rep < mo.bd; ++rep) {
            for (int j = 1; j <= mo.mq; ++j) bphist[bpstar + j] = 0.0;
            for (int j = 1; j <= bpstar; ++j) {
                int k = bpstar - j + mo.mq + 1;
                bphist[k] = bphist[k] - bphist[k - mo.mq];
            }
            bpstar += mo.mq;
        }
    }
    bpstar -= 1;
    for (int i = 1; i <= bpstar; ++i) bphist[i] = -bphist[i + 1];
    return bpstar;
}

// FCAST's own forward recursion (ansub1.f:2183-2201). Extends `series`
// (length nz) by `steps` points using the raw (unswitched) model MA
// coefficients cd.thstar[1..qstar_f] and the seed residuals `last_resids`
// (last_resids[m] == a(Na-m), the trailing historical exact-ML residuals --
// session 14: ALL of them are needed when qstar_f>=2, not just a(Na), see
// armafl_last_residuals's comment). `za` is the mean seed (ansub1.f:2166-
// 2180): 0 when imean=0, else wm (Pstar=0) or wm*(1-sum phist) (Pstar>0).
std::vector<double> fcast_extend(const SeatsModelOrders& mo,
                                  const double* thstar,
                                  const std::vector<double>& series,
                                  const std::vector<double>& last_resids,
                                  int steps, double za) {
    std::vector<double> bphist;
    int bpstar = build_bphist(mo, bphist);
    int qstar_f = mo.q + mo.bq * mo.mq;
    int nz_local = static_cast<int>(series.size());
    // aext[qstar_f-m] is a(Na-m) (the seeds, m=0..qstar_f-1); aext[qstar_f+i]
    // (i>=1) is a(Na+i), FCAST's own "future residuals are 0" convention.
    std::vector<double> aext(steps + qstar_f + 1, 0.0);
    for (int m = 0; m < qstar_f && m < static_cast<int>(last_resids.size());
         ++m)
        aext[qstar_f - m] = last_resids[m];
    std::vector<double> zext(steps, 0.0);
    for (int i = 1; i <= steps; ++i) {
        double sz = za;  // mean seed (0 when imean=0)
        for (int j = 1; j <= qstar_f; ++j)
            sz -= thstar[j] * aext[qstar_f + i - j];
        for (int j = 1; j <= bpstar; ++j) {
            int idx = nz_local + i - j;
            double zv = (idx <= nz_local) ? series[idx - 1]
                                           : zext[idx - nz_local - 1];
            sz += bphist[j] * zv;
        }
        zext[i - 1] = sz;
    }
    return zext;
}

}  // namespace

void estbur_historical(X13Context& ctx, const SeatsModelOrders& mo,
                        const SeatsCanonicalDenoms& cd,
                        const SeatsComponentModels& comp,
                        const SeatsOptions& opts, EstburResult& out) {
    out = EstburResult{};
    int nz = ctx.mdldat.nspobs;
    if (nz <= 0) return;

    std::vector<double> z(nz), bz(nz);
    for (int i = 0; i < nz; ++i) z[i] = ctx.series.tsrs(i + 1);
    for (int i = 0; i < nz; ++i) bz[i] = z[nz - 1 - i];  // plain reversal

    // Mean seed (imean!=0). A Constant (PRGTCN) regressor sets Imean=1; SEATS
    // then centers the DIFFERENCED series by wm (analts.f:1955-1979) and adds
    // it back via za in FCAST (ansub1.f:2166-2180) + wmf/wmb in ESTBUR's
    // general branch (ansub3.f:243-244). wm = plain mean of the fully-
    // differenced series over ALL Nw points (crmean default 0). The residual
    // t-value correction (analts.f:2417-2460, wm += first*rmean) never fires:
    // its gate is rtval>ta with ta default 100 (ansub9.f:1626). The
    // demeaned-series d+bd==0 branch (ansub3.f:121-134, zmean=DMEAN(z)) is the
    // separate d=0 gap -- run_seats still fatals mean+d==0; here d+bd>0 so
    // zaf/zab = za and zmean=0.
    //
    // L_IMEAN itself comes from seats_resolve_options (ansub9.f:1072-1080):
    // the Constant-regressor derivation is the DEFAULT, but an explicit
    // seats{imean=yes/no} overrides it in either direction -- the series SEATS
    // decomposes is unchanged (regeff/adjreg never strip the Constant), only
    // whether SEATS models it as having a mean.
    const bool imean = opts.imean != 0;
    // wm = mean of the fully-differenced series (crmean default 0 -> plain mean
    // over all Nw points). Forward CALCFX centers Wd by wm; the backward Wd is
    // the sign-flipped reversal (kd=(-1)^(d+bd)), so its center is kd*wm and
    // its FCAST seed zab = zaf*kd (analts.f:2793-2805). All 0 when imean=0.
    double wm = 0.0, za = 0.0;
    const double kd = ((mo.d + mo.bd) % 2 == 0) ? 1.0 : -1.0;
    if (imean) {
        std::vector<double> wd(z);           // fully-differenced series
        int nw = nz;
        for (int s = 0; s < mo.bd; ++s) {    // seasonal diffs (lag mq)
            nw -= mo.mq;
            for (int j = 0; j < nw; ++j) wd[j] = wd[j + mo.mq] - wd[j];
        }
        for (int s = 0; s < mo.d; ++s) {     // regular diffs (lag 1)
            nw -= 1;
            for (int j = 0; j < nw; ++j) wd[j] = wd[j + 1] - wd[j];
        }
        for (int j = 0; j < nw; ++j) wm += wd[j];
        if (nw > 0) wm /= nw;
        // za = wm*(1 - sum phist), phist = the STATIONARY AR poly coeffs
        // (Pstar = p + bp*mq); za = wm for Pstar==0 (ansub1.f:2168-2180).
        const int Pstar = mo.p + mo.bp * mo.mq;
        double sum_phist = 0.0;
        if (Pstar > 0) {
            std::vector<double> nn(mo.p + 1, 0.0); nn[0] = 1.0;
            for (int i = 0; i < mo.p; ++i) nn[i + 1] = mo.phi[i];
            std::vector<double> ss(mo.bp * mo.mq + 1, 0.0); ss[0] = 1.0;
            for (int k = 0; k < mo.bp; ++k) ss[(k + 1) * mo.mq] = mo.bphi[k];
            std::vector<double> arp(Pstar + 1, 0.0);
            for (int i = 0; i < (int)nn.size(); ++i)
                for (int j = 0; j < (int)ss.size(); ++j) arp[i + j] += nn[i] * ss[j];
            for (int j = 1; j <= Pstar; ++j) sum_phist += -arp[j];  // phist=-arp
        }
        za = wm * (1.0 - sum_phist);
    }

    // pstar/qstar/Totden/Thstr0: cd's own fields DIRECTLY (session 12 --
    // confirmed against an oracle instrumentation dump AND a direct probe
    // that cd.pstar/cd.qstar already equal what ESTBUR needs; no switch).
    int pstar = cd.pstar, qstar = cd.qstar;
    if (pstar <= 0 || qstar <= 0) return;
    int maxpq = std::max(pstar, qstar);

    std::vector<double> thstr0(maxpq + 1, 0.0);
    for (int i = 1; i <= qstar && i - 1 < cd.qstar; ++i)
        thstr0[i] = cd.thstar[i - 1];

    std::vector<double> totden(pstar + 1, 0.0);
    {
        double t0[80] = {};
        int nt0 = 0;
        conv(cd.psi, cd.npsi, cd.chcyc, cd.nchcyc, t0, nt0);
        for (int i = 1; i <= pstar && i <= nt0; ++i) totden[i] = t0[i - 1];
    }

    // ---- System A (ansub3.f:151-174): gt/gs/gc, the 1-sided filter ----
    std::vector<double> am1(60 * 66, 0.0);
    auto AM1 = [&](int i, int j) -> double& {
        return am1[(j - 1) * 60 + (i - 1)];
    };
    for (int i = 1; i <= maxpq; ++i) {
        for (int j = 1; j <= i; ++j) AM1(i, j) = thstr0[i - j + 1];
        int m = maxpq - i + 1;
        for (int j = m; j <= maxpq; ++j) AM1(i, j) += thstr0[maxpq - j + m];
        int k = maxpq - i + 1;
        AM1(i, maxpq + 1) = (k <= comp.nct) ? comp.ct[k - 1] : 0.0;
        AM1(i, maxpq + 2) = (k <= comp.ncs) ? comp.cs[k - 1] : 0.0;
        AM1(i, maxpq + 3) = (k <= comp.ncc) ? comp.cc[k - 1] : 0.0;
    }
    mltsol(am1.data(), maxpq, 3);
    std::vector<double> gt(maxpq + 1, 0.0), gs(maxpq + 1, 0.0),
        gc(maxpq + 1, 0.0);
    for (int i = 1; i <= maxpq; ++i) {
        int k = maxpq - i + 1;
        gt[k] = AM1(i, maxpq + 1);
        gs[k] = AM1(i, maxpq + 2);
        gc[k] = AM1(i, maxpq + 3);
    }
    // ---- The forecast horizon (ansub3.f's `lf`, and the save range). ----
    // Three different lengths, all derived from ONE input, `fh`:
    //   fh   = Nfcst          (ansub9.f:1116 `L_fh=Nfcst`; the l_fh=8 default at
    //                          :1609 is overwritten on every X-13 run, and
    //                          editor.f:387-400 forces Nfcst >= max(12,3*Sp)
    //                          on any seats{} run whatever forecast{maxlead=}
    //                          says);
    //   lf   = fhi            (analts.f:2773-2776) -- what ESTBUR decomposes;
    //   lfor = max(fh,max(8,2*mq))  (sigex.f:426) -- what sigex.f:3631-3636
    //                          punches into the tfd/sfd/afd/yfd tables.
    // They coincide for every corpus spec (fh=36 monthly / 12 quarterly
    // dominates both maxima), so `lfor > lf` -- where the oracle would punch
    // exp(0) padding out of an untouched buffer -- is not reproduced; the
    // save range is clamped to lf and the shortfall recorded here rather than
    // faked.
    const int fh = ctx.extend.nfcst;
    const int qbqmq = mo.q + mo.bq * mo.mq;
    int lf = std::max(fh, qbqmq + std::max(qbqmq, mo.p + mo.bp * mo.mq));
    if (opts.noadmiss == -1)
        lf = std::max(lf, 2 * (mo.p + mo.d + mo.mq * (mo.bp + mo.bd)));
    const int lfor = std::min(lf, std::max(fh, std::max(8, 2 * mo.mq)));
    // ansub3.f:108-116 -- extZ is filled from z over 1..nz+l2.
    int l1 = std::max(maxpq + qstar, 2 * mo.mq);
    l1 = std::max(l1, lf);
    int l2 = std::max(std::min(l1, qstar + maxpq - 2), lf);

    // ---- Forward/backward extension. The BACKWARD one only has to cover the
    // two-sided filter window (`qstar+maxpq-2`); the FORWARD one is also the
    // forecast of z that the whole forecast decomposition is built on, so it
    // runs to nz+l2. Extending it is prefix-stable -- FCAST's recursion is
    // causal, so the first qstar+maxpq-2 points are the same values the
    // historical solve used before this change (verified byte-identical). ----
    int lext = std::max(0, qstar + maxpq - 2);
    int lextf = std::max(lext, l2);
    std::vector<double> zextFwd, zextBwd;
    if (lextf > 0) {
        int qstar_f = mo.q + mo.bq * mo.mq;
        // Both the seed residuals (CALCFX) and the extension recursion (FCAST)
        // operate on the SEATS CANONICAL/approximated model -- the CAPPED MA
        // (cd.thstar, bth snapped to the xl invertibility bound) -- NOT the raw
        // estimate. The oracle passes the same model Thstar to both CALCFX and
        // FCAST (analts.f:2778). (An earlier revision used the uncapped
        // theta_raw here to compensate for armafl's inexact seeds; with the
        // faithful CALCFX seeds that compensation is wrong -- use cd.thstar
        // consistently.)
        std::vector<double> aFwd, aBwd;
        if (!calcfx_last_residuals(mo, cd, z, qstar_f, aFwd, wm) &&
            !armafl_last_residuals(ctx, z, qstar_f, aFwd)) return;
        if (!calcfx_last_residuals(mo, cd, bz, qstar_f, aBwd, kd * wm) &&
            !armafl_last_residuals(ctx, bz, qstar_f, aBwd)) return;
        zextFwd = fcast_extend(mo, cd.thstar, z, aFwd, lextf, za);
        zextBwd = fcast_extend(mo, cd.thstar, bz, aBwd, lext, kd * za);
    }
    auto EXTZ = [&](int idx) -> double {  // 1-indexed
        if (idx <= nz) return z[idx - 1];
        return zextFwd[idx - nz - 1];
    };
    auto BZX = [&](int idx) -> double {
        if (idx <= nz) return bz[idx - 1];
        return zextBwd[idx - nz - 1];
    };

    // ---- Filter application (ansub3.f:196-225) ----
    int n196 = nz + qstar - 1;
    std::vector<double> fyt(n196 + 1, 0.0), byt(n196 + 1, 0.0);
    std::vector<double> fys(n196 + 1, 0.0), bys(n196 + 1, 0.0);
    std::vector<double> fyc(n196 + 1, 0.0), byc(n196 + 1, 0.0);
    for (int i = 1; i <= n196; ++i) {
        double s1 = 0, s2 = 0, s3 = 0, s4 = 0, s5 = 0, s6 = 0;
        for (int j = 1; j <= maxpq; ++j) {
            int m = i + j - 1;
            double ez = EXTZ(m), bzv = BZX(m);
            s1 += gt[j] * ez;
            s2 += gt[j] * bzv;
            s3 += gs[j] * ez;
            s4 += gs[j] * bzv;
            s5 += gc[j] * ez;
            s6 += gc[j] * bzv;
        }
        fyt[i] = s1;
        byt[i] = s2;
        fys[i] = s3;
        bys[i] = s4;
        fyc[i] = s5;
        byc[i] = s6;
    }

    // fxt/bxt/etc need indices up to n+irow (the general branch's OUTPUT
    // STORE loop, ansub3.f:295-302 -- k=n+irow-i+1 for i=1, i.e. k=n+irow),
    // which for qstar>3 EXCEEDS n196+1 (session 12: a real heap-corruption
    // crash on airline_fixed-airline-seats, qstar=14, before this was
    // fixed). n+irow = (nz+qstar-pstar)+(pstar+qstar-2) = nz+2*qstar-2;
    // size generously (the oracle's own fxt(mpkp+np) is far larger than
    // strictly needed too) rather than compute the exact tight bound.
    // (+ lf, because the forecast blocks below continue the same recursions
    // out to Nz+lf.)
    int nbuf = nz + 2 * maxpq + 4 + std::max(0, lf) + qstar;
    std::vector<double> fxt(nbuf, 0.0), bxt(nbuf, 0.0);
    std::vector<double> fxs(nbuf, 0.0), bxs(nbuf, 0.0);
    std::vector<double> fxc(nbuf, 0.0), bxc(nbuf, 0.0);

    if (qstar == 1) {
        // Trivial branch (ansub3.f:226-237) -- not exercised by
        // unrate_seats (qstar=2), kept for completeness.
        for (int j = 1; j <= nz; ++j) {
            fxt[j] = fyt[j];
            bxt[j] = byt[j];
            fxs[j] = fys[j];
            bxs[j] = bys[j];
            fxc[j] = fyc[j];
            bxc[j] = byc[j];
        }
    } else {
        // General branch (ansub3.f:238-341).
        int irow = pstar + qstar - 2;
        // wmf=0.5*zaf, wmb=0.5*zab (ansub3.f:243-244); zaf=za, zab=kd*za
        // (analts.f:2805). za=0 when imean=0 (inert).
        double wmf = 0.5 * za, wmb = 0.5 * kd * za;
        std::vector<double> am2(60 * 66, 0.0);
        auto AM2 = [&](int i, int j) -> double& {
            return am2[(j - 1) * 60 + (i - 1)];
        };
        int n = nz + qstar - pstar;  // REASSIGNED n (ansub3.f:251)
        int iqrow = qstar - 1;
        for (int i = 1; i <= iqrow; ++i) {
            for (int j = 1; j <= pstar; ++j) {
                int m = i + j - 1;
                AM2(i, m) = totden[j];
            }
            AM2(i, irow + 1) = wmf;
            AM2(i, irow + 2) = wmb;
        }
        for (int i = qstar; i <= irow; ++i) {
            for (int j = 1; j <= qstar; ++j) {
                int m = i - j + 1;
                AM2(i, m) = thstr0[j];
            }
            int k = n + irow - i + 1;
            AM2(i, irow + 1) = fyt[k];
            AM2(i, irow + 2) = byt[k];
            AM2(i, irow + 3) = fys[k];
            AM2(i, irow + 4) = bys[k];
            AM2(i, irow + 5) = fyc[k];
            AM2(i, irow + 6) = byc[k];
        }
        mltsol(am2.data(), irow, 6);
        for (int i = 1; i <= irow; ++i) {
            int k = n + irow - i + 1;
            fxt[k] = AM2(i, irow + 1);
            bxt[k] = AM2(i, irow + 2);
            fxs[k] = AM2(i, irow + 3);
            bxs[k] = AM2(i, irow + 4);
            fxc[k] = AM2(i, irow + 5);
            bxc[k] = AM2(i, irow + 6);
        }
        for (int i = 1; i <= n; ++i) {
            int m = n - i + 1;
            double s1 = fyt[m], s2 = byt[m], s3 = fys[m], s4 = bys[m],
                   s5 = fyc[m], s6 = byc[m];
            for (int j = 2; j <= qstar; ++j) {
                int k = m + j - 1;
                s1 -= thstr0[j] * fxt[k];
                s2 -= thstr0[j] * bxt[k];
                s3 -= thstr0[j] * fxs[k];
                s4 -= thstr0[j] * bxs[k];
                s5 -= thstr0[j] * fxc[k];
                s6 -= thstr0[j] * bxc[k];
            }
            fxt[m] = s1;
            bxt[m] = s2;
            fxs[m] = s3;
            bxs[m] = s4;
            fxc[m] = s5;
            bxc[m] = s6;
        }
    }

    // ESTBUR's own trend/sc/cycle/sa/ir are all ADDITIVE, in whatever
    // domain `z` (ctx.series.tsrs) is in -- the model's TRANSFORMED
    // (log-linearized) domain for a log-transform spec, since z log(y)+
    // log(seasonal)+log(cycle)+log(irregular) decomposes additively in log
    // space (session 13: ctx.arima.lam==0.0 for log, ==1.0 for none --
    // confirmed via a probe against payems_seats/unrate_seats).
    std::vector<double> trend_i(nz), cycle_i(nz), sc_i(nz), sa_i(nz), ir_i(nz);
    const bool npsi1 = (cd.npsi == 1);
    for (int i = 1; i <= nz; ++i) {
        trend_i[i - 1] = fxt[i] + bxt[nz - i + 1];
        cycle_i[i - 1] = fxc[i] + bxc[nz - i + 1];
        sc_i[i - 1] = npsi1 ? 0.0 : (fxs[i] + bxs[nz - i + 1]);
    }
    for (int i = 1; i <= nz; ++i) {
        // ansub3.f:537/542/546 -- the SEASONAL case subtracts sc too:
        // ir = z - sc - trend - cycle, sa = z - sc. sc_i==0 when npsi==1
        // (unrate/payems), so this unified form matches the no-seasonal
        // branch (ansub3.f:521/526) there. ir_i feeds the bias2c mean; a
        // missing sc term left airline_seats' bias2c (hence s12/s13) 0.89%
        // off while npsi==1 specs stayed exact.
        ir_i[i - 1] =
            z[i - 1] - sc_i[i - 1] - trend_i[i - 1] - cycle_i[i - 1];
        sa_i[i - 1] = z[i - 1] - sc_i[i - 1];  // isCloseToTD always false here
    }

    // ---- FORECAST SPAN (ansub3.f:353-524 + :651-672) ----------------------
    // Continue the same one-sided recursions past Nz. Three blocks -- seasonal
    // (npsi!=1), trend (Nchi!=1) and cycle -- each with the same three steps:
    // run the fx* forward recursion on the total denominator, REFLECT the
    // first qstar bx* values onto Nz-qstar+1..Nz, then run the bx* recursion
    // driven by the filter (gs/gt/gc) applied to the forecast of z.
    //
    // Two deliberate narrowings, both observationally equivalent here.
    // (1) The Fortran loops run to `kp` (=PFCST, the ARRAY bound) rather than
    //     to lf, and store into sc/trend only when `k <= Nz+lf`; everything
    //     past that lands in forsbias/fortbias, which feed only ABIASC's
    //     `xx` (an over-limit PRINT flag) and the annual-average rate tables.
    //     Running to lf therefore changes no saved value -- and it is what
    //     lets the `forbias` fallback at ansub3.f:424-428 be dropped, since
    //     for k <= Nz+lf the index `k-j+1` is never past Nz+lf.
    // (2) The TREND block is transcribed but its result is DEAD: ansub3.f:660
    //     overwrites trend(Nz+1..Nz+lf) with `z - sc - cycle - ir`. It is kept
    //     because it is what fills fortbias past lf, and because dropping a
    //     block on a "nothing reads it" argument is exactly the reasoning that
    //     has been wrong before in this port.
    std::vector<double> f_trend_i, f_sc_i, f_cycle_i, f_sa_i;
    const bool have_cycle_fcst =
        comp.varwnc > 1.0e-10 && (comp.nthetc != 0 || cd.ncyc != 1);
    if (lfor > 0 && lf > 0 && nz + l2 <= nz + static_cast<int>(zextFwd.size())) {
        f_trend_i.assign(lf, 0.0);
        f_sc_i.assign(lf, 0.0);
        f_cycle_i.assign(lf, 0.0);
        f_sa_i.assign(lf, 0.0);
        // ansub3.f:355-361 -- the forecast span starts zeroed, so a block that
        // does not run leaves its component at 0 (i.e. no seasonal / no cycle).
        auto FCX = [&](int k) -> double {  // extZ(k), 1-indexed, k may exceed nz
            return (k <= nz) ? z[k - 1] : zextFwd[k - nz - 1];
        };
        // One block, three times (ansub3.f:363-405 / :407-444 / :447-478).
        // `g` is the one-sided filter, `fx`/`bx` the component's own halves.
        auto forecast_component = [&](const std::vector<double>& g,
                                       std::vector<double>& fx,
                                       std::vector<double>& bx,
                                       std::vector<double>& outc) {
            int k0 = 2 * qstar - 1;
            if (k0 <= lf) {
                for (int i = k0; i <= lf; ++i) {
                    double sum = 0.0;
                    for (int j = 2; j <= pstar; ++j)
                        sum -= totden[j] * fx[nz + i - j + 1];
                    fx[nz + i] = sum;
                }
            }
            // ansub3.f:377-379 -- the BACKWARD half is reflected: bx runs on
            // the reversed series, so its first qstar values are what the
            // forward direction needs at the sample's far end.
            for (int i = 1; i <= qstar; ++i) bx[nz - i + 1] = bx[i];
            for (int i = 1; i <= lf; ++i) {
                int k = nz + i;
                double sum = 0.0;
                for (int j = 1; j <= maxpq; ++j) sum += g[j] * FCX(k - j + 1);
                if (qstar != 1)
                    for (int j = 2; j <= qstar; ++j)
                        sum -= thstr0[j] * bx[k - j + 1];
                bx[k] = sum;
                outc[i - 1] = fx[k] + bx[k];
            }
        };
        if (!npsi1) forecast_component(gs, fxs, bxs, f_sc_i);
        if (cd.nchi != 1) forecast_component(gt, fxt, bxt, f_trend_i);
        if (have_cycle_fcst) forecast_component(gc, fxc, bxc, f_cycle_i);
        // ansub3.f:651-672. ir is identically 0 over the forecast span
        // (:518-520), so the trend is the RESIDUAL of the other two -- which
        // is what discards the trend block above. The floor is relative to the
        // largest |z| over the HISTORICAL span only.
        double maxz = 0.0;
        for (int i = 0; i < nz; ++i) maxz = std::max(maxz, std::fabs(z[i]));
        for (int i = 1; i <= lf; ++i) {
            double zk = FCX(nz + i);
            double t = zk - f_sc_i[i - 1] - f_cycle_i[i - 1];
            if (std::fabs(t) < 1.0e-15 * maxz) t = 0.0;
            f_trend_i[i - 1] = t;
            f_sa_i[i - 1] = zk - f_sc_i[i - 1];  // isCloseToTD always false
        }
    }


    // Back-transform to the ORIGINAL-units tables SEATS actually saves
    // (s10-s18): additive as-is for lam==1 (no transform); exp() (additive
    // -> multiplicative/ratio) for lam==0.0 (log) -- session 13, verified
    // against golden airline_seats/payems_seats (s13==s11/s12/cycle-factor
    // ratio form; s10/s16/s18==z/sa, all multiplicative). Any OTHER
    // Box-Cox power (sqrt/inverse/logistic) is unsupported this pass (no
    // corpus spec uses one) -- falls back to the additive path, which is
    // simply wrong for those, not merely approximate.
    bool is_log = std::fabs(ctx.arima.lam) < 1e-9;
    out.trend.resize(nz);
    out.sc.resize(nz);
    out.cycle.resize(nz);
    out.sa.resize(nz);
    out.ir.resize(nz);
    out.seasonal_factor.resize(nz);
    out.seasonal_add.resize(nz);
    out.combined_factor.resize(nz);
    out.combined_add.resize(nz);
    // s16/s18 refold the removed calendar/outlier effects: they are the original
    // series (calendar+mean inclusive) over sa, not the linearized z over sa
    // (=s10). ctx.seats_combined_orig carries that raw transformed series when
    // regressors were stripped; when empty, s16/s18 collapse onto s10.
    const std::vector<double>& corig = ctx.seats_combined_orig;
    const bool have_corig = static_cast<int>(corig.size()) >= nz;
    if (is_log) {
        // SEATS multiplicative bias correction (sigsub.f:1539-1585, bias==1 --
        // the default for the log path per ansub9.f:1118). bias1c = mean of the
        // multiplicative seasonal over complete years; bias2c = mean of the
        // multiplicative irregular over all obs; bias3c = bias1c*bias2c scales
        // the trend. For npsi==1 (no seasonal) sc_i==0 so bias1c==1 and the
        // whole term reduces to log(mean(exp(ir))) applied trend-only -- the
        // uniform +2.1978124351562656e-06 offset seen on payems_seats s12/s13.
        int nyr = (nz / mo.mq) * mo.mq;
        double bias1c = 0.0, bias2c = 0.0;
        for (int i = 0; i < nz; ++i) {
            if (i < nyr) bias1c += std::exp(sc_i[i]);
            bias2c += std::exp(ir_i[i]);
        }
        bias1c /= nyr;
        bias2c /= nz;
        double bias3c = bias1c * bias2c;
        for (int i = 0; i < nz; ++i) {
            double zorig = std::exp(z[i]);
            out.sc[i] = std::exp(sc_i[i]) / bias1c;
            out.cycle[i] = std::exp(cycle_i[i]);
            out.sa[i] = zorig / out.sc[i];               // non-TD (sigsub.f:1574)
            out.trend[i] = std::exp(trend_i[i]) * bias3c;
            double denom = out.trend[i] * out.cycle[i];
            out.ir[i] = (denom != 0.0) ? out.sa[i] / denom : 1.0;
            out.seasonal_factor[i] =
                (out.sa[i] != 0.0) ? zorig / out.sa[i] : 1.0;
            // s10 == z/sa in the log/multiplicative convention.
            out.seasonal_add[i] = out.seasonal_factor[i];
            // s16/s18 == a1/sa (regression + prior refolded); == s10 when
            // nothing was stripped (corig empty -> zc == zorig). corig is the
            // raw series in ORIGINAL units, so no exp() here.
            double zc = have_corig ? corig[i] : zorig;
            out.combined_factor[i] =
                (out.sa[i] != 0.0) ? zc / out.sa[i] : 1.0;
            out.combined_add[i] = out.combined_factor[i];
        }
        // sigsub.f:1586-1605 -- the forecast span's own antilog. NOT the same
        // expression as the historical one above: `noC` gates the trend's bias
        // scaling, `sa` is formed from the ALREADY-scaled sc (before its x100),
        // and sc/cycle are punched in PERCENT.
        const bool noC = comp.varwnc < 1.0e-10 && comp.nthetc == 0 &&
                         cd.ncyc == 1;
        for (int i = 0; i < lfor && i < static_cast<int>(f_trend_i.size());
             ++i) {
            double sck = npsi1 ? std::exp(f_sc_i[i])
                                : std::exp(f_sc_i[i]) / bias1c;
            // MEASURED, and it is `bias1c` where sigsub.f:1594 literally reads
            // `bias3c`. The reason is a NORMALIZATION difference, not a
            // transcription choice, and it is only visible here.
            //
            // A SEATS decomposition is unique only up to a constant log shift
            // between the seasonal and the trend, and the bias block absorbs
            // exactly such a shift: this port's raw `sc_i` sits ln(bias1c)
            // above the oracle's and its raw `trend_i` ln(bias3c) below, so
            // the HISTORICAL transform (`exp(sc)/bias1c`, `exp(trend)*bias3c`)
            // lands on the same s10/s11/s12/s13 either way -- all four gate
            // bit-exact. The oracle's own bias1c/bias3c are therefore ~1 where
            // this port's are 1.00882/1.00893 on airline.
            //
            // The FORECAST trend is not built by the filter at all: it is the
            // RESIDUAL `z - sc - cycle` (ansub3.f:660), so it inherits the
            // shift from `sc` rather than from `trend` -- one factor of
            // bias1c, not bias1c*bias2c. Two independent identities in the
            // goldens pin it: `tfd == afd` to the last digit on all 39 specs
            // with no transitory component, and `tfd*yfd/100 == afd` on the
            // 13 that have one. Using bias3c leaves tfd exactly bias2c
            // (1.0001048 on airline) off while sfd/afd/yfd are already exact.
            double trk = std::exp(f_trend_i[i]) * bias1c;
            double cyk = std::exp(f_cycle_i[i]);
            double zk = std::exp(f_sa_i[i] + f_sc_i[i]);  // exp(z(k))
            out.f_trend.push_back(trk);
            out.f_sa.push_back(zk / sck);
            out.f_cycle.push_back(cyk * 100.0);
            out.f_sc.push_back(sck * 100.0);
        }
        // sigex.f:3634 -- the cycle slice is punched only under this guard.
        // Fortran precedence: `A .and. B .or. C` is `(A and B) or C`, so a
        // model with Ncyc>1 qualifies whatever varwnc says. NOT the same test
        // as the ESTBUR forecast block above (which reads `ncycth != 0 ||
        // Ncyc != 1`), so a spec can decompose a cycle and still not save it.
        if (!((comp.varwnc > 1.0e-10 && comp.nthetc > 0) || cd.ncyc > 1)) {
            out.f_cycle.clear();
        }
    } else {
        for (int i = 0; i < nz; ++i) {
            out.trend[i] = trend_i[i];
            out.sc[i] = sc_i[i];
            out.cycle[i] = cycle_i[i];
            out.sa[i] = sa_i[i];
            out.ir[i] = ir_i[i];
            // s18 is the z/sa RATIO even in additive mode; s10/s16 are the
            // z-sa additive DIFFERENCE (verified vs unrate_fixed golden:
            // s18=0.99535 == z/sa, s10=s16=-0.0308 == z-sa).
            out.seasonal_factor[i] =
                (sa_i[i] != 0.0) ? z[i] / sa_i[i] : 1.0;
            out.seasonal_add[i] = z[i] - sa_i[i];
            // s16/s18 == original/sa (ratio) and original-sa (additive);
            // == s10 when no regressor was stripped (corig empty -> zc == z).
            double zc = have_corig ? corig[i] : z[i];
            out.combined_factor[i] = (sa_i[i] != 0.0) ? zc / sa_i[i] : 1.0;
            out.combined_add[i] = zc - sa_i[i];
        }
        // sigsub.f:1525-1534's lamd==1 arm rebuilds `sa` over 1..Nz ONLY, so
        // the forecast span is punched exactly as ESTBUR left it -- no antilog,
        // no bias scaling, and no x100 on sc/cycle.
        for (int i = 0; i < lfor && i < static_cast<int>(f_trend_i.size());
             ++i) {
            out.f_trend.push_back(f_trend_i[i]);
            out.f_sc.push_back(f_sc_i[i]);
            out.f_sa.push_back(f_sa_i[i]);
            out.f_cycle.push_back(f_cycle_i[i]);
        }
        if (!((comp.varwnc > 1.0e-10 && comp.nthetc > 0) || cd.ncyc > 1))
            out.f_cycle.clear();
    }
    out.ok = true;
}

}  // namespace x13
