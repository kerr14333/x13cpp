// x11_prestage.hpp -- the X-11 PRE-STAGE: everything x11ari.f runs before the
// decomposition branch splits.
//
// The oracle has ONE adjustment entry point. x11ari.f is reached with Lx11 or
// Lseats set, runs the editor's span/filter setup, setxpt, x11int, x11pt1 and
// -- at :199, gated `(.not.Lcmpaq).or.Lx11`, i.e. true for a non-composite run
// either way -- x11pt2, and only THEN branches: :204-243 substitutes the SEATS
// chain for x11pt3.
//
// This port grew two separate drivers instead, and run_seats skipped the whole
// pre-stage. That is invisible for the decomposition itself (SEATS reads
// ctx.series.tsrs and the fitted model, not the X-11 buffers) but it leaves the
// span GEOMETRY unset -- Length (x11pt2), Pos1ob (setxpt), Lyr (editor.f:235)
// and the ssprep snapshot -- which is exactly what slidingspans{}/history{}
// need, so both were silently dropped on a SEATS spec. Sharing the block puts
// the two drivers back on the oracle's single path.
#ifndef X13_DRIVER_X11_PRESTAGE_HPP
#define X13_DRIVER_X11_PRESTAGE_HPP

#include <string>
#include <vector>

namespace x13 {

struct X13Context;

// x11ari.f:60-199 (via editor.f's post-parse setup). `trnsrs` is the clean
// transformed series run_m2_after_parse captured; it is read only on the model
// path. `lseats` and `lx11` are the Lseats/Lx11 arguments x11pt1/x11pt2/chkadj
// take -- they do NOT branch the decomposition, which stays with the caller.
//
// They are INDEPENDENT, not complementary: x12run.f:181 reaches x11ari with
// BOTH false, on a spec that asks for no adjustment at all. x11pt2 still runs
// there (`:199` is gated `(.not.Lcmpaq).or.Lx11`, true for any non-composite
// run), and the diagnostics after it -- genqs, spcdrv, gennpsa -- are what that
// path exists to produce.
// Returns false on a fatal (ctx.error.lfatal set).
bool x11_prestage(X13Context& ctx, bool has_model, std::vector<double>& trnsrs,
                  bool lseats, bool lx11);

// editor.f:206-233 + :851 -- the padded X-11 buffer geometry (Begbak/Nbcst2/
// Begbk2, Nfdrp/Nobspf/Nofpob/Nbfpob/Lsp), the setxpt span pointers derived
// from it, and Setpri. Split out of x11_prestage because the MODEL path needs
// it twice: the oracle runs it in the editor, BEFORE arima, which is what keeps
// tdaic.f:600-623's direct Sprior write alive, and then re-derives the pointers
// alone at x11ari.f:149. `lsadj` is Lx11.or.Lseats (it only reaches Nfdrp and
// setxpt's forecast-drop clause). `set_setpri` must be true in exactly one
// caller per run -- whichever one is standing in for the editor.
void x11_editor_geometry(X13Context& ctx, bool lsadj, bool set_setpri);

}  // namespace x13

#endif  // X13_DRIVER_X11_PRESTAGE_HPP
