// estdgn.cpp -- see hpp.
#include "diag/estdgn.hpp"

#include "common/x13context.hpp"
#include "regarima/estimate.hpp"   // roots
#include "numeric/numeric.hpp"     // dpmpar
#include "specparse/specparse.hpp" // getstr
#include "gen/model.hpp"

#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace x13 {

void est_diagnostics(X13Context& ctx, bool lidotl) {
    EstDiagnostics& ed = ctx.estdgn;
    ed = EstDiagnostics{};

    const model_cmn& m = ctx.model;

    // ---- savotl.f:79-140 -- the outlier counts ----------------------------
    // Transcribed verbatim, including the two asymmetries: `iall` is
    // incremented INSIDE each type test rather than derived from the others,
    // and the `iauto` (automatically identified) tally counts only the three
    // AUTO types, which is a different set from the ones `iall` accumulates.
    for (int i1 = 1; i1 <= m.nb; ++i1) {
        const int t = m.rgvrtp(i1);
        // savotl.f:82-84 -- the PRGTAS (auto seasonal outlier) arm is COMMENTED
        // OUT in the Fortran, so a seasonal outlier never counts as automatic.
        if (t == prm::PRGTAA || t == prm::PRGTAL || t == prm::PRGTAT)
            ed.autoout += 1;
        if (t == prm::PRGTAA || t == prm::PRGTAO || t == prm::PRGUAO) {
            ed.ao += 1;
            ed.total += 1;
        }
        if (t == prm::PRGTAL || t == prm::PRGTLS || t == prm::PRGULS) {
            ed.ls += 1;
            ed.total += 1;
        }
        if (t == prm::PRGTAT || t == prm::PRGTTC) {
            ed.tc += 1;
            ed.total += 1;
        }
        if (t == prm::PRGTSO || t == prm::PRGUSO) {
            ed.so += 1;
            ed.total += 1;
        }
        if (t == prm::PRGTRP || t == prm::PRGTQD || t == prm::PRGTQI) {
            ed.rp += 1;
            ed.total += 1;
        }
        if (t == prm::PRGTTL) {
            ed.tls += 1;
            ed.total += 1;
        }
        if (ctx.usrreg.ncusrx > 0) {
            if (t == prm::PRGUAO || t == prm::PRGULS || t == prm::PRGUSO)
                ed.user += 1;
        }
    }
    ed.have_user = ctx.usrreg.ncusrx > 0;
    ed.have_autoout = lidotl;

    // ---- prtmdl.f:752-754 -- the differencing shape ----------------------
    {
        const int endopr = m.mdl(prm::MA) - 1;
        ed.nonseasonaldiff = m.nnsedf;
        ed.seasonaldiff = m.nseadf;
        ed.nmodel = (endopr > 0) ? (m.opr(endopr) - m.opr(m.mdl(prm::AR - 1) - 1))
                                 : 0;
    }

    // ---- prtmdl.f:830-934 -- the ARMA coefficient table -------------------
    // prtmdl.f:757-762: the standard-error columns exist only when the model
    // converged with a usable variance; otherwise the oracle writes literal
    // zeros in their place (the `.not.lprtse` branch at :883-894), so this is
    // a value of the row, not an omission.
    ed.have_se = ctx.mdldat.convrg && ctx.mdldat.var > 2.0 * dpmpar(1);
    {
        // isfixd.f keeps `cpntfx`/`oprfix` in SAVEd locals across its three
        // levels: a lag is marked "(fixed)" only when neither the whole
        // component nor its operator was already marked at a coarser level.
        const int begopr_all = m.mdl(prm::AR - 1);
        const int endopr_all = m.mdl(prm::MA) - 1;
        bool cpntfx = false;
        if (endopr_all > 0 && begopr_all <= endopr_all) {
            const int beglag_all = m.opr(begopr_all - 1);
            const int endlag_all = m.opr(endopr_all) - 1;
            cpntfx = true;
            for (int l = beglag_all; l <= endlag_all; ++l)
                cpntfx = cpntfx && m.arimaf(l);
        }
        // prtmdl.f:769 -- an all-fixed model also clears lprtse.
        if (cpntfx) ed.have_se = false;

        int iestpm = 0;
        for (int iflt = prm::AR; iflt <= prm::MA; ++iflt) {
            const int begopr = m.mdl(iflt - 1);
            const int endopr = m.mdl(iflt) - 1;
            for (int iopr = begopr; iopr <= endopr; ++iopr) {
                const int beglag = m.opr(iopr - 1);
                const int endlag = m.opr(iopr) - 1;

                bool oprfix = true;
                for (int l = beglag; l <= endlag; ++l)
                    oprfix = oprfix && m.arimaf(l);
                oprfix = !cpntfx && oprfix && (endlag > beglag);

                std::string tmpttl;
                int ntmpcr = 0;
                getstr(ctx, m.oprttl.data(), m.oprptr.data(), m.noprtl, iopr,
                       tmpttl, ntmpcr);
                if (ctx.error.lfatal) return;
                tmpttl = tmpttl.substr(0, ntmpcr);
                int spchr = ntmpcr;
                while (spchr >= 1 && tmpttl[spchr - 1] != ' ') --spchr;
                if (spchr < 1) spchr = 1;

                for (int ilag = beglag; ilag <= endlag; ++ilag) {
                    const bool lagfix =
                        !cpntfx && !oprfix && m.arimaf(ilag);
                    if (!m.arimaf(ilag)) ++iestpm;

                    ArmaCoef c;
                    // Unlike the roots key, this one keeps the title's own
                    // case: `MA$Nonseasonal$01$01`.
                    c.filter = tmpttl.substr(spchr, ntmpcr - spchr);
                    c.period = tmpttl.substr(0, spchr - 1);
                    while (!c.period.empty() && c.period.back() == ' ')
                        c.period.pop_back();
                    c.factor = m.oprfac(iopr);
                    c.lag = m.arimal(ilag);
                    c.value = ctx.mdldat.arimap(ilag);
                    c.fixed = lagfix;
                    if (ed.have_se && !m.arimaf(ilag)) {
                        c.se = std::sqrt(ctx.mdldat.var *
                                         ctx.mdldat.armacm(iestpm, iestpm));
                        c.t = (c.se != 0.0) ? c.value / c.se : 0.0;
                    }
                    ed.coefs.push_back(c);
                }
            }
        }
    }

    // ---- prtrts.f -- the roots of each ARMA operator ----------------------
    // prtrts.f:44-50: nothing at all when the model carries no ARMA operator.
    {
        int begopr = m.mdl(prm::AR - 1);
        const int endopr_all = m.mdl(prm::MA) - 1;
        if (endopr_all > 0 && begopr <= endopr_all) {
            for (int iflt = prm::AR; iflt <= prm::MA; ++iflt) {
                begopr = m.mdl(iflt - 1);
                const int endopr = m.mdl(iflt) - 1;
                for (int iopr = begopr; iopr <= endopr; ++iopr) {
                    const int beglag = m.opr(iopr - 1);
                    const int endlag = m.opr(iopr) - 1;

                    std::string tmpttl;
                    int ntmpcr = 0;
                    getstr(ctx, m.oprttl.data(), m.oprptr.data(), m.noprtl, iopr,
                           tmpttl, ntmpcr);
                    if (ctx.error.lfatal) return;
                    tmpttl = tmpttl.substr(0, ntmpcr);

                    // prtrts.f:84-89 -- split the operator title on its LAST
                    // blank: the head is the filter ("AR"/"MA"), the tail the
                    // period ("Nonseasonal"/"Seasonal"). Both are lowercased
                    // for the .udg key (prtrts.f:135-141 adds 32 to each char).
                    int spchr = ntmpcr;
                    while (spchr >= 1 && tmpttl[spchr - 1] != ' ') --spchr;
                    if (spchr < 1) spchr = 1;

                    const int factor = m.oprfac(iopr);
                    int degree = ctx.model.arimal(endlag) / factor;
                    std::vector<double> coeff(static_cast<std::size_t>(degree) + 1,
                                              0.0);
                    coeff[0] = -1.0;
                    for (int ilag = beglag; ilag <= endlag; ++ilag)
                        coeff[ctx.model.arimal(ilag) / factor] =
                            ctx.mdldat.arimap(ilag);

                    std::vector<double> zr(static_cast<std::size_t>(degree) + 1, 0.0),
                        zi(zr.size(), 0.0), zm(zr.size(), 0.0), zf(zr.size(), 0.0);
                    bool allinv = true;
                    int deg = degree;
                    roots(ctx, coeff.data(), deg, allinv, zr.data(), zi.data(),
                          zm.data(), zf.data());
                    if (ctx.error.lfatal) return;

                    // prtrts.f:135-142 builds the key as
                    //   'roots.' // <TAIL lowercased> // '.' // <HEAD, first
                    //   char lowercased> // '.' // <ROOT INDEX, i2.2>
                    // The operator title is e.g. "Nonseasonal MA", so the TAIL
                    // is the filter (ma/ar) and the HEAD the period -- which is
                    // the reverse of how the title reads. The trailing number is
                    // the root's index within the operator, NOT Oprfac.
                    std::string per, filt;
                    for (int k = 1; k < spchr; ++k)
                        if (tmpttl[k - 1] != ' ')
                            per += static_cast<char>(
                                std::tolower(static_cast<unsigned char>(tmpttl[k - 1])));
                    for (int k = spchr + 1; k <= ntmpcr; ++k)
                        filt += static_cast<char>(
                            std::tolower(static_cast<unsigned char>(tmpttl[k - 1])));

                    // prtrts.f:104-105 loops over the ORIGINAL degree, not the
                    // (possibly reduced) one `roots` returns -- transcribed.
                    for (int i = 1; i <= degree; ++i) {
                        ArmaRoot r;
                        r.filter = filt;
                        r.period = per;
                        r.factor = factor;
                        r.index = i;
                        r.real = zr[i - 1];
                        r.imag = zi[i - 1];
                        r.modulus = zm[i - 1];
                        r.frequency = zf[i - 1];
                        ed.roots.push_back(r);
                    }
                }
            }
        }
    }

    ed.ran = true;
}

}  // namespace x13
