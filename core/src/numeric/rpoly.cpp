// rpoly.cpp -- Jenkins-Traub real-polynomial root finder (see rpoly.hpp).
// Faithful port of the vendored Census RPOLY suite (rpoly/fxshfr/quadit/realit/
// calcsc/nextk/newest/quadsd/quad). The Fortran global.cmn scratch is the
// module-private RpolyState g threaded through the helpers; quad/quadsd are
// pure (argument-only) so they take plain pointers. Fortran 1-based indexing is
// preserved (state arrays carry an unused element 0). Transcendental steps
// (dlog/exp/sqrt) resolve to the same rtools44 libm as the oracle, so they
// match bit-for-bit; the machine constant Eta and the scale factor use a
// gfortran-faithful integer-power helper (dpow_ri) rather than std::pow.
#include "numeric/rpoly.hpp"

#include <cmath>

#include "numeric/numeric.hpp"  // dpeq

namespace x13 {
namespace {

constexpr int PORDER = 36;  // model.prm: 3*PSP, PSP=12
constexpr int PARR = PORDER + 2;  // 1-based arrays, indices 1..PORDER+1

// RPOLY shared scratch (Fortran global.cmn). Private to this translation unit.
struct RpolyState {
    double P0[PARR], Qp[PARR], K[PARR], Qk[PARR], Svk[PARR];
    double Snr, Sni, U, V0, A0, B0, C, D0, A1, A3, A7, E, F, G, H;
    double Szr, Szi, Lzr, Lzi, Eta, Are, Mre;
    int N, N0;
};

// gfortran real(8)**int(4): square-and-multiply, reciprocal for n<0. Used for
// the RPOLY machine constant and the coefficient scale factor so both match the
// oracle exactly (std::pow would differ in the last bit).
double dpow_ri(double base, int n) {
    if (n == 0) return 1.0;
    bool neg = n < 0;
    unsigned u = neg ? static_cast<unsigned>(-n) : static_cast<unsigned>(n);
    double pow = 1.0, x = base;
    for (;;) {
        if (u & 1u) pow *= x;
        u >>= 1;
        if (u)
            x *= x;
        else
            break;
    }
    return neg ? 1.0 / pow : pow;
}

// quadsd.f -- divide p by the quadratic (1,u,v); quotient in q, remainder in a,b.
void quadsd(int nn, double u, double v, const double* p, double* q, double& a,
            double& b) {
    b = p[1];
    q[1] = b;
    a = p[2] - u * b;
    q[2] = a;
    for (int i = 3; i <= nn; ++i) {
        double c = p[i] - u * a - v * b;
        q[i] = c;
        b = a;
        a = c;
    }
}

// quad.f -- zeros of a*z^2 + b1*z + c, overflow-avoiding quadratic formula.
void quad(double a, double b1, double c, double& snr, double& sni, double& lr,
          double& li) {
    constexpr double ZERO = 0.0, TWO = 2.0;
    if (dpeq(a, ZERO)) {
        snr = ZERO;
        if (!dpeq(b1, ZERO)) snr = -c / b1;
        lr = ZERO;
    } else if (!dpeq(c, ZERO)) {
        double b = b1 / TWO;
        double d, e;
        if (std::fabs(b) < std::fabs(c)) {
            e = a;
            if (c < 0.0) e = -a;
            e = b * (b / std::fabs(c)) - e;
            d = std::sqrt(std::fabs(e)) * std::sqrt(std::fabs(c));
        } else {
            e = 1.0 - (a / b) * (c / b);
            d = std::sqrt(std::fabs(e)) * std::fabs(b);
        }
        if (e < ZERO) {  // complex conjugate zeros
            snr = -b / a;
            lr = snr;
            sni = std::fabs(d / a);
            li = -sni;
            return;
        }
        // real zeros
        if (b >= ZERO) d = -d;
        lr = (-b + d) / a;
        snr = ZERO;
        if (!dpeq(lr, ZERO)) snr = (c / lr) / a;
    } else {
        snr = ZERO;
        lr = -b1 / a;
    }
    sni = ZERO;
    li = ZERO;
}

// calcsc.f -- scalars for nextk/newest; Type indicates the normalization.
void calcsc(RpolyState& g, int& type) {
    quadsd(g.N, g.U, g.V0, g.K, g.Qk, g.C, g.D0);
    if (std::fabs(g.C) <= std::fabs(g.K[g.N]) * 100.0 * g.Eta) {
        if (std::fabs(g.D0) <= std::fabs(g.K[g.N - 1]) * 100.0 * g.Eta) {
            type = 3;  // quadratic is almost a factor of K
            return;
        }
    }
    if (std::fabs(g.D0) >= std::fabs(g.C)) {
        type = 2;  // formulas divided by D
        g.E = g.A0 / g.D0;
        g.F = g.C / g.D0;
        g.G = g.U * g.B0;
        g.H = g.V0 * g.B0;
        g.A3 = (g.A0 + g.G) * g.E + g.H * (g.B0 / g.D0);
        g.A1 = g.B0 * g.F - g.A0;
        g.A7 = (g.F + g.U) * g.A0 + g.H;
        return;
    }
    type = 1;  // formulas divided by C
    g.E = g.A0 / g.C;
    g.F = g.D0 / g.C;
    g.G = g.U * g.E;
    g.H = g.V0 * g.B0;
    g.A3 = g.A0 * g.E + (g.H / g.C + g.G) * g.B0;
    g.A1 = g.B0 - g.A0 * (g.D0 / g.C);
    g.A7 = g.A0 + g.G * g.D0 + g.H * g.F;
}

// nextk.f -- next K polynomial from calcsc scalars.
void nextk(RpolyState& g, int type) {
    if (type != 3) {
        double temp = g.A0;
        if (type == 1) temp = g.B0;
        if (std::fabs(g.A1) > std::fabs(temp) * g.Eta * 10.0) {
            g.A7 = g.A7 / g.A1;
            g.A3 = g.A3 / g.A1;
            g.K[1] = g.Qp[1];
            g.K[2] = g.Qp[2] - g.A7 * g.Qp[1];
            for (int i = 3; i <= g.N; ++i)
                g.K[i] = g.A3 * g.Qk[i - 2] - g.A7 * g.Qp[i - 1] + g.Qp[i];
        } else {  // A1 nearly zero: special recurrence
            g.K[1] = 0.0;
            g.K[2] = -g.A7 * g.Qp[1];
            for (int i = 3; i <= g.N; ++i)
                g.K[i] = g.A3 * g.Qk[i - 2] - g.A7 * g.Qp[i - 1];
        }
        return;
    }
    // type 3: unscaled recurrence
    g.K[1] = 0.0;
    g.K[2] = 0.0;
    for (int i = 3; i <= g.N; ++i) g.K[i] = g.Qk[i - 2];
}

// newest.f -- new quadratic coefficient estimates from calcsc scalars.
void newest(RpolyState& g, int type, double& uu, double& vv) {
    constexpr double ONE = 1.0, ZERO = 0.0;
    if (type != 3) {
        double a4, a5;
        if (type == 2) {
            a4 = (g.A0 + g.G) * g.F + g.H;
            a5 = (g.F + g.U) * g.C + g.V0 * g.D0;
        } else {
            a4 = g.A0 + g.U * g.B0 + g.H * g.F;
            a5 = g.C + (g.U + g.V0 * g.F) * g.D0;
        }
        double b1 = -g.K[g.N] / g.P0[g.N0];
        double b2 = -(g.K[g.N - 1] + b1 * g.P0[g.N]) / g.P0[g.N0];
        double c1 = g.V0 * b2 * g.A1;
        double c2 = b1 * g.A7;
        double c3 = b1 * b1 * g.A3;
        double c4 = c1 - c2 - c3;
        double temp = a5 + b1 * a4 - c4;
        if (!dpeq(temp, ZERO)) {
            uu = g.U -
                 (g.U * (c3 + c2) + g.V0 * (b1 * g.A1 + b2 * g.A7)) / temp;
            vv = g.V0 * (ONE + c4 / temp);
            return;
        }
    }
    uu = ZERO;  // type 3, or zero denominator: zero the quadratic
    vv = ZERO;
}

// quadit.f -- variable-shift K-polynomial iteration for a quadratic factor.
void quadit(RpolyState& g, double uu, double vv, int& nz) {
    constexpr double ZERO = 0.0, ONE = 1.0, TWO = 2.0, FOUR = 4.0, FIVE = 5.0,
                     TWNTY = 20.0;
    double ui, vi, mp, omp = 0.0, ee, relstp = 0.0, t, zm;
    int type;
    bool tried = false;
    nz = 0;
    g.U = uu;
    g.V0 = vv;
    int j = 0;
    while (true) {
        quad(ONE, g.U, g.V0, g.Szr, g.Szi, g.Lzr, g.Lzi);
        // Return if the quadratic's roots are real and not close.
        if (std::fabs(std::fabs(g.Szr) - std::fabs(g.Lzr)) >
            0.01 * std::fabs(g.Lzr))
            return;
        quadsd(g.N0, g.U, g.V0, g.P0, g.Qp, g.A0, g.B0);
        mp = std::fabs(g.A0 - g.Szr * g.B0) + std::fabs(g.Szi * g.B0);
        zm = std::sqrt(std::fabs(g.V0));
        ee = TWO * std::fabs(g.Qp[1]);
        t = -g.Szr * g.B0;
        for (int i = 2; i <= g.N; ++i) ee = ee * zm + std::fabs(g.Qp[i]);
        ee = ee * zm + std::fabs(g.A0 + t);
        ee = (FIVE * g.Mre + FOUR * g.Are) * ee -
             (FIVE * g.Mre + TWO * g.Are) *
                 (std::fabs(g.A0 + t) + std::fabs(g.B0) * zm) +
             TWO * g.Are * std::fabs(t);
        if (mp > TWNTY * ee) {
            j = j + 1;
            if (j > 20) return;  // stop after 20 steps
            if (j >= 2) {
                if (!(relstp > 0.01 || mp < omp || tried)) {
                    // Cluster stalling: five fixed shifts near the cluster.
                    if (relstp < g.Eta) relstp = g.Eta;
                    relstp = std::sqrt(relstp);
                    g.U = g.U - g.U * relstp;
                    g.V0 = g.V0 + g.V0 * relstp;
                    quadsd(g.N0, g.U, g.V0, g.P0, g.Qp, g.A0, g.B0);
                    for (int i = 1; i <= 5; ++i) {
                        calcsc(g, type);
                        nextk(g, type);
                    }
                    tried = true;
                    j = 0;
                }
            }
            omp = mp;
            calcsc(g, type);
            nextk(g, type);
            calcsc(g, type);
            newest(g, type, ui, vi);
            if (dpeq(vi, ZERO)) return;  // not converging
            relstp = std::fabs((vi - g.V0) / vi);
            g.U = ui;
            g.V0 = vi;
        } else {
            nz = 2;
            return;
        }
    }
}

// realit.f -- variable-shift H-polynomial iteration for a real zero.
void realit(RpolyState& g, double& sss, int& nz, int& iflag) {
    double pv, kv, t = 0.0, s, ms, mp, omp = 0.0, ee;
    nz = 0;
    s = sss;
    iflag = 0;
    int j = 0;
    while (true) {
        pv = g.P0[1];
        g.Qp[1] = pv;
        for (int i = 2; i <= g.N0; ++i) {
            pv = pv * s + g.P0[i];
            g.Qp[i] = pv;
        }
        mp = std::fabs(pv);
        ms = std::fabs(s);
        ee = (g.Mre / (g.Are + g.Mre)) * std::fabs(g.Qp[1]);
        for (int i = 2; i <= g.N0; ++i) ee = ee * ms + std::fabs(g.Qp[i]);
        if (mp > 20.0 * ((g.Are + g.Mre) * ee - g.Mre * mp)) {
            j = j + 1;
            if (j > 10) return;  // stop after 10 steps
            if (j >= 2) {
                if (std::fabs(t) <= 0.001 * std::fabs(s - t) && mp > omp) {
                    iflag = 1;  // cluster near real axis -> try quadratic
                    sss = s;
                    return;
                }
            }
            omp = mp;
            kv = g.K[1];
            g.Qk[1] = kv;
            for (int i = 2; i <= g.N; ++i) {
                kv = kv * s + g.K[i];
                g.Qk[i] = kv;
            }
            if (std::fabs(kv) <= std::fabs(g.K[g.N]) * 10.0 * g.Eta) {
                g.K[1] = 0.0;  // unscaled form
                for (int i = 2; i <= g.N; ++i) g.K[i] = g.Qk[i - 1];
            } else {
                t = -pv / kv;  // scaled form
                g.K[1] = g.Qp[1];
                for (int i = 2; i <= g.N; ++i) g.K[i] = t * g.Qk[i - 1] + g.Qp[i];
            }
            kv = g.K[1];
            for (int i = 2; i <= g.N; ++i) kv = kv * s + g.K[i];
            t = 0.0;
            if (std::fabs(kv) > std::fabs(g.K[g.N]) * 10.0 * g.Eta) t = -pv / kv;
            s = s + t;
        } else {
            nz = 1;
            g.Szr = s;
            g.Szi = 0.0;
            return;
        }
    }
}

// fxshfr.f -- fixed-shift K-polynomials + convergence tests, then one of the
// variable-shift iterations. The Fortran GO-TO-10/20/30 spaghetti maps to inner
// labels (quad_iter/lin_iter/restore); label 40 is the loop tail.
void fxshfr(RpolyState& g, int l2, int& nz) {
    constexpr double PT25 = 0.25, ZERO = 0.0, ONE = 1.0;
    double svu, svv, ui, vi, s;
    double betav, betas, oss, ovv, ss, vv, ts, tv, ots = 0.0, otv = 0.0, tvv,
                                                     tss;
    int rtype, iflag;
    bool vpass, spass, vtry, stry;
    nz = 0;
    betav = PT25;
    betas = PT25;
    oss = g.Snr;
    ovv = g.V0;
    quadsd(g.N0, g.U, g.V0, g.P0, g.Qp, g.A0, g.B0);
    calcsc(g, rtype);
    for (int j = 1; j <= l2; ++j) {
        nextk(g, rtype);
        calcsc(g, rtype);
        newest(g, rtype, ui, vi);
        vv = vi;
        ss = ZERO;
        if (!dpeq(g.K[g.N], ZERO)) ss = -g.P0[g.N0] / g.K[g.N];
        tv = ONE;
        ts = ONE;
        if (!(j == 1 || rtype == 3)) {
            if (!dpeq(vv, ZERO)) tv = std::fabs((vv - ovv) / vv);
            if (!dpeq(ss, ZERO)) ts = std::fabs((ss - oss) / ss);
            tvv = ONE;
            if (tv < otv) tvv = tv * otv;
            tss = ONE;
            if (ts < ots) tss = ts * ots;
            vpass = tvv < betav;
            spass = tss < betas;
            if (spass || vpass) {
                // At least one sequence converged; save before iterating.
                svu = g.U;
                svv = g.V0;
                for (int i = 1; i <= g.N; ++i) g.Svk[i] = g.K[i];
                s = ss;
                vtry = false;
                stry = false;
                if (spass && ((!vpass) || tss < tvv)) goto lin_iter;
            quad_iter:
                quadit(g, ui, vi, nz);
                if (nz > 0) return;
                // Quadratic iteration failed.
                vtry = true;
                betav = betav * PT25;
                if (stry || (!spass)) goto restore;
                for (int i = 1; i <= g.N; ++i) g.K[i] = g.Svk[i];
            lin_iter:
                realit(g, s, nz, iflag);
                if (nz > 0) return;
                // Linear iteration failed.
                stry = true;
                betas = betas * PT25;
                if (iflag != 0) {
                    ui = -(s + s);
                    vi = s * s;
                    goto quad_iter;
                }
            restore:
                g.U = svu;
                g.V0 = svv;
                for (int i = 1; i <= g.N; ++i) g.K[i] = g.Svk[i];
                if (vpass && (!vtry)) goto quad_iter;
                quadsd(g.N0, g.U, g.V0, g.P0, g.Qp, g.A0, g.B0);
                calcsc(g, rtype);
            }
        }
        // label 40: loop tail
        ovv = vv;
        oss = ss;
        otv = tv;
        ots = ts;
    }
}

}  // namespace

// rpoly.f -- the public entry (see rpoly.hpp).
void rpoly(const double* op, int& degree, double* zeror, double* zeroi,
           bool& fail) {
    constexpr double ZERO = 0.0, TEN = 10.0, ONE = 1.0;
    auto OP = [&](int idx) { return op[idx - 1]; };  // 1-based read of op
    RpolyState g{};  // Fortran global.cmn is zero-initialized (BSS); match it
    double temp[PARR], pt[PARR];
    double t, aa, bb, cc, factor, lo, xmax, xmin, xx, yy, cosr, sinr, xxx, x,
        bnd, xm, ff, df, dx, sc, base, infin, smalno;
    int cnt, nz, i, j, jj, nm1, l;
    bool zerok, goto20;

    base = TEN;
    g.Eta = 0.5 * dpow_ri(base, 1 - 15);  // Fortran .5D0*base**(1-15)
    infin = 1.797e30;
    smalno = 1.0e-38;
    g.Are = g.Eta;
    g.Mre = g.Eta;
    lo = smalno / g.Eta;
    xx = 0.70710678;
    yy = -xx;
    cosr = -0.069756474;
    sinr = 0.99756405;
    fail = false;
    g.N = degree;
    g.N0 = g.N + 1;

    // Algorithm fails if the leading coefficient is zero.
    if (!dpeq(OP(1), ZERO)) {
        // Remove the zeros at the origin, if any.
        while (dpeq(OP(g.N0), ZERO)) {
            j = degree - g.N + 1;
            zeror[j - 1] = ZERO;
            zeroi[j - 1] = ZERO;
            g.N0 = g.N0 - 1;
            g.N = g.N - 1;
        }
        for (i = 1; i <= g.N0; ++i) g.P0[i] = OP(i);
    } else {
        fail = true;
        degree = 0;
        return;
    }

start_one_zero:  // Fortran label 10
    if (g.N > 2) {
        // Largest and smallest moduli of coefficients.
        xmax = ZERO;
        xmin = infin;
        for (i = 1; i <= g.N0; ++i) {
            x = std::fabs(g.P0[i]);
            if (x > xmax) xmax = x;
            if ((!dpeq(x, ZERO)) && x < xmin) xmin = x;
        }
    } else {
        if (g.N < 1) return;
        // Final zero or pair of zeros.
        if (g.N == 2) {
            quad(g.P0[1], g.P0[2], g.P0[3], zeror[degree - 2], zeroi[degree - 2],
                 zeror[degree - 1], zeroi[degree - 1]);
        } else {
            zeror[degree - 1] = -g.P0[2] / g.P0[1];
            zeroi[degree - 1] = ZERO;
        }
        return;
    }
    // Scale if there are large or very small coefficients. NB: this branch
    // carries a Census bug -- with the hardcoded constants above lo~2e-24, so
    // for any polynomial with max|coeff| >= 10 (xmax>=10, skipping the GO TO 20
    // shortcut) sc~2e-24, dpeq(sc,0) is TRUE (absolute threshold 3.834e-20),
    // sc is reset to smalno=1e-38, and the whole polynomial is multiplied by
    // ~1e-37, after which every dpeq(.,0) downstream sees zero and no root is
    // ever found (rpoly returns fail). Ported faithfully: X-13's AR/MA
    // polynomials have constant term 1 and small coefficients, so this branch
    // is never entered in practice. See the rpoly tests for the pinned behavior.
    sc = lo / xmin;
    goto20 = false;
    if (sc <= ONE) {
        if (xmax < TEN)
            goto20 = true;
        else if (dpeq(sc, ZERO))
            sc = smalno;
    } else if (infin / sc < xmax) {
        goto20 = true;
    }
    if (!goto20) {
        l = static_cast<int>(std::log(sc) / std::log(base) + 0.5);
        factor = dpow_ri(base * ONE, l);
        if (!dpeq(factor, ONE)) {
            for (i = 1; i <= g.N0; ++i) g.P0[i] = factor * g.P0[i];
        }
    }
    // label 20: lower bound on moduli of zeros.
    for (i = 1; i <= g.N0; ++i) pt[i] = std::fabs(g.P0[i]);
    pt[g.N0] = -pt[g.N0];
    // Upper estimate of bound.
    x = std::exp((std::log(-pt[g.N0]) - std::log(pt[1])) /
                 static_cast<double>(g.N));
    if (!dpeq(pt[g.N], ZERO)) {
        xm = -pt[g.N0] / pt[g.N];
        if (xm < x) x = xm;  // Newton step at the origin is better
    }
    // Chop the interval (0,x) until ff <= 0.
    while (true) {
        xm = x * 0.1;
        ff = pt[1];
        for (i = 2; i <= g.N0; ++i) ff = ff * xm + pt[i];
        if (ff <= ZERO) break;
        x = xm;
    }
    // label 30: Newton iteration until x converges to two decimals.
    dx = x;
    while (std::fabs(dx / x) > 0.005) {
        ff = pt[1];
        df = ff;
        for (i = 2; i <= g.N; ++i) {
            ff = ff * x + pt[i];
            df = df * x + ff;
        }
        ff = ff * x + pt[g.N0];
        dx = ff / df;
        x = x - dx;
    }
    bnd = x;
    // Derivative as the initial K polynomial; 5 no-shift steps.
    nm1 = g.N - 1;
    for (i = 2; i <= g.N; ++i)
        g.K[i] = static_cast<double>(g.N0 - i) * g.P0[i] /
                 static_cast<double>(g.N);
    g.K[1] = g.P0[1];
    aa = g.P0[g.N0];
    bb = g.P0[g.N];
    zerok = dpeq(g.K[g.N], ZERO);
    for (jj = 1; jj <= 5; ++jj) {
        cc = g.K[g.N];
        if (zerok) {
            // Unscaled form of the recurrence.
            for (i = 1; i <= nm1; ++i) {
                j = g.N0 - i;
                g.K[j] = g.K[j - 1];
            }
            g.K[1] = ZERO;
            zerok = dpeq(g.K[g.N], ZERO);
        } else {
            // Scaled form if K at 0 is nonzero.
            t = -aa / cc;
            for (i = 1; i <= nm1; ++i) {
                j = g.N0 - i;
                g.K[j] = t * g.K[j - 1] + g.P0[j];
            }
            g.K[1] = g.P0[1];
            zerok = std::fabs(g.K[g.N]) <= std::fabs(bb) * g.Eta * TEN;
        }
    }
    // Save K for restarts with new shifts.
    for (i = 1; i <= g.N; ++i) temp[i] = g.K[i];
    // Loop selecting the quadratic for each new shift.
    for (cnt = 1; cnt <= 20; ++cnt) {
        // Point of modulus bnd, amplitude rotated 94 degrees from the last.
        xxx = cosr * xx - sinr * yy;
        yy = sinr * xx + cosr * yy;
        xx = xxx;
        g.Snr = bnd * xx;
        g.Sni = bnd * yy;
        g.U = -2.0 * g.Snr;
        g.V0 = bnd;
        fxshfr(g, 20 * cnt, nz);
        if (nz == 0) {
            // Unsuccessful: restore K and choose another quadratic.
            for (i = 1; i <= g.N; ++i) g.K[i] = temp[i];
        } else {
            // Deflate, store the zero(s), return to the main algorithm.
            j = degree - g.N + 1;
            zeror[j - 1] = g.Szr;
            zeroi[j - 1] = g.Szi;
            g.N0 = g.N0 - nz;
            g.N = g.N0 - 1;
            for (i = 1; i <= g.N0; ++i) g.P0[i] = g.Qp[i];
            if (nz != 1) {
                zeror[j] = g.Lzr;
                zeroi[j] = g.Lzi;
            }
            goto start_one_zero;
        }
    }
    // No convergence with 20 shifts.
    fail = true;
    degree = degree - g.N;
}

}  // namespace x13
