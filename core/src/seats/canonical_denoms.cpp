// canonical_denoms.cpp -- see canonical_denoms.hpp.
#include "seats/canonical_denoms.hpp"
#include "seats/seatsalloc.hpp"
#include "seats/seatsdenoms.hpp"
#include "seats/seatspoly.hpp"

#include <cstring>

namespace x13 {

void seats_canonical_denoms(const SeatsModelOrders& mo, double rmod,
                             double epsphi, SeatsCanonicalDenoms& out) {
    out = SeatsCanonicalDenoms{};

    // phis(1)=1; phis(i+1)=Phi(i) -- analts.f:2848-2853 literally reads
    // phis(i+1)=-Phi(i), but this port's mo.phi are ALREADY the true-sign
    // polynomial coefficients (model_decode.cpp: raw=-arimap, so mo.phi =
    // -(reported AR) = the +coeff in (1 - phi1 B - ...) written true-sign),
    // exactly mirroring the MA-side session-5 correction below (ths(i+1)=
    // +Th(i)). VERIFIED on the (2 1 0)(0 1 1) probe: -mo.phi gave cyc =
    // [1,-0.3616,-0.0637] and Totden [1,-1.3616,...] vs oracle transitory AR
    // [1,+0.3616,+0.0637] / Totden [1,-0.6384,-0.2979,-0.0637,...]; +mo.phi
    // reproduces the oracle. (p=0 for every airline-family corpus spec, so
    // this sign never had a data point until the general-shape probe.)
    double phis[66] = {};
    phis[0] = 1.0;
    for (int i = 0; i < mo.p; ++i) phis[i + 1] = mo.phi[i];
    int nphi = mo.p + 1;

    if (mo.p > 0) {
        rpq(phis, nphi, out.rez, out.imz, out.modul, out.ar, out.pr, 1, 1);
    }

    // bphis(1)=1; bphis(Mq*j+1)=+Bphi(j), all other slots in [.,Mq*j] zero.
    // analts.f:2860-2868 literally reads -Bphi, but mo.bphi is ALREADY the
    // true-sign seasonal-AR coefficient (same convention as mo.phi -- see the
    // phis note above and build_bphist's `ss[(k+1)*mq] = mo.bphi[k]`). The
    // -bphi form gave ~3-5% error on bp>0 probes ((0 1 1)(1 1 0) etc); +bphi
    // reproduces the oracle. SEATS restricts Bp<=1 (seatsdenoms.hpp), so the
    // general j-loop is future-proofing, not exercised past j=1 today.
    double bphis[66] = {};
    bphis[0] = 1.0;
    for (int j = 1; j <= mo.bp; ++j) {
        for (int i = (j - 1) * mo.mq + 1; i < j * mo.mq; ++i) bphis[i] = 0.0;
        bphis[j * mo.mq] = mo.bphi[j - 1];
    }

    seats_init_denoms(mo.d, mo.bd, mo.bp, bphis, mo.mq, out.chins, out.nchins,
                       out.chis, out.nchis, out.psins, out.npsins, out.psis,
                       out.npsis, out.cycns, out.ncycns, out.cycs, out.ncycs);

    out.is_close_to_td = false;  // sigex.f:552, IsCloseToTD=.FALSE. before F1RST
    f1rst(mo.p, out.imz, out.rez, out.ar, epsphi, mo.mq, out.cycns, out.ncycns,
          out.psins, out.npsins, out.cycs, out.ncycs, out.chins, out.nchins,
          out.chis, out.nchis, out.modul, out.psis, out.npsis, rmod,
          out.root0c, out.rootpic, out.rootpis, out.is_close_to_td);

    // sigex.f:583-585: fold the stationary/nonstationary halves into the
    // FULL per-component AR denominators SPECTRU consumes.
    conv(out.chis, out.nchis, out.chins, out.nchins, out.chi, out.nchi);
    conv(out.psis, out.npsis, out.psins, out.npsins, out.psi, out.npsi);
    conv(out.cycs, out.ncycs, out.cycns, out.ncycns, out.cyc, out.ncyc);

    // sigex.f:655: pstar = p + d + mq*(bd+bp) + 1 (length of
    // Totden=Chi*Cyc*Psi -- NOT recomputed via CONV chaining; the oracle
    // uses this closed-form order count directly).
    out.pstar = mo.p + mo.d + mo.mq * (mo.bd + mo.bp) + 1;

    // sigex.f:454: Thstr0 = CONV(theta=ths, btheta=bths) -- the model's own
    // true-sign MA numerator polynomial.
    //
    // SESSION-5 CORRECTION: analts.f:2854-2859 literally reads
    // `ths(i+1) = -Th(i)` (mirroring phis(i+1)=-Phi(i) above), and that is
    // what session 4 ported here. It does NOT reproduce the oracle: chased
    // via unrate_seats (qstar==pstar, no ADDJ fold, the cleanest case) --
    // `ths(i+1) = -Th(i)` gives qt1=0.267622697308728 vs golden
    // irrvar=0.232977449296196 (14.9% high); `ths(i+1) = +Th(i)` (this line)
    // gives qt1=0.232977728... , matching golden to 6 sig figs (see
    // tools/seats_scope.md session-5 notes for the full derivation and the
    // remaining open question of WHY the literal Fortran text disagrees with
    // this -- not resolved, but the empirical direction is unambiguous and
    // confirmed independently on payems_seats too, including the ADDJ-fold
    // path). phis/bphis (the AR side, just above) are NOT touched by this
    // correction -- p=0 for every admissible corpus target, so there is no
    // data point to test that sign against; treat it as unverified.
    double ths[66] = {};
    ths[0] = 1.0;
    for (int i = 0; i < mo.q; ++i) ths[i + 1] = mo.th[i];
    int nth = mo.q + 1;

    double bths[66] = {};
    bths[0] = 1.0;
    for (int j = 1; j <= mo.bq; ++j) {
        for (int i = (j - 1) * mo.mq + 1; i < j * mo.mq; ++i) bths[i] = 0.0;
        bths[j * mo.mq] = mo.bth[j - 1];
    }
    int nbth = mo.bq * mo.mq + 1;

    conv(ths, nth, bths, nbth, out.thstar, out.qstar);

    // sigex.f:583-609 (.not.IsCloseToTD branch): Chcyc = CONV(Chi,Cyc), the
    // trend*cycle combined denominator (feeds the .mdc `saden` key).
    conv(out.chi, out.nchi, out.cyc, out.ncyc, out.chcyc, out.nchcyc);
}

}  // namespace x13
