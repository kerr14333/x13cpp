// model_decode.cpp -- nmlmdl.f + transc.f (TRANS0/TRANS2). See
// model_decode.hpp for the full parity discussion.
#include "seats/model_decode.hpp"

#include <cmath>

#include "gen/model.hpp"          // prm::AR, prm::MA, prm::DIFF
#include "specparse/specparse.hpp"  // errhdr, writln, abend, stdio::STDERR

namespace x13 {

// TRANS2(p,nn,x,m,n) -- transc.f:4. Only the C(1..3) closed-form values feed
// the output P(M+I)=-C(I); the cubic-root (Alph) branch for order-3 groups is
// oracle dead code (see model_decode.hpp) and is omitted here.
void trans2(double* p, int nn, const double* x, int m, int n) {
    (void)nn;
    double c[3] = {0.0, 0.0, 0.0};
    int j = n - m;
    if (j < 2) {
        c[0] = x[n - 1];
    } else if (j == 2) {
        c[0] = x[m] * (1.0 - x[n - 1]);
        c[1] = x[n - 1];
    } else {
        double s = (2.0 * x[m] - 1.0) * (1.0 - x[n - 1]);
        double d = (1.0 + x[n - 1]) * ((1.0 + x[m]) * (1.0 + x[m + 1]) - 1.0);
        c[0] = 0.5 * (s + d);
        c[1] = 0.5 * (s - d);
        c[2] = x[n - 1];
    }
    for (int i = 1; i <= n - m; ++i) p[m + i - 1] = -c[i - 1];
}

// TRANS0(p,nn,x,ib,ie,iprs,ur,xl) -- transc.f:144.
void trans0(const double* p, int nn, double* x, int ib, int ie, int iprs,
            double ur, double xl) {
    (void)nn;
    double xmin = -xl, xmax = xl;
    int npq = ie - ib + 1;
    double phith[3] = {0.0, 0.0, 0.0};
    for (int i = 1; i <= npq; ++i) phith[i - 1] = -p[ib + i - 2];

    if (npq <= 1) {
        x[ib - 1] = phith[0];
    } else if (npq <= 2) {
        if (std::fabs(1.0 - phith[1]) < 1.0e-9) phith[1] = ur;
        x[ib - 1] = phith[0] / (1.0 - phith[1]);
        x[ie - 1] = phith[1];
    } else {
        if (std::fabs(phith[2] - 1.0) < 1.0e-9)
            phith[2] = std::copysign(ur, phith[2]);
        x[ib - 1] = 0.5 * ((phith[0] + phith[1]) / (1.0 - phith[2]) + 1.0);
        x[ib] = 1.0 + (phith[0] - phith[1]) / (1.0 + phith[2]);
        if (std::fabs(x[ib - 1] + 1.0) < 1.0e-9) x[ib - 1] = -ur;
        x[ib] = x[ib] / (1.0 + x[ib - 1]) - 1.0;
        x[ie - 1] = phith[2];
    }

    for (int j = ib; j <= ie; ++j) {
        double xtest = (x[j - 1] - xmin) / (xmax - xmin);
        if (xtest < 0.01) x[j - 1] = (j <= iprs) ? -ur : xmin;
        if (xtest > 0.99) x[j - 1] = (j <= iprs) ? ur : xmax;
    }
}

namespace {

// One AR/MA operator group's decode target (nonseasonal or seasonal side of
// one of the two operator types).
struct GroupTarget {
    int* n;         // -> out.p / out.bp / out.q / out.bq
    double* raw;     // Phi(i)=-Arimap(iparma) landing array (3 slots)
    double* final_;  // out.phi / out.bphi / out.th / out.bth (3 slots)
    int period;      // 1 (nonseasonal) or Sp (seasonal), for the Arimal check
};

void decode_err(X13Context& ctx, const char* msg) {
    errhdr(ctx);
    writln(ctx, msg, stdio::STDERR, ctx.units.mt2, true);
    abend(ctx);
}

}  // namespace

bool seats_decode_model(X13Context& ctx, double xl, SeatsModelOrders& out) {
    using namespace prm;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    constexpr int NN = 3;
    constexpr double UR = 1.0;

    out = SeatsModelOrders{};
    out.d = m.nnsedf;
    out.bd = m.nseadf;
    out.mq = m.sp;
    int ardsp = m.nnsedf + m.nseadf;
    int sp = m.sp;

    double phi_raw[3] = {0, 0, 0}, bphi_raw[3] = {0, 0, 0};
    double th_raw[3] = {0, 0, 0}, bth_raw[3] = {0, 0, 0};

    int iparma = ardsp + 1;
    out.nfixed = 0;
    for (int iflt = AR; iflt <= MA; ++iflt) {
        int begopr = m.mdl(iflt - 1);
        int endopr = m.mdl(iflt) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int nlag = m.opr(iopr) - m.opr(iopr - 1);
            if (nlag == 0) continue;

            // Classify by period, exactly mirroring what mkoprt.f's title
            // string would have encoded (Period==1 -> nonseasonal,
            // Period==Sp -> seasonal, else silently uncounted -- see
            // model_decode.hpp).
            int period = m.oprfac(iopr);
            bool nonseasonal = (period == 1);
            bool seasonal = (period == sp) && !nonseasonal;
            if (!nonseasonal && !seasonal) continue;  // "Period N" -- unhandled

            if (nlag > NN) {
                decode_err(ctx,
                    "  NOTE: The SEATS signal extraction routines cannot process "
                    "more than  3 terms.\n        The program will stop "
                    "executing; try specifying another ARIMA model.\n");
                return false;
            }

            GroupTarget g{};
            if (iflt == AR) {
                g = nonseasonal
                        ? GroupTarget{&out.p, phi_raw, out.phi, 1}
                        : GroupTarget{&out.bp, bphi_raw, out.bphi, sp};
            } else {  // MA
                g = nonseasonal
                        ? GroupTarget{&out.q, th_raw, out.th, 1}
                        : GroupTarget{&out.bq, bth_raw, out.bth, sp};
            }
            *g.n = nlag;
            for (int i = 1; i <= nlag; ++i) {
                int expect_lag = i * g.period;
                if (m.arimal(iparma) != expect_lag) {
                    decode_err(ctx,
                        "  NOTE: The SEATS signal extraction routines cannot "
                        "process missing lag models.\n        The program will "
                        "stop executing; try specifying another ARIMA "
                        "model.\n");
                    return false;
                }
                g.raw[i - 1] = 0.0 - d.arimap(iparma);
                if (m.arimaf(iparma)) out.nfixed = out.nfixed + 1;
                ++iparma;
            }
        }
    }

