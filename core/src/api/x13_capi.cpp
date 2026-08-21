// x13_capi.cpp -- implementation of the flat C ABI (x13/capi.h).
//
// This is the only translation unit that both (a) drives the engine and (b) is
// visible to non-C++ callers, so it is where every Fortran-shaped thing stops:
// 1-based farrays, the x11ptr span pointers, `Lfatal`, and C++ exceptions all
// terminate here and what leaves is plain C.
//
// The table registry below deliberately mirrors tools/x13run_x11.cpp's `dump()`
// call sequence, INCLUDING its per-table punch ranges. Those ranges are not
// cosmetic -- d10's seasonal factors are projected a year past the data, b1 and
// e18 widen under x11{appendfcst}/{appendbcst}, and ffc runs to the
// forecast-extended `lstfrc`. The parity harness is the reference for what the
// oracle actually writes for each table, so anything that disagrees with it
// would be handing callers a differently-shaped series than the gated one. Keep
// the two in step.
#include "x13/capi.h"

#include <float.h>     // _control87 / _PC_64 / _MCW_PC (see FpPrecisionGuard)

#include <cstdio>
#include <cstring>
#include <exception>
#include <memory>
#include <new>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#define x13_capi_chdir _chdir
#define x13_capi_getcwd _getcwd
#else
#include <unistd.h>
#define x13_capi_chdir chdir
#define x13_capi_getcwd getcwd
#endif

#include "x13/version.hpp"
#include "common/x13context.hpp"
#include "gen/notset.hpp"   // x13::prm::DNOTST (genqs / x11opt "never computed" sentinel)
#include "specparse/specparse.hpp"

namespace {

// One named output series, already flattened to chronological order with its own
// calendar anchor (tables do not share a start date -- see capi.h).
struct Table {
    std::string name;
    int startYear = 0;
    int startPeriod = 0;
    std::vector<double> values;
};

struct Diag {
    std::string name;
    double value = 0.0;
};

// A numeric result that is NOT a time series: the spectrum grid and the spectra
// on it, peak probabilities, per-period filter codes. Kept apart from Table
// because Table's contract is that every element has a year/period.
struct Vector {
    std::string name;
    std::vector<double> values;
};

struct Text {
    std::string name;
    std::string value;
};

}  // namespace

// The handle. Holds only extracted VALUES, never the engine context: the context
// is ~25k of COMMON blocks and there is no reason to keep it alive once the
// numbers are out.
struct x13_run {
    bool ok = false;
    std::string error;

    int period = 0;
    int nobs = 0;
    bool modelBased = false;
    int mode = 0;
    std::string arimaModel;

    std::vector<Table> tables;
    std::vector<Diag> diags;
    std::vector<Vector> vectors;
    std::vector<Text> texts;

    const Table* find(const char* name) const {
        if (!name) return nullptr;
        for (const auto& t : tables)
            if (t.name == name) return &t;
        return nullptr;
    }

    const Vector* findVector(const char* name) const {
        if (!name) return nullptr;
        for (const auto& v : vectors)
            if (v.name == name) return &v;
        return nullptr;
    }

    const Text* findText(const char* name) const {
        if (!name) return nullptr;
        for (const auto& t : texts)
            if (t.name == name) return &t;
        return nullptr;
    }
};

