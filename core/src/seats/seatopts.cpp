// seatopts.cpp -- see seatopts.hpp.
#include "seats/seatopts.hpp"
#include "gen/notset.hpp"
#include "numeric/numeric.hpp"

namespace x13 {

SeatsOptions seats_resolve_options(const X13Context& ctx) {
    const seatop_cmn& o = ctx.seatop;
    SeatsOptions r;
    if (!dpeq(o.rmod2, prm::DNOTST)) r.rmod = o.rmod2;
    if (!dpeq(o.epsph2, prm::DNOTST)) r.epsphi = o.epsph2;
    if (!dpeq(o.xl2, prm::DNOTST)) r.xl = o.xl2;
    if (!dpeq(o.epsiv2, prm::DNOTST)) r.epsiv = o.epsiv2;
    if (o.maxit2 != prm::NOTSET) r.maxit = o.maxit2;
    if (o.qmax2 != prm::NOTSET) r.qmax = o.qmax2;
    return r;
}

}  // namespace x13
