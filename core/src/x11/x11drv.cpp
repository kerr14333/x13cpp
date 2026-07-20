// x11drv.cpp -- X-11 Tier 4 driver routines (see x11drv.hpp). Faithful port of
// the vendored oracle Fortran: vtc.f.
//
// Index convention: 0-based C pointers, Fortran index i -> element [i-1]; range
// args (Pos1bk/Posffc/ib/ie) stay Fortran 1-based. Loop and float op order are
// preserved verbatim (the I/C ratio feeds the trend-length selection; no
// algebraic simplification). COMMON state lives on ctx.x11opt/x11ptr/x11msc.
#include "x11/x11drv.hpp"

#include "common/x13context.hpp"
#include "x11/x11filt.hpp"       // hndtrn, divsub
#include "x11/x11seas.hpp"       // vsfa, vsfb
#include "numeric/numeric.hpp"   // dpeq

#include <cmath>

namespace x13 {

namespace {
// srslen.prm: PLEN = POBS + 2*PFCST -- the back/forecast-padded buffer length.
// vtc's /work/ Temp scratch is dimensioned PLEN in the Fortran.
constexpr int PLEN = 1020;
}  // namespace

// vtc.f -- variable trend cycle. Selects the Henderson trend-filter length from
// the measured I/C ratio (Ratic) after a first (Ny+1)-term pass, then re-filters.
void vtc(X13Context& ctx, double* stc, double* stci) {
    x11opt_cmn& opt = ctx.x11opt;
    x11ptr_cmn& ptr = ctx.x11ptr;

    const int pos1bk = ptr.pos1bk;
    const int posffc = ptr.posffc;
    const int posfob = ptr.posfob;
    const int muladd = opt.muladd;
    const int ny = opt.ny;
    const int ktcopt = opt.ktcopt;
    const int kpart = opt.kpart;
    const bool tru7hn = ctx.x11msc.tru7hn;

    // Nterm and Tic are COMMON (x11opt) -- every assignment writes them back, and
    // hndtrn updates Tic in place in the 7-term reduce; bind by reference.
    int& nterm = opt.nterm;
    double& tic = opt.tic;

    double temp[PLEN];  // COMMON /work/ Temp(PLEN)

    // 1-based views to keep the arithmetic diffable against the Fortran.
    auto STC = [&](int i) -> double& { return stc[i - 1]; };
    auto TMP = [&](int i) -> double& { return temp[i - 1]; };

    bool lsame = false;

    // If the series is monthly apply a 13-term Henderson; if quarterly a 5-term.
    nterm = ny + 1;
    hndtrn(stc, stci, pos1bk, posffc, nterm, tic, /*lend=*/false, lsame, tru7hn);

    // Drop end terms and calculate the irregular series.
    int ib = pos1bk + nterm / 2;
    int ie = posffc - nterm / 2;
    divsub(temp, stci, stc, ib, ie, muladd);

    // Calculate the Ibar/Cbar ratio.
    int ie1 = posfob - nterm / 2 - 1;
    double apcc = 0.0;
    double apci = 0.0;
    if (muladd == 0) {
        for (int i = ib; i <= ie1; ++i) {
            apcc += std::fabs(STC(i + 1) - STC(i)) / STC(i);
            apci += std::fabs(TMP(i + 1) - TMP(i)) / TMP(i);
        }
    } else {
        for (int i = ib; i <= ie1; ++i) {
            apcc += std::fabs(STC(i + 1) - STC(i));
            apci += std::fabs(TMP(i + 1) - TMP(i));
        }
    }

    double r;
    if (dpeq(apcc, 0.0)) {
        opt.ratic = 999.0;
        r = opt.ratic;
    } else {
        opt.ratic = apci / apcc;
        r = opt.ratic * 12 / ny;
    }

    // Check if the trend-cycle moving average was preselected.
    if (ktcopt <= 0) {
        if ((kpart == 2 && r >= 1.0) || (r >= 1.0 && r < 3.5)) {
            lsame = true;
        } else if (r < 1.0) {
            if (ny == 12) {
                nterm = 9;
                tic = 1.0;
            }
        } else {
            tic = 4.5;
            nterm = 23;
            if (ny == 4) nterm = 7;
        }
    } else if (ktcopt == nterm) {
        lsame = true;
    } else {
        nterm = ktcopt;
    }

    // Generate and apply the symmetric Henderson filter and its end weights.
    hndtrn(stc, stci, pos1bk, posffc, nterm, tic, /*lend=*/true, lsame, tru7hn);
}

// sfmsr.f -- MSR global seasonal-filter selection + the vsfa/vsfb seasonal pass.
void sfmsr(X13Context& ctx, double* sts, double* stsi, int lfda, int llda,
           int lldaf) {
    x11opt_cmn& opt = ctx.x11opt;
    const int ny = opt.ny;
    const int muladd = opt.muladd;
    const bool psuadd = ctx.x11msc.psuadd;
    const bool shrtsf = ctx.x11msc.shrtsf;

    // If the MSR seasonal-filter selection option is on, calculate a global MSR
    // and try to select a seasonal filter length.
    if (opt.lterm == 6) {
        if (opt.lmsr == 6) {
            // Set llda1 to the end of the last whole year.
            int llda1 = llda - (llda % ny);
            // (The Fortran pass counter `i` fed only the deferred WRITE output.)
            while (opt.lterm == 6) {
                // If the span to be tested is less than 5 years long, use a 3x5
                // seasonal filter.
                if ((llda1 - lfda + 1) < (5 * ny)) {
                    opt.lterm = 2;
                } else {
                    vsfa(stsi, lfda, llda1, ny, muladd, psuadd, opt.rati.data(),
                         opt.ratis);
                    if (opt.ratis <= 2.5) {
                        opt.lterm = 1;
                    } else if (opt.ratis >= 6.5) {
                        opt.lterm = 3;
                    } else if (opt.ratis >= 3.5 && opt.ratis <= 5.5) {
                        opt.lterm = 2;
                        ctx.work2.l3x5 = true;
                    } else {
                        // Global MSR meets none of the criteria: drop a year from
                        // the end of the series and try again.
                        llda1 = llda1 - ny;
                    }
                }
            }
            for (int i = 1; i <= ny; ++i) {
                if (opt.lter(i) == 6) opt.lter(i) = opt.lterm;
                if (ctx.work2.l3x5 && (opt.lter(i) != 0 && opt.lter(i) != 2))
                    ctx.work2.l3x5 = false;
            }
        } else {
            // Sliding-spans run: reset the seasonal filter length to the
            // selection made for the entire series.
            opt.lterm = opt.lmsr;
            for (int i = 1; i <= ny; ++i)
                if (opt.lter(i) == 6) opt.lter(i) = opt.lmsr;
        }
    }

    double temp[PLEN];  // vsfb work scratch
    vsfa(stsi, lfda, llda, ny, muladd, psuadd, opt.rati.data(), opt.ratis);
    vsfb(sts, stsi, lfda, lldaf, ny, opt.lterm, opt.lter.data(), opt.ksect,
         shrtsf, temp, muladd);
}

}  // namespace x13
