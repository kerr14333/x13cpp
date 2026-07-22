// decompspectrum.cpp -- see decompspectrum.hpp.
#include "seats/decompspectrum.hpp"
#include "seats/seatsfact.hpp"  // mak1
#include "seats/seatspoly.hpp"  // conv, conj (MAspectrum's unexercised branch)

#include <algorithm>

namespace x13 {

void decomp_spectrum(SpectruResult& sr, const SeatsCanonicalDenoms& cd,
                      bool is_close_to_td, SeatsComponentModels& out) {
    out = SeatsComponentModels{};
    SpectruHarmonics& H = sr.h;

    double utf[24] = {};
    double vf[52] = {};
    double ucf[34] = {};

    // spectrum.f:1462-1467: trend -- pad Ut to Ft's length (Nt==nchi
    // always, CONJ(chi,chi) preserves length), then subtract the floor.
    if (cd.nchi != 1) {
        H.ut[H.nt - 1] = 0.0;  // Ut(Nt) = ZERO
        H.nut = H.nt;
        for (int i = 0; i < H.nut; ++i) utf[i] = H.ut[i] - sr.enot * H.ft[i];
    }

    // spectrum.f:1535-1539: seasonal.
    if (cd.npsi != 1) {
        H.v[H.ns - 1] = 0.0;  // V(Ns) = ZERO
        for (int i = 0; i < H.ns; ++i) vf[i] = H.v[i] - sr.estar * H.fs[i];
    }

    // spectrum.f:1575-1592: cycle/transitory. When ncycth==0 (a genuine
    // ncyc>1 cycle, no ADDJ fold), Uc is padded to Fc's length; when
    // ncycth!=0 (the qstar>pstar ADDJ fold grew Uc past Fc's original
    // length), Fc itself is extended to match Uc instead, and Nc is
    // permanently updated (mutating cd's own cyc length as seen from here
    // on -- matches the oracle's COMMON-block mutation of Nc/Fc).
    if (sr.ncycth != 0 || cd.ncyc != 1) {
        if (sr.ncycth == 0) {
            for (int i = H.nuc; i < H.nc; ++i) H.uc[i] = 0.0;
            H.nuc = H.nc;
        } else {
            for (int i = H.nc; i < H.nuc; ++i) H.fc[i] = 0.0;
            H.nc = H.nuc;
        }
        for (int i = 0; i < H.nuc; ++i) ucf[i] = H.uc[i] - sr.enoc * H.fc[i];
    }

    // spectrum.f:1529-1533/1570-1573/1625-1628: ESTBUR's ct/cs/cc filter
    // numerators, built from the SAME utf/vf/ucf just computed above (plus
    // one more MULTFN pass through the OTHER two components' F-polynomials)
    // -- ct(1)=us(1), ct(j)=0.5*us(j) for j=2..nus. Session 12: confirmed
    // against an oracle instrumentation dump for unrate_seats (ct=[
    // 0.535245394617457, 0.267622697308728], matching bit-for-bit).
    //
    // SCOPE GUARD (session 12): this MULTFN chain's output size (mplus1+
    // nplus1-1, twice) can exceed the local scratch buffers for models with
    // larger seasonal/cycle polynomials than any currently-gated spec (a
    // heap-corruption crash was observed on airline_fixed-airline-seats
    // before this guard was added) -- ct/cs/cc are only validated for
    // unrate_seats's additive/no-seasonal/no-cycle shape this pass anyway,
    // so skip (leave zero) rather than risk overflowing the 80-wide
    // scratch buffers for untested, out-of-scope model shapes.
    constexpr int kScratch = 80;
    if (cd.nchi != 1 && H.nut + H.nc - 1 + H.ns - 1 < kScratch) {
        double vn[kScratch] = {};
        int nvn = 0;
        multfn(utf, H.nut, H.fc, H.nc, vn, nvn);
        double us[kScratch] = {};
        int nus = 0;
        multfn(vn, nvn, H.fs, H.ns, us, nus);
        out.ct[0] = us[0];
        for (int j = 1; j < nus; ++j) out.ct[j] = 0.5 * us[j];
        out.nct = nus;
    }
    if (cd.npsi != 1 && H.ns + H.nc - 1 + H.nt - 1 < kScratch) {
        double vn[kScratch] = {};
        int nvn = 0;
        multfn(vf, H.ns, H.fc, H.nc, vn, nvn);
        double us[kScratch] = {};
        int nus = 0;
        multfn(vn, nvn, H.ft, H.nt, us, nus);
        out.cs[0] = us[0];
        for (int j = 1; j < nus; ++j) out.cs[j] = 0.5 * us[j];
        out.ncs = nus;
    }
    if ((sr.ncycth != 0 || cd.ncyc != 1) &&
        H.nuc + H.ns - 1 + H.nt - 1 < kScratch) {
        double vn[kScratch] = {};
        int nvn = 0;
        multfn(ucf, H.nuc, H.fs, H.ns, vn, nvn);
        double us[kScratch] = {};
        int nus = 0;
        multfn(vn, nvn, H.ft, H.nt, us, nus);
        out.cc[0] = us[0];
        for (int j = 1; j < nus; ++j) out.cc[j] = 0.5 * us[j];
        out.ncc = nus;
    }

    // spectrum.f:1664-1671: trivial defaults (constant-1 "MA polynomials"),
    // overwritten below wherever the corresponding branch runs.
    out.nthetp = 1;
    out.thetp[0] = 1.0;
    out.nthets = 1;
    out.thets[0] = 1.0;
    out.nthetc = 1;
    out.thetc[0] = 1.0;
    out.nthadj = 1;
    out.thadj[0] = 1.0;

    // ---- MAspectrum (spectrum.f:2710-2890) ----
    // MAspectrum's own local `nounit` is always 0 (spectrum.f:2748) -> every
    // MAK1 call here has nnio=0, so the /unitmak/ XL nudge never fires and
    // its value is irrelevant (a placeholder is passed).
    constexpr int NOUNIT = 0;
    constexpr double XL_UNUSED = 0.99;
    double toterr;

    // spectrum.f:2749-2765: trend.
    out.varwnp = 0.0;
    if (cd.nchi != 1) {
        mak1(utf, H.nut, out.thetp, out.nthetp, out.varwnp, NOUNIT, XL_UNUSED,
             toterr);
    }

    // spectrum.f:2767-2787: seasonal.
    out.varwns = 0.0;
    if (cd.npsi != 1) {
        mak1(vf, H.ns, out.thets, out.nthets, out.varwns, NOUNIT, XL_UNUSED,
             toterr);
    }

    // spectrum.f:2789-2815: cycle/transitory.
    out.varwnc = 0.0;
    if (sr.ncycth != 0 || cd.ncyc != 1) {
        mak1(ucf, H.nuc, out.thetc, out.nthetc, out.varwnc, NOUNIT,
             XL_UNUSED, toterr);
    }

    // spectrum.f:2817-2884: seasonally-adjusted (trend + cycle + irregular
    // -- everything but the seasonal component).
    out.varwna = 0.0;
    if (cd.nchcyc != 1 || sr.ncycth != 0) {
        if (cd.npsi == 1) {
            // spectrum.f:2819-2828: no seasonal component at all -> SA IS
            // the whole model, verbatim. This is the branch every current
            // corpus spec (npsi==1 throughout) actually takes.
            for (int i = 0; i < cd.qstar; ++i) out.thadj[i] = cd.thstar[i];
            for (int i = cd.qstar; i < cd.nchcyc; ++i) out.thadj[i] = 0.0;
            out.nthadj = cd.qstar;
            out.varwna = 1.0;
        } else {
            // spectrum.f:2829-2883: real seasonally-adjusted factorization.
            // NOT exercised by any corpus spec (all have npsi==1); ported
            // for completeness, not gated.
            double us[80] = {}, vn[80] = {}, dum[80] = {};
            int nus = 0, nvn = 0, ndum = 0;
            if (is_close_to_td) {
                conj(cd.chi, cd.nchi, cd.chi, cd.nchi, us, nus);
            } else {
                conj(cd.chcyc, cd.nchcyc, cd.chcyc, cd.nchcyc, us, nus);
            }
            for (int i = 0; i < nus; ++i) us[i] *= sr.qt1;
            for (int i = nus; i < 50; ++i) us[i] = 0.0;

            if (cd.nchi != 1) {
                if (is_close_to_td) {
                    conj(out.thetp, out.nthetp, out.thetp, out.nthetp, dum,
                         ndum);
                } else {
                    conv(out.thetp, out.nthetp, cd.cyc, cd.ncyc, vn, nvn);
                    conj(vn, nvn, vn, nvn, dum, ndum);
                }
                for (int i = 0; i < ndum; ++i) us[i] += out.varwnp * dum[i];
                nus = std::max(nus, ndum);
            }
            if (!is_close_to_td) {
                if (sr.ncycth != 0 || cd.ncyc != 1) {
                    conv(out.thetc, out.nthetc, cd.chi, cd.nchi, vn, nvn);
                    conj(vn, nvn, vn, nvn, dum, ndum);
                    for (int i = 0; i < ndum; ++i)
                        us[i] += out.varwnc * dum[i];
                    nus = std::max(nus, ndum);
                }
            }
            mak1(us, nus, out.thadj, out.nthadj, out.varwna, NOUNIT,
                 XL_UNUSED, toterr);
        }
    }
}

}  // namespace x13
