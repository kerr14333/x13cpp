// canonical_denoms.hpp -- wire the already-ported root-finding path
// (rpq -> seats_init_denoms -> F1RST) onto a DECODED model
// (seats/model_decode.hpp's SeatsModelOrders), reproducing sigex.f:447-553
// verbatim: build the plain AR/seasonal-AR polynomials from the decoded
// orders exactly as analts.f:2848-2868 does (phis/bphis, the SAME arrays
// SIGEX receives as its own `phi`/`bphi` formal parameters -- see
// oracle/fortran/analts.f:2926, "call SIGEX(...,phis,bphis,ths,bths,...)"),
// root-find phis via RPQ when p>0, then run seats_init_denoms + F1RST.
//
// STATUS: this is the first time RPQ + seats_init_denoms + F1RST run
// together on a real DECODED model rather than synthetic unit-test inputs
// (tools/seats_scope.md next-increment #2). It is a standalone, independently
// testable building block -- NOT wired into core/src/driver/run_seats.cpp's
// main driver, since the next real consumer downstream (SPECTRU,
// spectrum.f:558) is still unported and nothing yet reads chins/chis/etc.
// past this point.
#ifndef X13_SEATS_CANONICAL_DENOMS_HPP
#define X13_SEATS_CANONICAL_DENOMS_HPP

#include "seats/model_decode.hpp"

namespace x13 {

// Running denominator polynomials + root-classification flags, exactly the
// output set sigex.f:477-554 produces (Chins/Chis/Psins/Psis/Cycns/Cycs +
// IsCloseToTD/root0c/rootpic/rootpis). Sized with headroom matching
// seatsdenoms.hpp's documented minimums.
struct SeatsCanonicalDenoms {
    double chins[66] = {};
    int nchins = 0;
    double chis[66] = {};
    int nchis = 0;
    double psins[66] = {};
    int npsins = 0;
    double psis[66] = {};
    int npsis = 0;
    double cycns[66] = {};
    int ncycns = 0;
    double cycs[66] = {};
    int ncycs = 0;
    bool root0c = false;
    bool rootpic = false;
    bool rootpis = false;
    bool is_close_to_td = false;
    // RPQ's own root arrays (only the first mo.p entries are meaningful;
    // exposed for the Chi*Psi*Cyc consistency check and diagnostics).
    double rez[66] = {}, imz[66] = {}, modul[66] = {}, ar[66] = {}, pr[66] = {};

    // The FULL per-component AR denominators SPECTRU consumes (sigex.f:583-
    // 585: Chi=CONV(Chis,Chins), Psi=CONV(Psis,Psins), Cyc=CONV(Cycs,Cycns)),
    // plus pstar (sigex.f:655: p+d+mq*(bd+bp)+1, the length of
    // Totden=Chi*Cyc*Psi) and thstar/qstar (the model's own MA numerator
    // polynomial, SPECTRU-ready -- built directly from mo.th/mo.bth, NOT
    // negated; see canonical_denoms.cpp's session-5 comment for why this
    // differs from a literal reading of analts.f:2854-2859, confirmed
    // against golden .mdc irrvar bit-exact on 7/7 available corpus specs).
    double chi[66] = {};
    int nchi = 0;
    double psi[66] = {};
    int npsi = 0;
    double cyc[66] = {};
    int ncyc = 0;
    int pstar = 0;
    double thstar[66] = {};
    int qstar = 0;

    // sigex.f:583-609 (the `.not.IsCloseToTD` branch -- IsCloseToTD is
    // always false in this port, see denoms.cpp/f1rst.cpp): Chcyc =
    // CONV(Chi,Nchi,Cyc,Ncyc), the trend*cycle combined AR denominator.
    // Feeds the .mdc's `saden` key (via THADJ's denominator, MAspectrum
    // spectrum.f:2868-2870 / ShowComp IFUNC=2012) and MAspectrum's own
    // seasonally-adjusted MAK1 input for the (unexercised) npsi!=1 branch.
    double chcyc[66] = {};
    int nchcyc = 0;
};

// Builds phis (and, if bp>0, bphis) from `mo` per analts.f:2848-2868, root-
// finds phis via RPQ when mo.p>0 (sigex.f:447 nphi=p+1 / analts.f's own RPQ
// call), then runs seats_init_denoms (sigex.f:477-542) + F1RST (sigex.f:553)
// to produce `out`. rmod/epsphi are the seats{} thresholds F1RST consumes
// (real defaults: rmod=0.5, epsphi=2.0, per ansub9.f's NMLSTS-adjacent
// SETDEFAULT -- see tools/seats_scope.md's GTSEAT note).
void seats_canonical_denoms(const SeatsModelOrders& mo, double rmod,
                             double epsphi, SeatsCanonicalDenoms& out);

}  // namespace x13

#endif  // X13_SEATS_CANONICAL_DENOMS_HPP
