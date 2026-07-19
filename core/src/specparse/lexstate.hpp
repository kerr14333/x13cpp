// lexstate.hpp -- lexer/parser state for the spec-file input subsystem.
//
// This mirrors the Fortran COMMON blocks that live in the .i includes the
// lexer uses (lex.i /clex/ and cchars.i /cchars/) plus the SAVEd ring-buffer
// state that rngbuf.f keeps across calls. These .i-based COMMONs are not
// generated into core/src/common/gen (the generator only covered .cmn files),
// so the state is captured here and threaded through X13Context as ctx.lex.
//
// Numeric token-type / parameter values are taken verbatim from lex.i so the
// ported code compares against the exact same constants.
#ifndef X13_SPECPARSE_LEXSTATE_HPP
#define X13_SPECPARSE_LEXSTATE_HPP

#include <array>
#include <string>
#include <vector>

namespace x13 {

// --- PARAMETERs from lex.i -------------------------------------------------
namespace lexprm {
inline constexpr int PLINE = 1;
inline constexpr int PCHAR = 2;
inline constexpr int LINLEN = 133;
inline constexpr int PBUFSZ = 3;

// inpter error-type codes (lex.i)
inline constexpr int PERROR = 1;
inline constexpr int PWARN = 2;
inline constexpr int PERRNP = 3;
inline constexpr int PWRNNP = 4;

// token type codes
inline constexpr int BADTOK = 21;
inline constexpr int COMMA = 12;
inline constexpr int COMMNT = 35;
inline constexpr int NAME = 31;
inline constexpr int QUOTE = 34;
inline constexpr int INTGR = 48;
inline constexpr int DBL = 101;
inline constexpr int LBRACE = 123;
inline constexpr int RBRACE = 125;
inline constexpr int LPAREN = 40;
inline constexpr int RPAREN = 41;
inline constexpr int LBRAKT = 91;
inline constexpr int RBRAKT = 93;
inline constexpr int NULLTOK = 0;   // NULL in Fortran
inline constexpr int PERIOD = 46;
inline constexpr int PLUS = 43;
inline constexpr int MINUS = 45;
inline constexpr int EQUALS = 61;
inline constexpr int EOFTOK = 26;   // EOF in Fortran
inline constexpr int SLASH = 47;
inline constexpr int STAR = 42;
inline constexpr int COLON = 58;
inline constexpr int BSLASH = 92;

inline constexpr char BIGA = 'A', BIGZ = 'Z';
inline constexpr char CNINE = '9', CZERO = '0';
inline constexpr char LITTLA = 'a', LITTLZ = 'z';
} // namespace lexprm

// Mirror of COMMON /clex/ (lex.i), /cchars/ (cchars.i), plus rngbuf SAVE state
// and the in-memory spec-file input source.
struct LexState {
    // /clex/
    std::array<int, 3> pos{{0, 0, 0}};       // Pos(2): [1]=line, [2]=char
    int lineln = 0;                          // Lineln
    int lineno = 0;                          // Lineno
    int inputx = 0;                          // Inputx (unit number; informational)
    std::array<int, 3> errpos{{0, 0, 0}};    // Errpos(2)
    std::array<int, 3> lstpos{{0, 0, 0}};    // Lstpos(2)
    int nxtkln = 0;                          // Nxtkln
    int nxtktp = 0;                          // Nxtktp
    bool lexok = true;                       // Lexok
    // Linex is CHARACTER*(LINLEN+1); 1-based access via linchr().
    std::string linex = std::string(lexprm::LINLEN + 1, ' ');   // Linex
    std::string nxttok = std::string(lexprm::LINLEN, ' ');      // Nxttok*(LINLEN)

    // /cchars/
    char chreof = '\x1a';   // CHREOF (ASCII SUB / EOF marker)
    char newlin = '\n';     // NEWLIN
    char tabchr = '\t';     // TABCHR

    // rngbuf.f SAVE state
    std::array<std::string, lexprm::PBUFSZ> buf{
        {std::string(lexprm::LINLEN, ' '), std::string(lexprm::LINLEN, ' '),
         std::string(lexprm::LINLEN, ' ')}};
    std::array<int, lexprm::PBUFSZ> bufln{{0, 0, 0}};
    int begbuf = 0, endbuf = 0, crntbf = 0, crntln = 0;
    bool psteof = false;

    // In-memory input source (replaces the Fortran READ(Inputx,...) unit).
    std::vector<std::string> input_lines;
    std::size_t read_idx = 0;

    // 1-based accessor for Linex (like Linex(i:i)).
    char& linchr(int i) { return linex[static_cast<std::size_t>(i - 1)]; }
    char linchr(int i) const { return linex[static_cast<std::size_t>(i - 1)]; }

    void load(const std::string& text);
};

// Instrumentation (not a Fortran COMMON): key parsed settings captured during
// M1 spec parsing so x13parse can echo them and the parity harness can check
// them. Populated by the spec readers as they consume arguments.
struct ParseSettings {
    int period = 0;                     // Sp
    int nobs = 0;                       // Nobs
    std::array<int, 2> series_start{{0, 0}};  // Begsrs (year, period)
    std::array<int, 2> span_start{{0, 0}};    // Begspn
    std::array<int, 2> span_end{{0, 0}};      // Endspn
    bool has_series = false;
    std::string data_file;
    std::string title;
    std::string transform_function;     // transform{ function = ... }
    double transform_power = -999.0;     // transform{ power = ... } (DNOTST sentinel)
    std::vector<std::string> save_tables;   // all requested save=(...) extensions
    std::string model_desc;             // arima{ model = ... } text
    bool has_model = false;             // Lmodel (a model spec was present)
    int forecast_maxlead = -1;          // forecast{ maxlead = ... }
    std::string x11_mode;               // x11{ mode = ... }
    bool has_x11 = false;
    bool has_seats = false;
    std::vector<std::string> regression_vars;   // regression{ variables = ... }
    std::vector<std::string> aictest_vars;      // regression{ aictest = ... }
    std::vector<std::string> spec_order;        // spec names in order encountered
};

} // namespace x13

#endif // X13_SPECPARSE_LEXSTATE_HPP