namespace {

// Pull [first,last] out of a 1-based engine buffer, dating each observation from
// `begspn` by its offset from `anchor`. This is dump()'s arithmetic: `anchor` is
// separate from `first` precisely because several tables start BEFORE the span
// (backcasts) or after it, and dating them from their own first row would slide
// every label (the bug x13run_x11's dump() had for appendbcst).
void addTable(x13_run& h, const char* name, const int* begspn, int sp, int first,
              int last, const double* oneBased, int anchor) {
    if (!oneBased || last < first) return;
    Table t;
    t.name = name;
    int d[2];
    x13::addate(begspn, sp, first - anchor, d);
    t.startYear = d[0];
    t.startPeriod = d[1];
    t.values.reserve(static_cast<std::size_t>(last - first + 1));
    for (int i = first; i <= last; ++i) t.values.push_back(oneBased[i - 1]);
    h.tables.push_back(std::move(t));
}

// Same, for the already-0-based-from-pos1ob vectors a few features leave on ctx.
void addTableVec(x13_run& h, const char* name, const int* begspn, int sp,
                 const std::vector<double>& v) {
    if (v.empty()) return;
    Table t;
    t.name = name;
    int d[2];
    x13::addate(begspn, sp, 0, d);
    t.startYear = d[0];
    t.startPeriod = d[1];
    t.values = v;
    h.tables.push_back(std::move(t));
}

// The .err channel (Mt2): every message the engine emitted this run, which
// is where a spec conflict explains itself. Blank lines and the boilerplate
// header the oracle prints are stripped, leaving the messages themselves.
std::string errorText(x13::X13Context& ctx) {
    std::string raw;
    try {
        raw = ctx.channels_.unit(ctx.units.mt2).str();
    } catch (...) {
        return std::string();
    }
    std::string out;
    std::size_t i = 0;
    while (i < raw.size()) {
        std::size_t j = raw.find('\n', i);
        if (j == std::string::npos) j = raw.size();
        std::string line = raw.substr(i, j - i);
        i = j + 1;
        std::size_t b = line.find_first_not_of(" \t\r");
        if (b == std::string::npos) continue;
        std::size_t e = line.find_last_not_of(" \t\r");
        line = line.substr(b, e - b + 1);
        // The two-line banner the error file opens with says nothing a caller
        // who already knows they called X-13 does not know.
        if (line.rfind("Error messages generated from processing", 0) == 0)
            continue;
        if (!line.empty() && line.back() == ':' &&
            line.find(".spc") != std::string::npos)
            continue;
        if (!out.empty()) out += ' ';
        out += line;
    }
    return out;
}

void addDiag(x13_run& h, const std::string& name, double v) {
    h.diags.push_back(Diag{name, v});
}

// Same shape as addTableVec, but for values with no calendar at all.
void addVector(x13_run& h, const std::string& name, const std::vector<double>& v) {
    if (v.empty()) return;
    h.vectors.push_back(Vector{name, v});
}

void addText(x13_run& h, const std::string& name, const std::string& v) {
    if (v.empty()) return;
    h.texts.push_back(Text{name, v});
}

// A dated series that starts `offset` observations after the table anchor --
// the forecasts, which begin one period past the end of the span.
void addTableAt(x13_run& h, const char* name, const int* begspn, int sp,
                int offset, const std::vector<double>& v) {
    if (v.empty()) return;
    Table t;
    t.name = name;
    int d[2];
    x13::addate(begspn, sp, offset, d);
    t.startYear = d[0];
    t.startPeriod = d[1];
    t.values = v;
    h.tables.push_back(std::move(t));
}

// genqs statistics are DNOTST when their series was never formed. Each name has
// TWO values, not one: genqs runs every test twice, over the whole span and
// over the shortened span spectrum{start=} asks for (genqs.cpp both_spans), and
// the short one is only present when that argument was given. Neither is a
// p-value -- QS is chi-square on 2 df, so a caller derives it.
void addQs(x13_run& h, const char* name, double full, double span) {
    if (full != x13::prm::DNOTST) addDiag(h, std::string("qs.") + name, full);
    if (span != x13::prm::DNOTST)
        addDiag(h, std::string("qs.") + name + ".span", span);
}

// x11opt.Lterm / Lter codes, as the seasonalma dictionary orders them
// (readers_spec.cpp:431). 6 (msr) is resolved to 1..3 before harvest, and 0
// (x11default) means the engine never had to pick one.
const char* seasonalMaLabel(int code) {
    switch (code) {
    case 1: return "3x3";
    case 2: return "3x5";
    case 3: return "3x9";
    case 4: return "3x15";
    case 5: return "stable";
    case 6: return "msr";
    case 7: return "3x1";
    default: return "";
    }
}

// The parts of a run that are neither X-11 nor SEATS specific: the spectrum
// block (computed on every monthly run, spectrum{} or not), the QS seasonality
// statistics and the forecasts.
void harvestCommon(x13_run& h, const x13::X13Context& ctx) {
    const int sp = ctx.model.sp;
    const int* begspn = ctx.mdldat.begspn.data();
    const x13::SpectrumOutput& s = ctx.spcout;

    // --- spectra ---------------------------------------------------------
    if (s.ran) {
        addVector(h, "spectrum.freq", s.frq);
        if (s.have_sp0) addVector(h, "spectrum.sp0", s.sp0);
        if (s.have_sp1) addVector(h, "spectrum.sp1", s.sp1);
        if (s.have_sp2) addVector(h, "spectrum.sp2", s.sp2);
        addVector(h, "spectrum.tukey.freq", s.frq_tukey);
        if (s.have_st0) addVector(h, "spectrum.st0", s.st0);
        if (s.have_st1) addVector(h, "spectrum.st1", s.st1);
        if (s.have_st2) addVector(h, "spectrum.st2", s.st2);
        addText(h, "spectrum.peaks.seas", s.peaks_seas);
        addText(h, "spectrum.peaks.td", s.peaks_td);
        addText(h, "spectrum.tukey.peaks.seas", s.tukey_labels.seas);
        addText(h, "spectrum.tukey.peaks.td", s.tukey_labels.td);
        addText(h, "spectrum.tukey.peaks.p90.seas", s.tukey_labels.p90_seas);
        addText(h, "spectrum.tukey.peaks.p90.td", s.tukey_labels.p90_td);
    }
    // spr is filed in the estimation phase, so it survives a run with no
    // decomposition and is published on its own flag.
    if (s.have_spr) {
        if (h.findVector("spectrum.freq") == nullptr)
            addVector(h, "spectrum.freq", s.frq);
        addVector(h, "spectrum.spr", s.spr);
    }

    // The AR-spectrum peak heights, under the oracle savelog names
    // (spcori.s1.stars, spcsa.t1.stars, ...) so a caller that knows the .udg
    // knows these.
    auto emitPeaks = [&h](const x13::SpecPeaks& pk) {
        if (pk.prefix.empty()) return;
        addDiag(h, pk.prefix + ".median", pk.median);
        addDiag(h, pk.prefix + ".range", pk.range);
        const std::vector<x13::SpecPeakRow>* rows[2] = {&pk.seas, &pk.td};
        for (const auto* rowset : rows)
            for (const auto& r : *rowset) {
                if (r.nopeak) continue;
                addDiag(h, pk.prefix + "." + r.label + ".stars", r.stars);
            }
    };
    for (const auto& pk : s.peaks) emitPeaks(pk);
    if (s.have_spr_peaks) emitPeaks(s.spr_peaks);

    // Tukey peak probabilities: trading day first, then seasonal 1..6, which is
    // the order the .udg prints them in.
    auto tukeyVec = [](const x13::TukeyPeaks& t) {
        std::vector<double> v;
        v.push_back(t.ptd);
        for (int i = 0; i < 6; ++i) v.push_back(t.ps[i]);
        return v;
    };
    for (const auto& e : s.tukey)
        if (e.pk.ok) addVector(h, "spectrum.tukey.p." + e.label, tukeyVec(e.pk));
    if (s.have_spr_tukey && s.spr_tukey.ok &&
        h.findVector("spectrum.tukey.p.rsd") == nullptr)
        addVector(h, "spectrum.tukey.p.rsd", tukeyVec(s.spr_tukey));

    // --- QS seasonality ---------------------------------------------------
    if (ctx.qs.ran) {
        addQs(h, "ori", ctx.qs.qsori, ctx.qs.qsoris);
        addQs(h, "orievadj", ctx.qs.qsori2, ctx.qs.qsoris2);
        addQs(h, "rsd", ctx.qs.qsrsd, ctx.qs.qsrsd2);
        addQs(h, "sadj", ctx.qs.qssadj, ctx.qs.qssadjs);
        addQs(h, "sadjevadj", ctx.qs.qssadj2, ctx.qs.qssadjs2);
        addQs(h, "irr", ctx.qs.qsirr, ctx.qs.qsirrs);
        addQs(h, "irrevadj", ctx.qs.qsirr2, ctx.qs.qsirrs2);
    }

    // --- forecasts --------------------------------------------------------
    // Dated from one period past the end of the span; the transformed-scale
    // pair has no calendar of its own and rides along as vectors.
    const auto& f = ctx.forecasts;
    if (f.nfcst > 0) {
        addTableAt(h, "fct", begspn, sp, h.nobs, f.fcst);
        addTableAt(h, "fctlo", begspn, sp, h.nobs, f.lwrci);
        addTableAt(h, "fcthi", begspn, sp, h.nobs, f.uprci);
        addVector(h, "forecast.trn", f.trnfct);
        addVector(h, "forecast.trnse", f.trnse);
    }
    if (f.nbcst > 0) {
        addVector(h, "backcast.trn", f.trnbct);
        addVector(h, "backcast.trnse", f.trnbse);
    }
}

// The filter lengths X-11 actually used. Only meaningful on the X-11 path --
// SEATS has no moving averages to report.
void harvestX11Filters(x13_run& h, const x13::X13Context& ctx) {
    const x13::x11opt_cmn& o = ctx.x11opt;
    const int ny = (o.ny > 0 && o.ny <= 12) ? o.ny : 0;

    if (o.nterm > 0) {
        addDiag(h, "x11.trendma.nterm", static_cast<double>(o.nterm));
        addText(h, "x11.trendma", std::to_string(o.nterm) + "-term Henderson");
    }
    if (o.ratic != x13::prm::DNOTST) addDiag(h, "x11.ic", o.ratic);
    if (o.ratis != x13::prm::DNOTST) addDiag(h, "x11.msr", o.ratis);
    if (o.mcd > 0) addDiag(h, "x11.mcd", static_cast<double>(o.mcd));

    const char* global = seasonalMaLabel(o.lterm);
    if (*global) addDiag(h, "x11.seasonalma.lterm", static_cast<double>(o.lterm));
    addText(h, "x11.seasonalma.selected",
            ctx.x11_sfmsr_filter != 0 ? "msr" : "spec");

    if (ny == 0) {
        if (*global) addText(h, "x11.seasonalma", global);
        return;
    }
    // One code per period. They agree with each other on almost every run, so
    // the label collapses to a single filter name when it can.
    std::vector<double> codes;
    std::string label;
    bool uniform = true;
    for (int i = 1; i <= ny; ++i) {
        const int c = o.lter(i);
        codes.push_back(static_cast<double>(c));
        if (i > 1 && c != o.lter(1)) uniform = false;
        if (i > 1) label += ",";
        const char* nm = seasonalMaLabel(c);
        label += *nm ? nm : "?";
    }
    addVector(h, "x11.seasonalma.code", codes);
    const char* first = seasonalMaLabel(o.lter(1));
    if (uniform && *first) addText(h, "x11.seasonalma", first);
    else if (!uniform)     addText(h, "x11.seasonalma", label);
    else if (*global)      addText(h, "x11.seasonalma", global);
}

void harvestCommon(x13_run& h, const x13::X13Context& ctx);

// SEATS leaves its tables 0-indexed from Begspn, all the same length, so they
// need none of the X-11 punch-range machinery above.
void harvestSeats(x13_run& h, const x13::X13Context& ctx) {
    const int sp = ctx.model.sp;
    const int* begspn = ctx.mdldat.begspn.data();

    h.period = sp;
    h.nobs = static_cast<int>(ctx.seats_sa.size());
    h.modelBased = true;              // SEATS has no no-model path in the oracle
    h.mode = ctx.x11msc.psuadd ? 3 : ctx.x11opt.muladd;
    h.arimaModel = ctx.arima.bstdsn.str();
    while (!h.arimaModel.empty() && h.arimaModel.back() == ' ')
        h.arimaModel.pop_back();
    if (h.arimaModel == "?") h.arimaModel.clear();

    // Tag names follow the oracle's save tokens, as x13run_seats.cpp dumps them.
    addTableVec(h, "s10", begspn, sp, ctx.seats_seasonal_add);
    addTableVec(h, "s11", begspn, sp, ctx.seats_sa);
    addTableVec(h, "s12", begspn, sp, ctx.seats_trend);
    addTableVec(h, "s13", begspn, sp, ctx.seats_ir);
    addTableVec(h, "s14", begspn, sp, ctx.seats_cycle);
    addTableVec(h, "s16", begspn, sp, ctx.seats_combined_add);
    addTableVec(h, "s18", begspn, sp, ctx.seats_combined_factor);

    harvestCommon(h, ctx);
}

// Harvest everything the X-11 driver leaves on the context.
void harvest(x13_run& h, const x13::X13Context& ctx) {
    const int sp = ctx.model.sp;
    const int* begspn = ctx.mdldat.begspn.data();
    const int pos1ob = ctx.x11ptr.pos1ob;
    const int posfob = ctx.x11ptr.posfob;

    h.period = sp;
    h.nobs = (posfob >= pos1ob) ? (posfob - pos1ob + 1) : 0;
    h.modelBased = ctx.captured.has_model;
    h.mode = ctx.x11msc.psuadd ? 3 : ctx.x11opt.muladd;
    // bstdsn is a fixed-width Fortran field: blank-padded, and literally "?"
    // when no model was identified. Both mean "none" to a caller, so normalize
    // to the empty string the header documents.
    h.arimaModel = ctx.arima.bstdsn.str();
    while (!h.arimaModel.empty() && h.arimaModel.back() == ' ')
        h.arimaModel.pop_back();
    if (h.arimaModel == "?") h.arimaModel.clear();

    if (posfob < pos1ob) return;

    // b1 / d10 / d16 ranges: see the long comments at x13run_x11.cpp:140-178.
    const int b1_frst = (ctx.captured.has_model && ctx.tbllog.savbct)
                            ? ctx.x11ptr.pos1bk : pos1ob;
    const int b1_last = (ctx.captured.has_model && ctx.tbllog.savfct)
                            ? ctx.x11ptr.posffc : posfob;
    const double* b1src = (ctx.captured.has_model || ctx.x11opt.khol > 1)
                              ? ctx.orisrs.stoap.data()
                              : ctx.inpt.series.data();
    addTable(h, "b1", begspn, sp, b1_frst, b1_last, b1src, pos1ob);

    const int sf_frst = ctx.tbllog.savbct ? ctx.x11ptr.pos1bk : pos1ob;
    const int sf_last = !ctx.tbllog.savfct
                            ? posfob
                            : (ctx.extend.nfcst > 0 ? ctx.x11ptr.posffc
                                                    : posfob + sp);
    addTable(h, "d10", begspn, sp, sf_frst, sf_last, ctx.x11srs.sts.data(), pos1ob);
    addTable(h, "d11", begspn, sp, pos1ob, posfob, ctx.x11srs.stci.data(), pos1ob);
    addTable(h, "d12", begspn, sp, pos1ob, posfob, ctx.x11srs.stc.data(), pos1ob);
    addTable(h, "d13", begspn, sp, pos1ob, posfob, ctx.x11srs.sti.data(), pos1ob);
    if (!ctx.x11_ststd.empty())
        addTable(h, "d16", begspn, sp, sf_frst, sf_last, ctx.x11_ststd.data(),
                 pos1ob);

    // Part-E tables (only when x11pt4's Part-E ran).
    if (ctx.x11_etables_set) {
        addTable(h, "e1", begspn, sp, pos1ob, posfob, ctx.adxser.stome.data(),
                 pos1ob);
        addTable(h, "e2", begspn, sp, pos1ob, posfob, ctx.adxser.stcime.data(),
                 pos1ob);
        addTable(h, "e3", begspn, sp, pos1ob, posfob, ctx.mq5a_stime.data(),
                 pos1ob);
        const int chg1 = pos1ob + 1;
        const struct { const char* tag; const std::vector<double>* v; } chg[] = {
            {"e5", &ctx.x11_e5},   {"e6", &ctx.x11_e6},
            {"e6a", &ctx.x11_e6a}, {"e6r", &ctx.x11_e6r},
            {"e7", &ctx.x11_e7},   {"e8", &ctx.x11_e8},
        };
        for (const auto& c : chg)
            if (!c.v->empty())
                addTable(h, c.tag, begspn, sp, chg1, posfob, c.v->data(), pos1ob);
        addTable(h, "e11", begspn, sp, pos1ob, posfob, ctx.x11_e11.data(), pos1ob);
        const int e18_frst = ctx.tbllog.savbct ? ctx.x11ptr.pos1bk : pos1ob;
        const int e18_last = ctx.tbllog.savfct ? ctx.x11ptr.posffc : posfob;
        addTable(h, "e18", begspn, sp, e18_frst, e18_last, ctx.x11_e18.data(),
                 pos1ob);
        addTable(h, "eb", begspn, sp, e18_frst, e18_last, ctx.x11_eb.data(),
                 pos1ob);
    }

    // transform{constant=}: D11 / published D12 with the constant still in.
    if (!ctx.x11_stcipc.empty())
        addTable(h, "sac", begspn, sp, pos1ob, posfob, ctx.x11_stcipc.data(),
                 pos1ob);
    if (!ctx.x11_stc2pc.empty())
        addTable(h, "tac", begspn, sp, pos1ob, posfob, ctx.x11_stc2pc.data(),
                 pos1ob);

    // x11regression{tdprior=}: the user prior trading-day factor, already
    // 0-based from pos1ob.
    addTableVec(h, "a4", begspn, sp, ctx.x11_a4_prior);

    // force{}: the forced SA series and the per-observation forcing factor.
    if (ctx.force.iyrt > 0) {
        addTable(h, "saa", begspn, sp, pos1ob, posfob, ctx.adxser.stci2.data(),
                 pos1ob);
        if (!ctx.x11_frcfac.empty()) {
            const int lstfrc = ctx.force.lfctfr ? ctx.x11ptr.posffc : posfob;
            addTable(h, "ffc", begspn, sp, pos1ob, lstfrc, ctx.x11_frcfac.data(),
                     pos1ob);
        }
    }
    if (ctx.force.lrndsa)
        addTable(h, "rnd", begspn, sp, pos1ob, posfob, ctx.adxser.stcirn.data(),
                 pos1ob);

    // The F2/F3 quality statistics -- the .udg savelog block, as scalars.
    if (ctx.x11_f3_set) {
        const x13::work2_cmn& w = ctx.x11_f2work2;
        for (int i = 1; i <= w.nn; ++i) {
            // svf2f3.f:97 -- M6 is suppressed when Kfulsm==2.
            if (i == 6 && ctx.x11opt.kfulsm >= 2) continue;
            char buf[16];
            std::snprintf(buf, sizeof(buf), "f3.m%02d", i);
            addDiag(h, buf, w.qu(i));
        }
        addDiag(h, "f3.q", w.qual);
        addDiag(h, "f3.qm2", w.q2m2);
        addDiag(h, "f3.fail", static_cast<double>(w.kfail));
        addDiag(h, "f2.ic", ctx.x11_f2ratic);
        addDiag(h, "f2.is", ctx.x11_f2ratis);
        addDiag(h, "f2.mcd", static_cast<double>(ctx.x11_f2mcd));
    }

    harvestX11Filters(h, ctx);
    harvestCommon(h, ctx);
}

// Pin the x87 floating-point precision-control word for the duration of a run,
// and put it back afterwards.
//
// THIS IS A PARITY REQUIREMENT, not hygiene. The engine is validated bit-exact
// against the Fortran oracle inside a MinGW-built executable, where the C
// runtime starts the x87 unit in EXTENDED (64-bit) precision. A shared library
// inherits whatever precision its HOST process set, and hosts disagree: R on
// Windows is itself MinGW-built and leaves it extended, while CPython is MSVC-
// built and runs at 53-bit. The same DLL therefore produced two different
// answers from R and from Python -- R matching the oracle exactly, Python
// drifting in the last ULP of every long-double intermediate.
//
// On well-conditioned specs that drift is ~1e-16 and invisible. On the
// near-non-invertible ones (the fixed-airline variants, the short-span sfshort
// filters) the optimizer amplifies it: measured 8.2e-6 on unrate_sfshort's d10,
// which is six orders worse than the 1e-6 estimation floor those specs
// otherwise hold. So the library sets the mode it was validated under rather
// than trusting the host, and restores the host's on the way out so we do not
// perturb the caller's own arithmetic.
// Measured x87 control words, same DLL, same spec (unrate_sfshort-x11):
//     MinGW .exe   0x037f   PC=3, 64-bit extended   -> matches the oracle
//     R (Rscript)  0x037f   PC=3, 64-bit extended   -> matches the oracle
//     CPython      0x027f   PC=2, 53-bit double     -> d10 off by 8.2e-6
// MXCSR was identical (0x1fa0) in all three, so this is precision control
// alone.
//
// NOTE the trap: `_control87(_PC_64, _MCW_PC)` does NOT do this. On x86-64 the
// CRT ignores the precision-control mask entirely (it is a no-op kept for
// source compatibility), because SSE has no equivalent field. The x87 register
// still exists and still governs long-double arithmetic, so it has to be
// written directly with fnstcw/fldcw.
class FpPrecisionGuard {
public:
    FpPrecisionGuard() {
#if defined(__x86_64__) || defined(__i386__)
        __asm__ __volatile__("fnstcw %0" : "=m"(saved_));
        const unsigned short want =
            static_cast<unsigned short>((saved_ & ~0x0300u) | 0x0300u);  // PC=3
        if (want != saved_) {
            __asm__ __volatile__("fldcw %0" : : "m"(want));
            active_ = true;
        }
#endif
    }
    ~FpPrecisionGuard() {
#if defined(__x86_64__) || defined(__i386__)
        // Restore the host's word: a library that silently changed the
        // interpreter's FP mode would perturb the caller's own arithmetic.
        if (active_) __asm__ __volatile__("fldcw %0" : : "m"(saved_));
#endif
    }
    FpPrecisionGuard(const FpPrecisionGuard&) = delete;
    FpPrecisionGuard& operator=(const FpPrecisionGuard&) = delete;

private:
    unsigned short saved_ = 0;
    bool active_ = false;
};

// Shared body of both run entry points. Never throws: the whole point of this
// layer is that no exception reaches a ctypes/.C caller, where it would be
// undefined behaviour rather than an error.
x13_run* runSpec(const std::string& text, const std::string& base) {
    FpPrecisionGuard fpGuard;
    x13_run* h = nullptr;
    try {
        h = new x13_run();
    } catch (...) {
        return nullptr;    // allocation failure is the only NULL return
    }
    try {
        // Which decomposition the spec asks for is only knowable after parsing,
        // and both drivers parse for themselves -- so parse once into a scratch
        // context purely to read `has_seats`, then hand a FRESH context to the
        // driver. Parsing twice is wasteful but safe; re-running a driver over
        // an already-parsed context is neither.
        bool wantSeats = false;
        {
            auto probe = std::make_unique<x13::X13Context>();
            if (x13::parse_spec(*probe, text, base))
                wantSeats = probe->captured.has_seats;
        }

        auto ctxp = std::make_unique<x13::X13Context>();
        const bool ok = wantSeats ? x13::run_seats(*ctxp, text, base)
                                  : x13::run_x11(*ctxp, text, base);
        if (!ok || ctxp->error.lfatal) {
            h->ok = false;
            const std::string why = errorText(*ctxp);
            h->error = "engine reported a fatal condition for '" + base + "'";
            if (!why.empty()) h->error += ": " + why;
            // Harvest anyway where the span is sane: metadata on a failed run is
            // more useful than nothing, and callers gate on x13_ok().
            return h;
        }
        if (wantSeats && ctxp->seats_ran) harvestSeats(*h, *ctxp);
        else                              harvest(*h, *ctxp);
        addText(*h, "log.messages", errorText(*ctxp));
        h->ok = true;
    } catch (const std::exception& e) {
        h->ok = false;
        h->error = std::string("engine error: ") + e.what();
    } catch (...) {
        h->ok = false;
        h->error = "engine error: unknown exception";
    }
    return h;
}

const char* kEmpty = "";

}  // namespace

