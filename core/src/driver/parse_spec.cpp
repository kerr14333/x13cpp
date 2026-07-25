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
    return inptok && !ctx.error.lfatal;
}

} // namespace x13
