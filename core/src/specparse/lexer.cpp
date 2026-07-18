// lexer.cpp -- tokenizer: lex.f, qtoken.f, qname.f, qquote.f, qcmmnt.f,
// qintgr.f, qdoble.f. Tokens land in ctx.lex.nxttok / nxtkln / nxtktp.
#include "specparse/specparse.hpp"

namespace x13 {

using namespace lexprm;

namespace {
inline char& tok1(LexState& L, int i) { return L.nxttok[static_cast<std::size_t>(i - 1)]; }
} // namespace

// qname.f -- Astrng is Nxttok.
bool qname(X13Context& ctx, std::string& astrng, int& nchr) {
    LexState& L = ctx.lex;
    char dmychr;
    bool isname = false;
    int pchr = static_cast<int>(astrng.size());
    nchr = 0;
    char chr = getchr(ctx, dmychr);
    if ((chr >= BIGA && chr <= BIGZ) || (chr >= LITTLA && chr <= LITTLZ)) {
        isname = true;
        nchr = 1;
        astrng[static_cast<std::size_t>(nchr - 1)] = chr;
        while (true) {
            chr = getchr(ctx, dmychr);
            if ((chr >= BIGA && chr <= BIGZ) || (chr >= LITTLA && chr <= LITTLZ) ||
                (chr >= CZERO && chr <= CNINE) || chr == '-' || chr == '_' ||
                chr == '@' || chr == '$' || chr == '%' || chr == '.') {
                nchr = nchr + 1;
                if (nchr <= pchr) {
                    astrng[static_cast<std::size_t>(nchr - 1)] = chr;
                    continue;
                } else {
                    L.pos[PCHAR] = L.pos[PCHAR] - nchr;
                    std::string s(5, ' ');
                    int nichr = 1;
                    itoc(ctx, pchr + 1, s, nichr);
                    inpter(ctx, PERROR, L.pos.data() + 1,
                           "NAME must be shorter than " + s.substr(0, static_cast<std::size_t>(nichr - 1)) +
                           " characters.");
                    L.pos[PCHAR] = L.pos[PCHAR] + nchr;
                    L.lexok = false;
                }
            }
            break;
        }
    }
    putbak(ctx, chr);
    while (true) {
        if (isname) {
            if (astrng[static_cast<std::size_t>(nchr - 1)] == '.') {
                putbak(ctx, astrng[static_cast<std::size_t>(nchr - 1)]);
                nchr = nchr - 1;
                continue;
            }
        }
        break;
    }
    return isname;
}

// qquote.f
bool qquote(X13Context& ctx, std::string& astrng, int& nchr) {
    LexState& L = ctx.lex;
    char dmychr;
    int pchr = static_cast<int>(astrng.size());
    nchr = 0;
    char chr = getchr(ctx, dmychr);
    bool isquote;
    if (chr != '"' && chr != '\'') {
        isquote = false;
        putbak(ctx, chr);
    } else {
        isquote = true;
        nchr = 0;
        char qchr = chr;
        while (true) {
            chr = getchr(ctx, dmychr);
            if (chr == L.newlin) {
                L.pos[PCHAR] = L.pos[PCHAR] - nchr - 2;
                inpter(ctx, PERROR, L.pos.data() + 1,
                       "Quote can't wrap to next line--end-of-line assumed to be end quote");
                L.pos[PCHAR] = L.pos[PCHAR] + nchr + 2;
                L.lexok = false;
            } else if (nchr >= pchr) {
                L.pos[PCHAR] = L.pos[PCHAR] - nchr;
                std::string s(5, ' ');
                int nichr = 1;
                itoc(ctx, pchr + 1, s, nichr);
                inpter(ctx, PERROR, L.pos.data() + 1,
                       "QUOTE must be shorter than " + s.substr(0, static_cast<std::size_t>(nichr - 1)) +
                       " characters.");
                L.pos[PCHAR] = L.pos[PCHAR] + nchr;
                L.lexok = false;
            } else if (chr != qchr) {
                nchr = nchr + 1;
                astrng[static_cast<std::size_t>(nchr - 1)] = chr;
                continue;
            }
            break;
        }
    }
    if (isquote && nchr == 0) {
        L.pos[PCHAR] = L.pos[PCHAR] - 1;
        inpter(ctx, PERROR, L.pos.data() + 1, "Quotes must contain at least one character.");
        L.pos[PCHAR] = L.pos[PCHAR] + 1;
        L.lexok = false;
    }
    return isquote;
}

// qcmmnt.f
bool qcmmnt(X13Context& ctx, std::string& str, int& nchr) {
    LexState& L = ctx.lex;
    char chr;
    if (getchr(ctx, chr) == '#') {
        int pchr = static_cast<int>(str.size());
        nchr = 0;
        char dmychr;
        while (true) {
            chr = getchr(ctx, dmychr);
            if (nchr >= pchr) {
                L.pos[PCHAR] = L.pos[PCHAR] - nchr;
                std::string s(5, ' ');
                int nichr = 1;
                itoc(ctx, pchr + 1, s, nichr);
                inpter(ctx, PERROR, L.pos.data() + 1,
                       "COMMENT must be shorter than " + s.substr(0, static_cast<std::size_t>(nichr - 1)) +
                       " characters.");
                L.pos[PCHAR] = L.pos[PCHAR] + nchr;
            } else if (chr == L.chreof) {
                putbak(ctx, chr);
            } else if (chr != L.newlin) {
                nchr = nchr + 1;
                str[static_cast<std::size_t>(nchr - 1)] = chr;
                continue;
            }
            break;
        }
        return true;
    } else {
        putbak(ctx, chr);
        return false;
    }
}

// qintgr.f -- writes into str beginning at offset off (0-based). Returns count in nchr.
bool qintgr(X13Context& ctx, std::string& str, int off, int& nchr) {
    LexState& L = ctx.lex;
    char dmychr;
    int pchr = static_cast<int>(str.size()) - off;
    bool isint = false;
    nchr = 0;
    while (true) {
        if (nchr <= pchr) {
            char chr = getchr(ctx, dmychr);
            if (chr >= CZERO && chr <= CNINE) {
                isint = true;
                nchr = nchr + 1;
                str[static_cast<std::size_t>(off + nchr - 1)] = chr;
                continue;
            }
            putbak(ctx, chr);
        }
        break;
    }
    (void)L;
    return isint;
}

// qdoble.f
bool qdoble(X13Context& ctx, std::string& str, int& nchr, bool& alsoin) {
    char dmychr;
    bool isdbl = false;
    alsoin = false;
    int nchr2 = 0;
    char sgnchr = getchr(ctx, dmychr);
    bool havsgn;
    if (sgnchr == '+' || sgnchr == '-') {
        havsgn = true;
        nchr = 1;
        str[static_cast<std::size_t>(nchr - 1)] = sgnchr;
    } else {
        havsgn = false;
        putbak(ctx, sgnchr);
        nchr = 0;
    }
    isdbl = qintgr(ctx, str, nchr, nchr2);
    alsoin = isdbl;
    if (isdbl) nchr = nchr + nchr2;

    char decchr = getchr(ctx, dmychr);
    if (decchr == '.') {
        alsoin = false;
        nchr = nchr + 1;
        str[static_cast<std::size_t>(nchr - 1)] = decchr;
        if (qintgr(ctx, str, nchr, nchr2)) {
            isdbl = true;
            nchr = nchr + nchr2;
        } else if (!isdbl) {
            putbak(ctx, decchr);
            nchr = nchr - 1;
        }
    } else {
        putbak(ctx, decchr);
    }

    if (!isdbl && havsgn) {
        putbak(ctx, sgnchr);
        nchr = 0;
    }

    if (isdbl) {
        char expchr = getchr(ctx, dmychr);
        if (indx("eEdD^", expchr) == 0) {
            putbak(ctx, expchr);
        } else {
            nchr = nchr + 1;
            str[static_cast<std::size_t>(nchr - 1)] = expchr;
            sgnchr = getchr(ctx, dmychr);
            if (sgnchr == '+' || sgnchr == '-') {
                havsgn = true;
                nchr = nchr + 1;
                str[static_cast<std::size_t>(nchr - 1)] = sgnchr;
            } else {
                havsgn = false;
                putbak(ctx, sgnchr);
            }
            if (qintgr(ctx, str, nchr, nchr2)) {
                alsoin = false;
                nchr = nchr + nchr2;
            } else {
                if (havsgn) {
                    putbak(ctx, sgnchr);
                    nchr = nchr - 1;
                }
                putbak(ctx, expchr);
                nchr = nchr - 1;
            }
        }
    }
    return isdbl;
}

// qtoken.f
void qtoken(X13Context& ctx) {
    LexState& L = ctx.lex;
    char dmychr;
    char chr = getchr(ctx, dmychr);
    if (chr == L.chreof || chr == L.newlin) {
        putbak(ctx, chr);
    } else {
        int toktyp;
        if (chr == '{') toktyp = LBRACE;
        else if (chr == '}') toktyp = RBRACE;
        else if (chr == '(') toktyp = LPAREN;
        else if (chr == ')') toktyp = RPAREN;
        else if (chr == '[') toktyp = LBRAKT;
        else if (chr == ']') toktyp = RBRAKT;
        else if (chr == ',') toktyp = COMMA;
        else if (chr == '+') toktyp = PLUS;
        else if (chr == '-') toktyp = MINUS;
        else if (chr == '=') toktyp = EQUALS;
        else if (chr == '.') toktyp = PERIOD;
        else if (chr == '/') toktyp = SLASH;
        else if (chr == '*') toktyp = STAR;
        else toktyp = BADTOK;
        L.nxtkln = 1;
        L.nxtktp = toktyp;
        tok1(L, 1) = chr;
    }
}

// lex.f
void lex(X13Context& ctx) {
    LexState& L = ctx.lex;
    bool alsoin = false;
    while (whitsp(ctx) != EOFTOK) {
        if (qcmmnt(ctx, L.nxttok, L.nxtkln)) {
            L.nxtktp = COMMNT;
        } else if (qquote(ctx, L.nxttok, L.nxtkln)) {
            L.nxtktp = QUOTE;
        } else if (qname(ctx, L.nxttok, L.nxtkln)) {
            L.nxtktp = NAME;
        } else if (qdoble(ctx, L.nxttok, L.nxtkln, alsoin)) {
            L.nxtktp = alsoin ? INTGR : DBL;
        } else {
            qtoken(ctx);
        }
        if (L.nxtktp != COMMNT) goto done10;
    }
    // EOF
    L.nxtktp = EOFTOK;
    tok1(L, 1) = L.chreof;
    L.nxtkln = 1;
done10:
    // Lstpos <- Errpos copy in Fortran is CALL cpyint(Lstpos,2,1,Errpos):
    // that copies Lstpos into Errpos (src=Lstpos, dst=Errpos).
    L.errpos[PLINE] = L.lstpos[PLINE];
    L.errpos[PCHAR] = L.lstpos[PCHAR];
    L.lstpos[PLINE] = L.pos[PLINE];
    L.lstpos[PCHAR] = L.pos[PCHAR] - L.nxtkln;
    if (L.nxtktp == QUOTE) L.lstpos[PCHAR] = L.lstpos[PCHAR] - 2;
}

} // namespace x13
