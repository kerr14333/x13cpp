// loadxr.cpp -- loadxr.f (regARIMA <-> x11-regression model swap).
#include "x11/loadxr.hpp"

#include "common/x13context.hpp"

namespace x13 {

// gtinpt.f:805-816: delete every regressor group before gtxreg parses the
// x11regression variables into a cleared model. (The Xy-column compaction dlrgef
// does is a no-op once Nb hits 0; a model that carries real regressors -- e.g.
// outliers -- is the deferred general case.)
model_design_backup capture_model_design(const X13Context& ctx) {
    const auto& m = ctx.model;
    model_design_backup b;
    b.ngrp = m.ngrp; b.ngrptl = m.ngrptl; b.ncxy = m.ncxy; b.nb = m.nb;
    b.ncoltl = m.ncoltl; b.iregfx = m.iregfx;
    b.nrxy = ctx.arima.nrxy; b.ncusrx = ctx.usrreg.ncusrx;
    b.nrusrx = ctx.arima.nrusrx; b.fulltd = ctx.picktd.fulltd;
    b.colttl = m.colttl.raw(); b.grpttl = m.grpttl.raw();
    b.usrttl = ctx.usrreg.usrttl.raw();
    b.colptr.resize(prm::PB + 1);
    b.grp.resize(prm::PGRP + 1);
    b.grpptr.resize(prm::PGRP + 1);
    b.usrptr.resize(prm::PUREG + 1);
    for (int i = 0; i <= prm::PB; ++i) b.colptr[i] = m.colptr(i);
    for (int i = 0; i <= prm::PGRP; ++i) { b.grp[i] = m.grp(i); b.grpptr[i] = m.grpptr(i); }
    for (int i = 0; i <= prm::PUREG; ++i) b.usrptr[i] = ctx.usrreg.usrptr(i);
    b.rgvrtp.resize(prm::PB + 1);
    b.regfx.resize(prm::PB + 1);
    b.b.resize(prm::PB + 1);
    b.usrtyp.resize(prm::PUREG + 1);
    for (int i = 1; i <= prm::PB; ++i) {
        b.rgvrtp[i] = m.rgvrtp(i);
        b.regfx[i] = static_cast<char>(m.regfx(i));
        b.b[i] = ctx.mdldat.b(i);
    }
    for (int i = 1; i <= prm::PUREG; ++i) b.usrtyp[i] = ctx.usrreg.usrtyp(i);
    return b;
}

void restore_model_design(X13Context& ctx, const model_design_backup& b) {
    auto& m = ctx.model;
    m.ngrp = b.ngrp; m.ngrptl = b.ngrptl; m.ncxy = b.ncxy; m.nb = b.nb;
    m.ncoltl = b.ncoltl; m.iregfx = b.iregfx;
    ctx.arima.nrxy = b.nrxy; ctx.usrreg.ncusrx = b.ncusrx;
    ctx.arima.nrusrx = b.nrusrx; ctx.picktd.fulltd = b.fulltd;
    m.colttl = b.colttl; m.grpttl = b.grpttl;
    ctx.usrreg.usrttl = b.usrttl;
    for (int i = 0; i <= prm::PB; ++i) m.colptr(i) = b.colptr[i];
    for (int i = 0; i <= prm::PGRP; ++i) { m.grp(i) = b.grp[i]; m.grpptr(i) = b.grpptr[i]; }
    for (int i = 0; i <= prm::PUREG; ++i) ctx.usrreg.usrptr(i) = b.usrptr[i];
    for (int i = 1; i <= prm::PB; ++i) {
        m.rgvrtp(i) = b.rgvrtp[i];
        m.regfx(i) = b.regfx[i] != 0;
        ctx.mdldat.b(i) = b.b[i];
    }
    for (int i = 1; i <= prm::PUREG; ++i) ctx.usrreg.usrtyp(i) = b.usrtyp[i];
}

void xrg_clear_working(X13Context& ctx) {
    auto& m = ctx.model;
    if (m.nb <= 0) return;
    m.nb = 0;
    m.ncxy = 1;
    m.ngrp = 0;
    m.ngrptl = 0;
    m.ncoltl = 0;
    for (int i = 0; i <= 80; ++i) { m.grp(i) = 0; m.grpptr(i) = 0; m.colptr(i) = 0; }
    for (int i = 1; i <= 80; ++i) { m.rgvrtp(i) = 0; m.regfx(i) = false; }
    m.colttl = "";
    m.grpttl = "";
    if (ctx.picktd.fulltd) ctx.picktd.fulltd = false;  // gtinpt.f:815
}

void loadxr(X13Context& ctx, bool toxreg) {
    auto& m = ctx.model;
    auto& x = ctx.xrgmdl;
    auto& ar = ctx.arima;
    if (toxreg) {
        // Toxreg=T: working regARIMA model -> x11reg store.
        x.nxgrp = m.ngrp;
        x.ngrptx = m.ngrptl;
        x.nxcxy = m.ncxy;
        x.nbx = m.nb;
        x.priadx = ctx.prior.priadj;
        x.ncoltx = m.ncoltl;
        x.colttx = m.colttl;
        x.grpttx = m.grpttl;
        x.clxptr = m.colptr;
        x.grpx = m.grp;
        x.gpxptr = m.grpptr;
        x.rgxvtp = m.rgvrtp;
        x.bx = ctx.mdldat.b;
        x.bgxusx = ar.bgusrx;
        x.nxrxy = ar.nrxy;
        x.xbegxy = ar.begxy;
        x.irgxfx = m.iregfx;
        x.regfxx = m.regfx;
        x.usrxfx = m.userfx;
        x.xeasid = m.easidx;
        x.pckxtd = ctx.picktd.picktd;
        x.xtddat = ctx.picktd.tddate;
        x.xtdzro = ctx.picktd.tdzero;
        x.xrgmtd = ctx.picktd.lrgmtd;
        x.fulxtd = ctx.picktd.fulltd;
        return;
    }
    // Toxreg=F: x11reg store -> working regARIMA model.
    m.ngrp = x.nxgrp;
    m.ngrptl = x.ngrptx;
    m.ncxy = x.nxcxy;
    m.nb = x.nbx;
    ctx.prior.priadj = x.priadx;
    m.ncoltl = x.ncoltx;
    m.colttl = x.colttx;
    m.grpttl = x.grpttx;
    ctx.x11adj.nusrrg = x.nusxrg;
    m.colptr = x.clxptr;
    m.grp = x.grpx;
    m.grpptr = x.gpxptr;
    m.rgvrtp = x.rgxvtp;
    ctx.usrreg.usrtyp = ctx.usrxrg.usxtyp;
    ctx.usrreg.usrptr = ctx.usrxrg.usrxpt;
    ctx.usrreg.usrttl = ctx.usrxrg.usrxtt;
    ctx.mdldat.b = x.bx;
    ar.userx = x.xuserx;
    ar.nrusrx = ctx.usrxrg.nrxusx;
    ctx.usrreg.ncusrx = ctx.usrxrg.ncxusx;
    ar.bgusrx = x.bgxusx;
    ar.nrxy = x.nxrxy;
    ar.begxy = x.xbegxy;
    m.iregfx = x.irgxfx;
    m.regfx = x.regfxx;
    m.userfx = x.usrxfx;
    ctx.picktd.picktd = x.pckxtd;
    m.easidx = x.xeasid;
    ctx.picktd.tddate = x.xtddat;
    ctx.picktd.tdzero = x.xtdzro;
    ctx.picktd.lrgmtd = x.xrgmtd;
    ctx.picktd.fulltd = x.fulxtd;
    // loadxr.f:98-104: clear ARIMA-operator state (x11reg is regression-only).
    m.lma = false;
    m.lar = false;
    m.nintvl = 0;
    m.nextvl = 0;
    m.mxdflg = 0;
    m.mxarlg = 0;
    m.mxmalg = 0;
}

}  // namespace x13
