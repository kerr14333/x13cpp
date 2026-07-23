#ifndef X13_X11_X11EASTER_HPP
#define X13_X11_X11EASTER_HPP

namespace x13 {

struct X13Context;

// holday.f -- X-11 (classic) Easter holiday estimation. Given the preliminary
// irregular Sti (from the transparent x11 pass), fills ctx.x11fac.x11hol with
// the Easter holiday factors (as ratios) and sets Khol=2 so the downstream
// prior-calendar fold applies them. iforc = Nfcst, xdsp = Xdsp. Reads/writes
// ctx.xeastr (Yhol/Xhol/Ieast/Lgenx) and the ctx.x11ptr window.
void holday(X13Context& ctx, const double* sti, int iforc, int xdsp);

}  // namespace x13

#endif  // X13_X11_X11EASTER_HPP