extern "C" {

int x13_abi_version(void) { return 2; }

unsigned x13_host_fp_control(void) {
    unsigned short cw = 0;
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("fnstcw %0" : "=m"(cw));
#endif
    return cw;
}

const char* x13_engine_version(void) { return x13::upstream_version(); }

x13_run* x13_run_spec_text(const char* spec_text, const char* series_name) {
    if (!spec_text) return nullptr;
    // runSpec is itself no-throw, but its two std::string PARAMETERS are
    // constructed here, at the call site, outside that guarantee -- a
    // std::bad_alloc from either would cross the ABI. Same reason the file
    // entry point below is wrapped.
    try {
        return runSpec(spec_text, series_name ? series_name : "series");
    } catch (...) {
        return nullptr;
    }
}

// Body of x13_run_spec_file. Split out so the whole thing -- including the
// std::string construction, the substr-based dir/base split and the
// error-message concatenations, all of which can throw -- sits inside one
// catch. Only the file-read block used to be guarded.
static x13_run* runSpecFileImpl(const char* spec_path) {
    std::string path(spec_path);

    std::string text;
    try {
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) {
            x13_run* h = new (std::nothrow) x13_run();
            if (h) {
                h->ok = false;
                h->error = "cannot open spec file: " + path;
            }
            return h;
        }
        char buf[8192];
        std::size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) text.append(buf, n);
        std::fclose(f);
    } catch (...) {
        return nullptr;
    }

    // Split dir / base so relative data paths inside the spec resolve against
    // the spec's own directory, as the reference driver does.
    const std::size_t slash = path.find_last_of("/\\");
    const std::string dir =
        (slash == std::string::npos) ? std::string() : path.substr(0, slash);
    std::string base =
        (slash == std::string::npos) ? path : path.substr(slash + 1);
    if (base.size() >= 4 && base.substr(base.size() - 4) == ".spc")
        base = base.substr(0, base.size() - 4);

    if (dir.empty()) return runSpec(text, base);

    // Run with the spec's directory as CWD, then restore. NOT thread-safe --
    // documented in the loaders; the engine's COMMON-derived context is
    // per-handle but the process CWD is not.
    char saved[4096];
    const bool haveCwd = x13_capi_getcwd(saved, sizeof(saved)) != nullptr;
    if (x13_capi_chdir(dir.c_str()) != 0) {
        x13_run* h = new (std::nothrow) x13_run();
        if (h) {
            h->ok = false;
            h->error = "cannot enter spec directory: " + dir;
        }
        return h;
    }
    x13_run* h = runSpec(text, base);
    if (haveCwd) x13_capi_chdir(saved);
    return h;
}

