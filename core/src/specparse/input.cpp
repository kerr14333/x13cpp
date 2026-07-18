// input.cpp -- low-level input buffer + character stream:
// rngbuf.f, intinp.f, getchr.f, putbak.f, whitsp.f.
#include "specparse/specparse.hpp"

namespace x13 {

using namespace lexprm;

namespace {
// Ensure a string is at least LINLEN+1 chars, space padded (Linex-like).
void pad_line(std::string& s) {
    if (s.size() < static_cast<std::size_t>(LINLEN + 1))
        s.resize(static_cast<std::size_t>(LINLEN + 1), ' ');
}
inline char& at1(std::string& s, int i) { return s[static_cast<std::size_t>(i - 1)]; }
} // namespace

// rngbuf.f  (Cmd: 1=init, 2=next line, 3=push back, 4=fetch line Linno)
bool rngbuf(X13Context& ctx, int cmd, int& linno, std::string& lin, int& linln) {
    LexState& L = ctx.lex;
    constexpr bool PFAIL = false, PSCCD = true;
    bool result = PSCCD;
    pad_line(lin);

    switch (cmd) {
    case 1: {  // Initialize the buffer
        L.crntln = 0;
        L.begbuf = PBUFSZ - 1;
        L.endbuf = PBUFSZ - 1;
        L.crntbf = L.endbuf;
        L.bufln[0] = 0;
        L.psteof = false;
        L.lexok = true;
        return result;
    }
    case 2: {  // Read the next line, fail at EOF
        if (L.psteof) goto eof70;
        if (L.crntbf == L.endbuf) {
            if (L.read_idx >= L.input_lines.size()) goto eof70;
            // READ(Inputx,'(A)') Lin  -- blank-pad into the fixed record.
            std::string raw = L.input_lines[L.read_idx++];
            for (auto& c : lin) c = ' ';
            pad_line(lin);
            for (std::size_t i = 0; i < raw.size() && i < static_cast<std::size_t>(LINLEN); ++i)
                lin[i] = raw[i];
            L.endbuf = (L.endbuf + 1) % PBUFSZ;
            if (L.begbuf == L.endbuf) L.begbuf = (L.begbuf + 1) % PBUFSZ;
            L.crntbf = (L.crntbf + 1) % PBUFSZ;
            linln = nblank(std::string_view(lin.data(), static_cast<std::size_t>(LINLEN)));
            linln = linln + 1;
            if (linln > LINLEN) { abend(ctx); return result; }
            at1(lin, linln) = L.newlin;
            // Filter out unprintable characters (tab allowed, BCM May 2005).
            int i = 1;
            while (true) {
                if (i < linln) {
                    char nxtchr = at1(lin, i);
                    if ((nxtchr < ' ' || nxtchr > '~') && nxtchr != L.tabchr) {
                        for (int k = i; k <= linln - 1; ++k) at1(lin, k) = at1(lin, k + 1);
                        linln = linln - 1;
                    } else {
                        i = i + 1;
                    }
                    continue;
                }
                break;
            }
            L.buf[static_cast<std::size_t>(L.endbuf)] =
                lin.substr(0, static_cast<std::size_t>(LINLEN));
            L.bufln[static_cast<std::size_t>(L.endbuf)] = linln;
        } else {
            L.crntbf = (L.crntbf + 1) % PBUFSZ;
            lin = L.buf[static_cast<std::size_t>(L.crntbf)];
            pad_line(lin);
            linln = L.bufln[static_cast<std::size_t>(L.crntbf)];
        }
        L.crntln = L.crntln + 1;
        linno = L.crntln;
        return result;
    }
    case 3: {  // Push back the last line
        if (L.crntbf == L.begbuf) {
            result = PFAIL;
            linln = 0;
        } else {
            if (!L.psteof) {
                L.crntbf = (L.crntbf + PBUFSZ - 1) % PBUFSZ;
                L.crntln = L.crntln - 1;
            }
            lin = L.buf[static_cast<std::size_t>(L.crntbf)];
            pad_line(lin);
            linln = L.bufln[static_cast<std::size_t>(L.crntbf)];
            linno = L.crntln;
            L.psteof = false;
        }
        return result;
    }
    case 4: {  // Fetch line number Linno if still buffered
        if (linno < L.crntln - ((L.crntbf + PBUFSZ - L.begbuf) % PBUFSZ) ||
            linno > L.crntln) {
            result = PFAIL;
            linln = 0;
        } else {
            int i = (L.crntbf + linno - L.crntln + PBUFSZ) % PBUFSZ;
            lin = L.buf[static_cast<std::size_t>(i)];
            pad_line(lin);
            linln = L.bufln[static_cast<std::size_t>(i)];
        }
        return result;
    }
    default:
        abend(ctx);
        return result;
    }

eof70:
    at1(lin, 1) = L.chreof;
    linln = 1;
    L.psteof = true;
    return PFAIL;
}

// intinp.f
void intinp(X13Context& ctx, int instr) {
    LexState& L = ctx.lex;
    L.inputx = instr;
    L.lineno = 0;
    L.lineln = 0;
    bool ldmy = rngbuf(ctx, 1, L.lineno, L.linex, L.lineln);
    if (!ldmy || ctx.error.lfatal) return;
    L.pos[PLINE] = 0;
    L.pos[PCHAR] = 1;
    L.lstpos[PLINE] = 0;
    L.lstpos[PCHAR] = 1;
    L.errpos[PLINE] = 0;
    L.errpos[PCHAR] = 1;
    lex(ctx);
    if (L.nxtktp == EOFTOK) {
        inpter(ctx, PERROR, L.pos.data() + 1,
               "Cannot process empty input specifications file.");
        abend(ctx);
    }
}

// getchr.f
char getchr(X13Context& ctx, char& nxtchr) {
    LexState& L = ctx.lex;
    if (L.pos[PCHAR] > L.lineln) {
        if (rngbuf(ctx, 2, L.lineno, L.linex, L.lineln)) {
            L.pos[PLINE] = L.lineno;
            L.pos[PCHAR] = 1;
        } else {
            L.pos[PCHAR] = 1;
        }
    }
    char c = L.linchr(L.pos[PCHAR]);
    L.pos[PCHAR] = L.pos[PCHAR] + 1;
    nxtchr = c;
    return c;
}

// putbak.f
void putbak(X13Context& ctx, char lstchr) {
    LexState& L = ctx.lex;
    if (L.pos[PCHAR] <= 1) {
        if (rngbuf(ctx, 3, L.pos[PLINE], L.linex, L.lineln)) {
            L.pos[PCHAR] = L.lineln;
        } else {
            int ptr[2] = {L.pos[PLINE], L.pos[PCHAR]};
            inpter(ctx, PERROR, ptr, "Can't push input buffer back anymore");
        }
    }
    if (lstchr != L.linchr(L.pos[PCHAR] - 1)) {
        L.pos[PCHAR] = L.pos[PCHAR] - 1;
        std::string msg = std::string("\"") + lstchr + "\" is not the last character ";
        int ptr[2] = {L.pos[PLINE], L.pos[PCHAR]};
        inpter(ctx, PERROR, ptr, msg);
        abend(ctx);
        return;
    } else {
        L.pos[PCHAR] = L.pos[PCHAR] - 1;
    }
}

// whitsp.f
int whitsp(X13Context& ctx) {
    LexState& L = ctx.lex;
    char dmychr;
    while (true) {
        char chr = getchr(ctx, dmychr);
        if (chr != ' ' && chr != L.tabchr && chr != L.newlin) {
            if (chr == L.chreof) {
                return EOFTOK;
            } else {
                putbak(ctx, chr);
                return NULLTOK;
            }
        }
    }
}

} // namespace x13
