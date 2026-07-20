// idmodel.cpp -- chkrt1.f / chkurt.f: root checks over the current model's AR
// and MA operators. Both mirror the operator-walk in estimate.cpp's setmdl root
// loop (build each operator's polynomial in increasing powers, call roots), then
// classify each root's modulus. See idmodel.hpp for the returned quantities.
#include "automdl/idmodel.hpp"

#include "numeric/numeric.hpp"       // (dpeq etc, via roots' guards)
#include "regarima/estimate.hpp"     // roots
#include "specparse/specparse.hpp"   // setdp
#include "gen/model.hpp"             // prm::AR, prm::MA, prm::PORDER
#include "gen/notset.hpp"            // prm::DNOTST

namespace x13 {

void chkrt1(X13Context& ctx, int& irunit, int& isunit, double& rmaxr,
            double& rmaxs, bool& linv, double ublim) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    irunit = 0;
    isunit = 0;
    rmaxr = prm::DNOTST;
    rmaxs = prm::DNOTST;

    int endopr = m.mdl(prm::MA) - 1;
    if (endopr <= 0) return;

    int iflt = prm::AR;
    int begopr = m.mdl(iflt - 1);
    endopr = m.mdl(iflt) - 1;
    if (begopr > endopr) return;

    double coef[prm::PORDER + 1], zeror[prm::PORDER], zeroi[prm::PORDER],
        zerom[prm::PORDER], zerof[prm::PORDER];

    for (int iopr = begopr; iopr <= endopr; ++iopr) {
        int beglag = m.opr(iopr - 1);
        int endlag = m.opr(iopr) - 1;
        int factor = m.oprfac(iopr);
        int degree = m.arimal(endlag) / factor;
        coef[0] = -1.0;
        setdp(0.0, degree, coef + 1);
        setdp(0.0, prm::PORDER, zeror);
        setdp(0.0, prm::PORDER, zeroi);
        setdp(0.0, prm::PORDER, zerom);
        for (int ilag = beglag; ilag <= endlag; ++ilag)
            coef[m.arimal(ilag) / factor] = d.arimap(ilag);
        bool allinv = false;
        roots(ctx, coef, degree, allinv, zeror, zeroi, zerom, zerof);
        if (ctx.error.lfatal) return;
        linv = linv && allinv;
        for (int i = 0; i < degree; ++i) {
            if (zerom[i] <= ublim && zeroi[i] <= 5.0e-2 && zeror[i] > 0.0) {
                if (factor == 1)
                    ++irunit;
                else
                    ++isunit;
            } else if (zeroi[i] <= 2.0e-2 && zeror[i] > 0.0) {
                double zmi = 1.0 / zerom[i];
                if (factor == 1) {
                    if (zmi > rmaxr) rmaxr = zmi;
                } else {
                    if (zmi > rmaxs) rmaxs = zmi;
                }
            }
        }
    }
}

void chkurt(X13Context& ctx, int& urpr, int& urps, int& urqr, int& urqs) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    urpr = 0;
    urps = 0;
    urqr = 0;
    urqs = 0;

    int endopr = m.mdl(prm::MA) - 1;
    if (endopr <= 0) return;

    const double modlim = 1.0 / 0.95;

    double coef[prm::PORDER + 1], zeror[prm::PORDER], zeroi[prm::PORDER],
        zerom[prm::PORDER], zerof[prm::PORDER];

    for (int iflt = prm::AR; iflt <= prm::MA; ++iflt) {
        int begopr = m.mdl(iflt - 1);
        endopr = m.mdl(iflt) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = m.opr(iopr - 1);
            int endlag = m.opr(iopr) - 1;
            int factor = m.oprfac(iopr);
            int degree = m.arimal(endlag) / factor;
            coef[0] = -1.0;
            setdp(0.0, degree, coef + 1);
            setdp(0.0, prm::PORDER, zeror);
            setdp(0.0, prm::PORDER, zeroi);
            setdp(0.0, prm::PORDER, zerom);
            for (int ilag = beglag; ilag <= endlag; ++ilag)
                coef[m.arimal(ilag) / factor] = d.arimap(ilag);
            bool allinv = false;
            roots(ctx, coef, degree, allinv, zeror, zeroi, zerom, zerof);
            if (ctx.error.lfatal) return;
            for (int i = 0; i < degree; ++i) {
                if (zerom[i] <= modlim) {
                    if (factor == 1) {
                        if (iflt == prm::AR)
                            ++urpr;
                        else
                            ++urqr;
                    } else {
                        if (iflt == prm::AR)
                            ++urps;
                        else
                            ++urqs;
                    }
                }
            }
        }
    }
}

}  // namespace x13