x13_run* x13_run_spec_file(const char* spec_path) {
    if (!spec_path) return nullptr;
    try {
        return runSpecFileImpl(spec_path);
    } catch (...) {
        return nullptr;
    }
}

void x13_run_free(x13_run* run) { delete run; }

int x13_ok(const x13_run* run) { return (run && run->ok) ? 1 : 0; }

const char* x13_error(const x13_run* run) {
    return run ? run->error.c_str() : kEmpty;
}

int x13_period(const x13_run* run) { return run ? run->period : 0; }
int x13_nobs(const x13_run* run) { return run ? run->nobs : 0; }
int x13_model_based(const x13_run* run) {
    return (run && run->modelBased) ? 1 : 0;
}
int x13_mode(const x13_run* run) { return run ? run->mode : 0; }

const char* x13_arima_model(const x13_run* run) {
    return run ? run->arimaModel.c_str() : kEmpty;
}

int x13_table_count(const x13_run* run) {
    return run ? static_cast<int>(run->tables.size()) : 0;
}

const char* x13_table_name(const x13_run* run, int index) {
    if (!run || index < 0 || index >= static_cast<int>(run->tables.size()))
        return kEmpty;
    return run->tables[static_cast<std::size_t>(index)].name.c_str();
}

int x13_table_length(const x13_run* run, const char* name) {
    if (!run) return 0;
    const Table* t = run->find(name);
    return t ? static_cast<int>(t->values.size()) : 0;
}