    // Preserve the raw (pre-TRANS0) estimated MA coefficients for the forecast
    // extension (see model_decode.hpp).
    for (int i = 0; i < 3; ++i) {
        out.th_raw[i] = th_raw[i];
        out.bth_raw[i] = bth_raw[i];
    }

    int iprs = out.p + out.bp;
    int iqrs = out.q + out.bq;

    // TRANS0-then-TRANS2 bounded reparametrization round-trip, per operator
    // group, exactly mirroring nmlmdl.f's four IF blocks (each group uses the
    // FULL x2/x buffer laid out [AR-nonseas | AR-seas | MA-nonseas |
    // MA-seas], hence the M/N offsets below).
    double x2[12] = {0}, xbuf[12] = {0};
    if (out.p > 0) {
        for (int i = 0; i < out.p; ++i) x2[i] = phi_raw[i];
        trans0(x2, 12, xbuf, 1, out.p, iprs, UR, xl);
        trans2(x2, 12, xbuf, 0, out.p);
        for (int i = 0; i < out.p; ++i) out.phi[i] = x2[i];
    }
    if (out.bp > 0) {
        for (int i = 0; i < out.bp; ++i) x2[out.p + i] = bphi_raw[i];
        trans0(x2, 12, xbuf, out.p + 1, iprs, iprs, UR, xl);
        trans2(x2, 12, xbuf, out.p, iprs);
        for (int i = 0; i < out.bp; ++i) out.bphi[i] = x2[out.p + i];
    }
    if (out.q > 0) {
        for (int i = 0; i < out.q; ++i) x2[iprs + i] = th_raw[i];
        trans0(x2, 12, xbuf, iprs + 1, iprs + out.q, iprs, UR, xl);
        trans2(x2, 12, xbuf, iprs, iprs + out.q);
        for (int i = 0; i < out.q; ++i) out.th[i] = x2[iprs + i];
    }
    if (out.bq > 0) {
        for (int i = 0; i < out.bq; ++i) x2[iprs + out.q + i] = bth_raw[i];
        trans0(x2, 12, xbuf, iprs + out.q + 1, iprs + iqrs, iprs, UR, xl);
        trans2(x2, 12, xbuf, iprs + out.q, iprs + iqrs);
        for (int i = 0; i < out.bq; ++i) out.bth[i] = x2[iprs + out.q + i];
    }

    return true;
}

}  // namespace x13
