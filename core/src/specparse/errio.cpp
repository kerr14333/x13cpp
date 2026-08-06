// errio.cpp -- error/diagnostic output: abend.f, errhdr.f, writln.f, inpter.f.
// inpter's Ptr argument is a 2-int C array {line, column}.
#include "specparse/specparse.hpp"
#include "common/x13context.hpp"
#include "gen/notset.hpp"
#include "x13/fformat.hpp"

#include <algorithm>
#include <string>

namespace x13 {

using namespace lexprm;

// abend.f -- abnormal termination. For M1 (no sliding-spans / history runs) the
// diagnostic-file bookkeeping is inert; the essential effect is Lfatal = .true.
void abend(X13Context& ctx) {
    ctx.error.lfatal = true;
}

// errhdr.f -- prints a banner into the error file to say which HIDDEN RUN
// produced the messages that follow, once per span. Every writln to Mt2 goes
// through it, so it is the framing around any warning a sliding-spans or
// history span emits. `Ierhdr` is the span already announced: the second
// message from the same span adds no second banner, and the Issap==3 / Irev==5
// arms close the section off once the loop is over.
void errhdr(X13Context& ctx) {
    hiddn_cmn& hid = ctx.hiddn;
    if (hid.issap < 2 && hid.irev < 4) return;
    if ((hid.issap == 2 && hid.ierhdr == ctx.ssft.icol) ||
        (hid.irev == 4 && hid.ierhdr == ctx.rev.revptr)) return;
    auto& mt2 = ctx.channels_.unit(ctx.units.mt2);
    // errhdr.f:1010 -- 40 (star,dash) pairs under FORMAT(80A1).
    static const std::string rule = [] {
        std::string s;
        for (int i = 0; i < 40; ++i) s += "*-";
        return s;
    }();
    if (hid.issap == 2) {
        mt2.put(rule + "\n");
        mt2.put(fwrite_fmt("('  Error/Warning Messages for sliding span # ',"
                           "i1,':')",
                           ctx.ssft.icol) +
                "\n");
        hid.ierhdr = ctx.ssft.icol;
    } else if (hid.issap == 3 && hid.ierhdr != prm::NOTSET) {
        mt2.put(rule + "\n");
        mt2.put(fwrite_fmt("(a)", " ") + "\n");
        hid.ierhdr = prm::NOTSET;
    } else if (hid.irev == 4) {
        mt2.put(rule + "\n");
        mt2.put(fwrite_fmt("('  Error/Warning Messages for history run ending ',"
                           "a,':')",
                           std::string(hid.crvend.raw().substr(
                               0, static_cast<std::size_t>(hid.nrvend)))) +
                "\n");
        hid.ierhdr = ctx.rev.revptr;
    } else if (hid.irev == 5 && hid.ierhdr != prm::NOTSET) {
        mt2.put(rule + "\n");
        mt2.put(fwrite_fmt("(a)", " ") + "\n");
        hid.ierhdr = prm::NOTSET;
    }
}

// writln.f
void writln(X13Context& ctx, std::string_view oline, int flhdnl, int flhdn2, bool lblnk) {
    int mt2 = ctx.units.mt2;
    if (flhdnl == mt2 || flhdn2 == mt2) errhdr(ctx);
    std::size_t n = std::min<std::size_t>(131, oline.size());
    std::string body(oline.substr(0, n));
    if (flhdnl > 0) {
        if (lblnk) ctx.channels_.unit(flhdnl).put(fwrite_fmt("(' ',a)", " ") + "\n");
        ctx.channels_.unit(flhdnl).put(fwrite_fmt("(' ',a)", body) + "\n");
    }
    if (flhdn2 > 0) {
        if (lblnk) ctx.channels_.unit(flhdn2).put(fwrite_fmt("(' ',a)", " ") + "\n");
        ctx.channels_.unit(flhdn2).put(fwrite_fmt("(' ',a)", body) + "\n");
    }
}

// inpter.f
void inpter(X13Context& ctx, int errtyp, const int* ptr, std::string_view errmsg) {
    const int STDERR = stdio::STDERR;
    const int Mt2 = ctx.units.mt2;
    auto& err = ctx.channels_.unit(STDERR);
    auto& mt2 = ctx.channels_.unit(Mt2);

    const int ptr_line = ptr[0];
    const int ptr_char = ptr[1];

    bool lprtln = false;
    int llim = 70;
    if (ctx.title.lwdprt) llim = 121;

    int displc = 0;
    if (errtyp == PERROR || errtyp == PWARN) {
        std::string prglin(LINLEN + 1, ' ');
        int prgln = 0;
        int linno = ptr_line;
        lprtln = rngbuf(ctx, 4, linno, prglin, prgln);
        if (lprtln) {
            if (prgln - 1 > llim) {
                int w = std::min(llim + 10, prgln - 1);
                std::string body = prglin.substr(0, static_cast<std::size_t>(w));
                std::string rec = fwrite_fmt("(/,' Line',i5,':  ',/,' ',a)", ptr_line, body) + "\n";
                err.put(rec); mt2.put(rec);
                displc = 0;
            } else {
                std::string body = prglin.substr(0, static_cast<std::size_t>(prgln - 1));
                std::string rec = fwrite_fmt("(/,' Line',i5,':  ',a)", ptr_line, body) + "\n";
                err.put(rec); mt2.put(rec);
                displc = 12;
            }
            // Carrot under the offending column.
            std::string carrot(static_cast<std::size_t>(ptr_char + displc), ' ');
            carrot += '^';
            std::string rec = fwrite_fmt("(a)", carrot) + "\n";
            err.put(rec); mt2.put(rec);
        }
    }

    std::string errstr;
    int nerrcr;
    if (errtyp % 2 == 0) { errstr = "WARNING"; nerrcr = 7; }
    else { errstr = "ERROR"; nerrcr = 5; }

    if (!lprtln || (errtyp != PERROR && errtyp != PWARN)) {
        std::string rec = fwrite_fmt("()") + "\n";
        err.put(rec); mt2.put(rec);
    }

    if (static_cast<int>(errmsg.size()) <= llim) {
        std::string rec = fwrite_fmt("(' ',a,':  ',a)", errstr.substr(0, static_cast<std::size_t>(nerrcr)),
                                     std::string(errmsg)) + "\n";
        err.put(rec); mt2.put(rec);
    } else {
        int llim2 = llim;
        while (errmsg[static_cast<std::size_t>(llim2 - 1)] != ' ') --llim2;
        std::string blnk(static_cast<std::size_t>(nerrcr), ' ');
        std::string first(errmsg.substr(0, static_cast<std::size_t>(llim2)));
        std::string rest(errmsg.substr(static_cast<std::size_t>(llim2)));
        std::string rec = fwrite_fmt("(' ',a,':  ',a,/,' ',a,'   ',a)",
                                     errstr.substr(0, static_cast<std::size_t>(nerrcr)),
                                     first, blnk, rest) + "\n";
        err.put(rec); mt2.put(rec);
    }

    if (!lprtln && (errtyp == PERROR || errtyp == PWARN)) {
        std::string blnk(static_cast<std::size_t>(nerrcr + 3), ' ');
        std::string rec = fwrite_fmt("(a,' Problem was discovered on line',i5,', column ',i4,'.')",
                                     blnk, ptr_line, ptr_char) + "\n";
        err.put(rec); mt2.put(rec);
    }
}

} // namespace x13