int x13_table_start_year(const x13_run* run, const char* name) {
    if (!run) return 0;
    const Table* t = run->find(name);
    return t ? t->startYear : 0;
}

int x13_table_start_period(const x13_run* run, const char* name) {
    if (!run) return 0;
    const Table* t = run->find(name);
    return t ? t->startPeriod : 0;
}

int x13_table_values(const x13_run* run, const char* name, double* out,
                     int capacity) {
    if (!run) return 0;
    const Table* t = run->find(name);
    if (!t) return 0;
    const int n = static_cast<int>(t->values.size());
    if (!out || capacity < n) return -n;    // report the need, write nothing
    for (int i = 0; i < n; ++i) out[i] = t->values[static_cast<std::size_t>(i)];
    return n;
}

int x13_table_dates(const x13_run* run, const char* name, int* years,
                    int* periods, int capacity) {
    if (!run) return 0;
    const Table* t = run->find(name);
    if (!t) return 0;
    const int n = static_cast<int>(t->values.size());
    if (capacity < n) return -n;
    if (!years && !periods) return -n;
    for (int i = 0; i < n; ++i) {
        int y = t->startYear;
        int p = t->startPeriod + i;
        if (run->period > 0) {
            y += (p - 1) / run->period;
            p = (p - 1) % run->period + 1;
        }
        if (years) years[i] = y;
        if (periods) periods[i] = p;
    }
    return n;
}

