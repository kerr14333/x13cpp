// parse_spec.cpp -- library entry point for M1: parse a spec-file's text and
// run the spec-parser subsystem (gtinpt), returning the fatal/ok outcome.
//
// This is the M1-reachable slice of the x12run driver: file/unit setup, the
// error-file header (genfor.f), the spec parse + series load, and error
// reporting. Estimation / adjustment / output phases are out of M1 scope.
#include "specparse/specparse.hpp"
#include "x13/fformat.hpp"
#include "common/x13context.hpp"
#include "numeric/numeric.hpp"   // dpeq
#include "gen/notset.hpp"        // prm::DNOTST

namespace x13 {

// Unit-number assignment for the in-memory channel registry (mirrors the units
// the Fortran main opens: Mt input, Mt1 out, Mt2 err, Ng log, Nform udg).
static void setup_units(X13Context& ctx) {
    ctx.units.mt = 5;      // input spec file (informational; source is ctx.lex)
    ctx.units.mt1 = 7;     // main printout
    ctx.units.mt2 = 8;     // error file (.err)
    ctx.units.ng = 9;      // run log
    ctx.units.nform = 10;  // udg diagnostics
    ctx.units.mtm = 11;    // model file
    ctx.units.mtprof = 12; // profiling
}

// genfor.f error-file header.
static void write_err_header(X13Context& ctx, const std::string& infile_name) {
    std::string rec = fwrite_fmt(
        "(5x,'Error messages generated from processing the ',a,' spec file',/,5x,A,':',//)",
        std::string(stdio::PRGNAM), infile_name);
    ctx.channels_.unit(ctx.units.mt2).put(rec + "\n");
}

// parse_spec: infile_name is the spec-file name shown in the .err header
// (e.g. "malformed-unknown-arg.spc").
bool parse_spec(X13Context& ctx, const std::string& spec_text,
                const std::string& infile_name) {
    setup_units(ctx);
    ctx.lex.load(spec_text);
    write_err_header(ctx, infile_name);

    bool lx11 = false, lseats = false, lmodel = false, inptok = true;
    gtinpt(ctx, lx11, lseats, lmodel, inptok);
    ctx.captured.has_model = lmodel;

    // editor.f:429-434 -- transform{constant=} shifts the WHOLE input series up
    // by the constant, before anything reads it (the transform, the model, the
    // X-11 spine all work on Y+c; the constant is taken back out at x11pt3's
    // output points). Missing values are left alone. This port has no separate
    // editor stage, and Y is not read before here, so this is the one place that
    // is unambiguously "after gtinpt, before everything else".
    if (inptok && !ctx.error.lfatal && !dpeq(ctx.adj.cnstnt, prm::DNOTST)) {
        const double c = ctx.adj.cnstnt;
        const double mv = ctx.missng.mvcode;
        for (int i = 1; i <= ctx.arima.nobs; ++i)
            if (!dpeq(ctx.arima.y(i), mv)) ctx.arima.y(i) += c;
    }

    // editor.f:389-401 -- SEATS needs a longer forecast horizon than the user
    // may have asked for, so an EXPLICIT `forecast{maxlead=}` below
    // max(12,3*Sp) is silently RAISED to it (the oracle writes a NOTE saying
    // so). gtinpt.f:1151 already applies the same floor, but only as the
    // DEFAULT when no maxlead was given -- which is why this was invisible:
    // every corpus spec that asks for a short horizon alongside seats{}
    // (`*_fixed-airline-seats`, `*_finite-seats`, `airline_seats-qmax-rmod`,
    // `airline_seats-tabtables`, all `forecast{maxlead=12}`) came out with
    // `nfcst: 12` against the golden's `nfcst: 36`, and nothing downstream of
    // the HISTORICAL decomposition read it. The forecast decomposition
    // (tfd/sfd/afd/yfd) does: it punched 24 rows against the golden's 36.
    // Placed here rather than in gtinpt because gtinpt's own rule is keyed on
    // "not set" and must stay that way; the pointer bookkeeping the Fortran
    // does alongside (Posffc/Nobspf/Nofpob/setxpt) is derived from Nfcst later
    // in this port, so raising Nfcst before anything reads it is equivalent.
    if (inptok && !ctx.error.lfatal && ctx.captured.has_seats) {
        const int ip1 = std::max(12, 3 * ctx.model.sp);
        if (ctx.extend.nfcst >= 0 && ctx.extend.nfcst < ip1) {
            ctx.extend.nfcst = ip1;
            // :396-406 -- and the oracle SAYS so. The raise was ported in
            // isolation and the three lines under it were not, which is the
            // shape this file's comment above already describes: a silent
            // horizon change is exactly the thing the NOTE exists to stop
            // being silent. `clen` is CHARACTER*(4) and itoc writes into it
            // at a 1-based position, so the buffer is padded to 4 and cut
            // back to what itoc actually wrote -- no zero padding, no width.
            std::string clen(4, ' ');
            int ip2 = 1;
            itoc(ctx, ip1, clen, ip2);
            if (ctx.error.lfatal) return false;
            clen.resize(static_cast<std::size_t>(ip2 - 1));
            writln(ctx,
                   "NOTE: A longer forecast horizon is required by the SEATS "
                   "signal extraction",
                   ctx.units.mt1, ctx.units.mt2, true);
            writln(ctx,
                   "      procedure, so the number of forecasts generated by "
                   "this run has",
                   ctx.units.mt1, ctx.units.mt2, false);
            writln(ctx, "      been changed to " + clen + ".",
                   ctx.units.mt1, ctx.units.mt2, false);
        }
    }

    // editor.f:517-518 -- on a run with NO x11{} (SEATS, or model-only) that is
    // not taking a log, the adjustment mode is forced ADDITIVE, overriding the
    // multiplicative default gtinpt.f:956 just resolved. gtinpt cannot do this
    // itself: its own rule keys on Fcntyp alone and cannot see Lx11, so a
    // no-transform SEATS spec comes out of the parser with Muladd 0.
    //
    // NOT print surface. genqs/gennpsa re-centre a ratio irregular by
    // subtracting one under `Muladd != 1`, and a SEATS decomposition without a
    // log produces an ADDITIVE irregular centred on zero -- so the missing rule
    // shifted the whole series to about -1 and turned a QS of 0.00000 into
    // 1460.26951 on every `unrate_*-seats` spec. It reaches divsub/addmul on
    // this path too.
    //
    // Tmpma is deliberately NOT updated: gtinpt.f:970 sets it at parse time from
    // the pre-editor value and editor.f:518 touches only Muladd.
    if (inptok && !ctx.error.lfatal && !ctx.captured.has_x11) {
        const int fcntyp = ctx.arima.fcntyp;
        if (fcntyp == 4 || fcntyp == 0 || dpeq(ctx.arima.lam, 1.0))
            ctx.x11opt.muladd = 1;
    }
    return inptok && !ctx.error.lfatal;
}

} // namespace x13