int x13_vector_count(const x13_run* run) {
    return run ? static_cast<int>(run->vectors.size()) : 0;
}

const char* x13_vector_name(const x13_run* run, int index) {
    if (!run || index < 0 || index >= static_cast<int>(run->vectors.size()))
        return kEmpty;
    return run->vectors[static_cast<std::size_t>(index)].name.c_str();
}

int x13_vector_length(const x13_run* run, const char* name) {
    if (!run) return 0;
    const Vector* v = run->findVector(name);
    return v ? static_cast<int>(v->values.size()) : 0;
}

int x13_vector_values(const x13_run* run, const char* name, double* out,
                      int capacity) {
    if (!run) return 0;
    const Vector* v = run->findVector(name);
    if (!v) return 0;
    const int n = static_cast<int>(v->values.size());
    if (!out || capacity < n) return -n;    // report the need, write nothing
    for (int i = 0; i < n; ++i) out[i] = v->values[static_cast<std::size_t>(i)];
    return n;
}

int x13_text_count(const x13_run* run) {
    return run ? static_cast<int>(run->texts.size()) : 0;
}

const char* x13_text_name(const x13_run* run, int index) {
    if (!run || index < 0 || index >= static_cast<int>(run->texts.size()))
        return kEmpty;
    return run->texts[static_cast<std::size_t>(index)].name.c_str();
}

const char* x13_text_value(const x13_run* run, const char* name) {
    if (!run) return kEmpty;
    const Text* t = run->findText(name);
    return t ? t->value.c_str() : kEmpty;
}

int x13_diag_count(const x13_run* run) {
    return run ? static_cast<int>(run->diags.size()) : 0;
}

const char* x13_diag_name(const x13_run* run, int index) {
    if (!run || index < 0 || index >= static_cast<int>(run->diags.size()))
        return kEmpty;
    return run->diags[static_cast<std::size_t>(index)].name.c_str();
}

int x13_diag_value(const x13_run* run, const char* name, double* out) {
    if (!run || !name || !out) return 0;
    for (const auto& d : run->diags)
        if (d.name == name) {
            *out = d.value;
            return 1;
        }
    return 0;
}

}  // extern "C"
