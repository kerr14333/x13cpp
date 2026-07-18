// fformat.hpp -- Fortran formatted-WRITE output engine for the X13cpp port.
//
// Implements the FORMAT edit descriptors used by x13as output:
//   Iw / Iw.m           integer
//   Fw.d                fixed decimal
//   Ew.d / Ew.dEe       scientific (mantissa 0.ddd x 10^exp by default)
//   Gw.d / Gw.dEe       general (F when in range, else E)
//   Aw / A              character
//   nX                  spaces
//   Tc / TRn / TLn      tab / tab-right / tab-left positioning
//   kP                  scale factor (affects following F/E/G)
//   'literal' "literal" literal text
//   /                   record separator (newline)
//   n(...)              repeat groups
//
// Rounding: numeric conversions go through the C runtime (snprintf), whose
// default IEEE-754 rounding is round-to-nearest-ties-to-even -- matching
// gfortran's default RN mode at the printed digit. Field overflow fills the
// field with '*' (Fortran behaviour).
//
// API:
//   std::string fwrite_fmt(std::string_view fmt, args...);   // one-shot
//   class FormatWriter;                                       // incremental
//
// The engine is validated against gfortran itself (tests/unit/fformat_ref.f90).
#ifndef X13_FFORMAT_HPP
#define X13_FFORMAT_HPP

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <cstdint>

namespace x13 {

// ---------------------------------------------------------------------------
// Argument holder.
// ---------------------------------------------------------------------------
class FmtArg {
public:
    enum class Kind { Int, Real, Str, Logical };

    FmtArg(int v)          : k_(Kind::Int),     i_(v) {}
    FmtArg(long v)         : k_(Kind::Int),     i_(v) {}
    FmtArg(long long v)    : k_(Kind::Int),     i_(v) {}
    FmtArg(unsigned v)     : k_(Kind::Int),     i_(static_cast<long long>(v)) {}
    FmtArg(double v)       : k_(Kind::Real),    r_(v) {}
    FmtArg(float v)        : k_(Kind::Real),    r_(static_cast<double>(v)) {}
    FmtArg(bool v)         : k_(Kind::Logical), i_(v ? 1 : 0) {}
    FmtArg(const char* v)  : k_(Kind::Str),     s_(v) {}
    FmtArg(std::string v)  : k_(Kind::Str),     s_(std::move(v)) {}
    FmtArg(std::string_view v) : k_(Kind::Str), s_(v) {}

    Kind kind() const { return k_; }
    long long as_int() const { return i_; }
    double as_real() const { return (k_ == Kind::Real) ? r_ : static_cast<double>(i_); }
    const std::string& as_str() const { return s_; }
    bool as_logical() const { return i_ != 0; }

private:
    Kind k_;
    long long i_ = 0;
    double r_ = 0.0;
    std::string s_;
};

// ---------------------------------------------------------------------------
// Parsed edit descriptor.
// ---------------------------------------------------------------------------
namespace detail {

struct Desc {
    enum Type { Int, Real_F, Real_E, Real_G, Char, Logical, Space, Literal,
                Slash, TabAbs, TabRight, TabLeft, Scale, GroupOpen, GroupClose,
                Colon } type;
    int repeat = 1;   // repeat count
    int w = 0;        // field width
    int d = 0;        // digits after decimal
    int e = 0;        // exponent width (0 = default 2)
    int m = 0;        // min digits (Iw.m)
    bool has_d = false;
    bool has_w = false;
    bool has_e = false;
    std::string lit;  // literal text
};

// Tokenize + parse a format string (without the outer parentheses required).
class FormatParser {
public:
    explicit FormatParser(std::string_view fmt) : s_(fmt) {}

    std::vector<Desc> parse() {
        std::vector<Desc> out;
        skip_outer_parens();
        parse_list(out);
        return out;
    }

private:
    std::string_view s_;
    std::size_t p_ = 0;

    void skip_outer_parens() {
        std::size_t a = 0, b = s_.size();
        while (a < b && std::isspace((unsigned char)s_[a])) ++a;
        while (b > a && std::isspace((unsigned char)s_[b-1])) --b;
        if (a < b && s_[a] == '(' && s_[b-1] == ')') { s_ = s_.substr(a+1, b-a-2); }
        else { s_ = s_.substr(a, b-a); }
        p_ = 0;
    }

    void skip_ws() { while (p_ < s_.size() && std::isspace((unsigned char)s_[p_])) ++p_; }

    int read_int(bool& got) {
        skip_ws();
        int v = 0; got = false;
        while (p_ < s_.size() && std::isdigit((unsigned char)s_[p_])) {
            v = v * 10 + (s_[p_] - '0'); ++p_; got = true;
        }
        return v;
    }

    void parse_list(std::vector<Desc>& out) {
        while (p_ < s_.size()) {
            skip_ws();
            if (p_ >= s_.size()) break;
            char c = s_[p_];
            if (c == ',') { ++p_; continue; }
            if (c == '/') { ++p_; Desc d; d.type = Desc::Slash; out.push_back(d); continue; }
            if (c == ':') { ++p_; Desc d; d.type = Desc::Colon; out.push_back(d); continue; }
            if (c == '\'' || c == '"') { out.push_back(read_literal()); continue; }
            if (c == ')') { ++p_; return; }

            // Leading integer: repeat count, scale factor, tab, or Hollerith-less.
            bool got = false;
            int n = read_int(got);
            skip_ws();
            if (p_ >= s_.size()) break;
            c = s_[p_];

            if (c == '(') {
                ++p_;
                Desc open; open.type = Desc::GroupOpen; open.repeat = got ? n : 1;
                out.push_back(open);
                parse_list(out);
                Desc close; close.type = Desc::GroupClose;
                out.push_back(close);
                continue;
            }
            if (c == 'P' || c == 'p') {
                ++p_;
                Desc d; d.type = Desc::Scale; d.repeat = got ? n : 0; d.w = got ? n : 0;
                // scale value stored in d.m
                d.m = got ? n : 0;
                out.push_back(d);
                continue;
            }
            out.push_back(read_descriptor(got ? n : 1));
        }
    }

    Desc read_literal() {
        char q = s_[p_]; ++p_;
        Desc d; d.type = Desc::Literal;
        std::string lit;
        while (p_ < s_.size()) {
            char c = s_[p_++];
            if (c == q) {
                if (p_ < s_.size() && s_[p_] == q) { lit.push_back(q); ++p_; } // doubled quote
                else break;
            } else lit.push_back(c);
        }
        d.lit = lit;
        return d;
    }

    Desc read_descriptor(int repeat) {
        Desc d; d.repeat = repeat;
        char c = s_[p_++];
        char up = (char)std::toupper((unsigned char)c);
        switch (up) {
            case 'I': d.type = Desc::Int;    read_w_d(d, /*allow_d=*/true, /*d_is_m=*/true); break;
            case 'F': d.type = Desc::Real_F; read_w_d(d, true, false); break;
            case 'E': d.type = Desc::Real_E; read_w_d_e(d); break;
            case 'D': d.type = Desc::Real_E; read_w_d_e(d); break; // D like E
            case 'G': d.type = Desc::Real_G; read_w_d_e(d); break;
            case 'A': d.type = Desc::Char;   read_optional_w(d); break;
            case 'L': d.type = Desc::Logical; read_optional_w(d); break;
            case 'X': d.type = Desc::Space;  d.w = repeat; break;   // nX -> repeat spaces
            case 'T':
                if (p_ < s_.size() && (s_[p_]=='R'||s_[p_]=='r')) { ++p_; d.type=Desc::TabRight; bool g; d.w=read_int(g); }
                else if (p_ < s_.size() && (s_[p_]=='L'||s_[p_]=='l')) { ++p_; d.type=Desc::TabLeft; bool g; d.w=read_int(g); }
                else { d.type = Desc::TabAbs; bool g; d.w = read_int(g); }
                break;
            default:
                // Unknown: treat as literal char.
                d.type = Desc::Literal; d.lit = std::string(1, c); break;
        }
        return d;
    }

    void read_optional_w(Desc& d) {
        skip_ws();
        if (p_ < s_.size() && std::isdigit((unsigned char)s_[p_])) { d.w = read_int(d.has_w); }
    }

    void read_w_d(Desc& d, bool allow_d, bool d_is_m) {
        d.w = read_int(d.has_w);
        skip_ws();
        if (allow_d && p_ < s_.size() && s_[p_] == '.') {
            ++p_;
            int v = read_int(d.has_d);
            if (d_is_m) { d.m = v; } else { d.d = v; }
        }
    }

    void read_w_d_e(Desc& d) {
        d.w = read_int(d.has_w);
        skip_ws();
        if (p_ < s_.size() && s_[p_] == '.') { ++p_; d.d = read_int(d.has_d); }
        skip_ws();
        if (p_ < s_.size() && (s_[p_] == 'E' || s_[p_] == 'e')) {
            ++p_; d.e = read_int(d.has_e);
        }
    }
};

// ---------------------------------------------------------------------------
// Numeric formatters.
// ---------------------------------------------------------------------------

inline std::string fill_stars(int w) { return std::string(w > 0 ? w : 1, '*'); }

inline std::string right_justify(const std::string& s, int w) {
    if ((int)s.size() >= w) return s;
    return std::string(w - s.size(), ' ') + s;
}

// Iw / Iw.m
inline std::string fmt_int(long long v, int w, int m, bool has_m) {
    bool neg = v < 0;
    unsigned long long uv = neg ? (unsigned long long)(-(v+1)) + 1ULL : (unsigned long long)v;
    std::string digits = std::to_string(uv);
    if (has_m && (int)digits.size() < m) digits = std::string(m - digits.size(), '0') + digits;
    // Iw.m with m==0 and value 0 => blanks
    if (has_m && m == 0 && v == 0) digits = "";
    std::string body = (neg ? "-" : "") + digits;
    if ((int)body.size() > w) return fill_stars(w);
    return right_justify(body, w);
}

// Fw.d  (scale = current P scale factor)
inline std::string fmt_f(double v, int w, int d, int scale) {
    if (std::isnan(v)) { std::string s = "NaN"; return (int)s.size() > w ? fill_stars(w) : right_justify(s, w); }
    if (std::isinf(v)) { std::string s = v < 0 ? "-Inf" : "Inf"; return (int)s.size() > w ? fill_stars(w) : right_justify(s, w); }
    double scaled = v;
    if (scale != 0) scaled = v * std::pow(10.0, scale);
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%.*f", d, scaled);
    std::string body(buf);
    // Fortran F editing always prints the decimal point, even with d==0 ("2."),
    // whereas C's %.0f omits it. Append it to match.
    if (d == 0) body += '.';
    // Note: Fortran KEEPS the minus sign for a negative value that rounds to zero
    // (e.g. F10.4 of -1e-5 -> "-0.0000"), matching snprintf. Do not strip it.
    if ((int)body.size() > w) return fill_stars(w);
    return right_justify(body, w);
}

// Build exponent field: E form.  exp is the power; ewidth default logic.
inline std::string exp_field(int exp, int e, char letter) {
    bool neg = exp < 0;
    int a = neg ? -exp : exp;
    std::string digits = std::to_string(a);
    int width = e; // explicit exponent width if given
    if (width == 0) {
        // default: 2 digits; if exp doesn't fit in 2, drop the letter (3 digits).
        if (a > 99) {
            // No exponent letter; sign + 3 digits.
            if ((int)digits.size() < 3) digits = std::string(3 - digits.size(), '0') + digits;
            return std::string(neg ? "-" : "+") + digits;
        }
        width = 2;
    }
    if ((int)digits.size() < width) digits = std::string(width - digits.size(), '0') + digits;
    return std::string(1, letter) + (neg ? "-" : "+") + digits;
}

// Ew.d[Ee] with scale factor.
//
// Default (scale k==0): mantissa normalized to [0.1,1), form "0.d1..dd" so there
// are d fractional digits. Scale k>0 (e.g. 1P): k integer digits, (d-k+1)
// fractional digits, mantissa in [10^(k-1),10^k). Exponent field via exp_field.
inline std::string fmt_e(double v, int w, int d, int e, int scale, char letter) {
    if (std::isnan(v) || std::isinf(v)) return fmt_f(v, w, d, 0);
    bool neg = std::signbit(v);
    double av = std::fabs(v);
    int k = scale;
    int frac = (k > 0) ? (d - k + 1) : d;
    if (frac < 0) frac = 0;

    int exp;
    std::string digits;
    if (av == 0.0) {
        exp = 0;
        if (k > 0) digits = std::string(k, '0') + "." + std::string(frac, '0');
        else       digits = "0." + std::string(frac, '0');
    } else {
        // Extract correctly-rounded significant digits directly from the value via
        // %e (round-half-even from the true double), then rearrange into Fortran's
        // scaled mantissa form. This avoids the FP error of dividing by 10^exp.
        int nsig = (k > 0) ? (k + frac) : frac;   // total significant digits
        if (nsig < 1) nsig = 1;
        char buf[512];
        std::snprintf(buf, sizeof(buf), "%.*e", nsig - 1, av);
        std::string t(buf);
        std::size_t epos = t.find_first_of("eE");
        // significant digits (strip the '.') and the base-10 exponent of digit 0.
        std::string sd;
        for (std::size_t i = 0; i < epos; ++i) if (t[i] != '.') sd += t[i];
        int e10 = std::stoi(t.substr(epos + 1));   // value = sd[0].sd[1..] x 10^e10
        if ((int)sd.size() < nsig) sd += std::string(nsig - sd.size(), '0');
        if (k > 0) {
            exp = e10 - (k - 1);
            digits = sd.substr(0, k) + "." + sd.substr(k);
        } else {
            exp = e10 + 1;
            digits = "0." + sd;
        }
    }
    std::string body = (neg ? "-" : "") + digits + exp_field(exp, e, letter);
    if ((int)body.size() > w) return fill_stars(w);
    return right_justify(body, w);
}

// Gw.d[Ee]: use F when 0.1 <= |v| < 10^d (after rounding), else E.
inline std::string fmt_g(double v, int w, int d, int e, int scale) {
    if (std::isnan(v) || std::isinf(v)) return fmt_f(v, w, d, 0);
    double av = std::fabs(v);
    bool use_f = false;
    int fexp = 0;
    if (av == 0.0) { use_f = true; fexp = 0; }
    else {
        double lg = std::log10(av);
        fexp = (int)std::floor(lg);
        // rounding at d significant digits may push to next power; approximate.
        if (av >= 0.1 && av < std::pow(10.0, (double)d)) use_f = true;
    }
    // Width of exponent portion reserved by G's F-branch = 4 (or e+2).
    int ew = (e > 0) ? (e + 2) : 4;
    if (use_f) {
        // Fortran G F-branch: number of decimals = d - (fexp+1) when value >=1,
        // and the field is (w-ew) wide, then ew trailing blanks appended.
        int nd;
        if (av == 0.0) nd = d - 1 >= 0 ? d - 1 : 0;
        else nd = d - 1 - fexp;
        if (nd < 0) nd = 0;
        std::string f = fmt_f(v, w - ew, nd, 0);
        // if it fit, append ew spaces
        if (f.find('*') == std::string::npos) return f + std::string(ew, ' ');
        // else fall through to E
    }
    return fmt_e(v, w, d, e, scale == 0 ? 0 : scale, 'E');
}

} // namespace detail

// ---------------------------------------------------------------------------
// Incremental writer.
// ---------------------------------------------------------------------------
class FormatWriter {
public:
    explicit FormatWriter(std::string_view fmt)
        : descs_(detail::FormatParser(fmt).parse()) {}

    // Feed all args, produce the formatted string.
    std::string write(const std::vector<FmtArg>& args) {
        out_.clear();
        line_.clear();
        col_ = 0;
        scale_ = 0;
        ai_ = 0;
        args_ = &args;
        run(descs_, 0, descs_.size(), /*top=*/true);
        flush_line();
        return out_;
    }

private:
    std::vector<detail::Desc> descs_;
    std::string out_;
    std::string line_;
    int col_ = 0;
    int scale_ = 0;
    std::size_t ai_ = 0;
    const std::vector<FmtArg>* args_ = nullptr;

    bool have_arg() const { return args_ && ai_ < args_->size(); }
    const FmtArg& next_arg() { return (*args_)[ai_++]; }

    void emit(const std::string& s) {
        // place at current column (col_ == line_.size() normally, but tabs can move it)
        if (col_ == (int)line_.size()) { line_ += s; col_ += (int)s.size(); }
        else {
            if (col_ > (int)line_.size()) line_.append(col_ - (int)line_.size(), ' ');
            for (char c : s) {
                if (col_ < (int)line_.size()) line_[col_] = c; else line_ += c;
                ++col_;
            }
        }
    }

    void flush_line() {
        out_ += line_;
        line_.clear();
        col_ = 0;
    }
    void newline() { flush_line(); out_ += '\n'; }

    // Execute descriptors [begin,end). Returns true if all args consumed / done.
    // Implements format reversion for repeated data descriptors at top level.
    void run(const std::vector<detail::Desc>& ds, std::size_t begin, std::size_t end, bool top) {
        bool progressed = true;
        // For reversion: remember position of last top-level group for re-entry.
        while (true) {
            for (std::size_t i = begin; i < end; ++i) {
                const auto& d = ds[i];
                switch (d.type) {
                    case detail::Desc::GroupOpen: {
                        std::size_t close = match_close(ds, i);
                        for (int r = 0; r < std::max(1, d.repeat); ++r) {
                            if (is_data_only(ds, i+1, close) && !have_arg()) break;
                            run(ds, i + 1, close, false);
                        }
                        i = close;
                        break;
                    }
                    case detail::Desc::GroupClose: break;
                    case detail::Desc::Slash: newline(); break;
                    case detail::Desc::Colon:
                        if (!have_arg()) { return; }
                        break;
                    case detail::Desc::Literal: emit(d.lit); break;
                    // X is positional in Fortran: it advances the cursor but writes
                    // no characters itself. Trailing X therefore adds no trailing
                    // blanks; intermediate gaps are filled only when later data is
                    // written at a higher column (handled by emit()).
                    case detail::Desc::Space: col_ += std::max(1, d.w); break;
                    case detail::Desc::Scale: scale_ = d.m; break;
                    case detail::Desc::TabAbs: col_ = d.w > 0 ? d.w - 1 : 0; break;
                    case detail::Desc::TabRight: col_ += d.w; break;
                    case detail::Desc::TabLeft: col_ = std::max(0, col_ - d.w); break;
                    case detail::Desc::Int:
                    case detail::Desc::Real_F:
                    case detail::Desc::Real_E:
                    case detail::Desc::Real_G:
                    case detail::Desc::Char:
                    case detail::Desc::Logical:
                        for (int r = 0; r < std::max(1, d.repeat); ++r) {
                            if (!have_arg()) return;
                            emit_data(d);
                        }
                        break;
                }
            }
            if (!top) return;
            // Top-level format exhausted. Fortran reversion: if args remain, a new
            // record starts and control reverts to the last top-level '(' group (or
            // whole format if none). Guard against a revert target that consumes no
            // args (e.g. a format with only literals) -- that would loop forever.
            if (!have_arg()) return;
            std::size_t before = ai_;
            newline();
            begin = last_toplevel_group(ds, begin, end);
            if (before == ai_ && !is_data_only(ds, begin, end)) return;
            (void)progressed;
        }
    }

    static std::size_t match_close(const std::vector<detail::Desc>& ds, std::size_t open) {
        int depth = 0;
        for (std::size_t i = open; i < ds.size(); ++i) {
            if (ds[i].type == detail::Desc::GroupOpen) ++depth;
            else if (ds[i].type == detail::Desc::GroupClose) { if (--depth == 0) return i; }
        }
        return ds.size();
    }

    static bool is_data_only(const std::vector<detail::Desc>& ds, std::size_t a, std::size_t b) {
        for (std::size_t i = a; i < b; ++i) {
            auto t = ds[i].type;
            if (t == detail::Desc::Int || t == detail::Desc::Real_F || t == detail::Desc::Real_E ||
                t == detail::Desc::Real_G || t == detail::Desc::Char) return true;
        }
        return false;
    }

    static std::size_t last_toplevel_group(const std::vector<detail::Desc>& ds,
                                           std::size_t begin, std::size_t end) {
        std::size_t found = begin;
        int depth = 0;
        for (std::size_t i = begin; i < end; ++i) {
            if (ds[i].type == detail::Desc::GroupOpen) { if (depth == 0) found = i; ++depth; }
            else if (ds[i].type == detail::Desc::GroupClose) --depth;
        }
        return found;
    }

    void emit_data(const detail::Desc& d) {
        const FmtArg& a = next_arg();
        switch (d.type) {
            case detail::Desc::Int:
                emit(detail::fmt_int(a.as_int(), d.w, d.m, d.m > 0 || (d.has_d))); break;
            case detail::Desc::Real_F:
                emit(detail::fmt_f(a.as_real(), d.w, d.d, scale_)); break;
            case detail::Desc::Real_E:
                emit(detail::fmt_e(a.as_real(), d.w, d.d, d.e, scale_, 'E')); break;
            case detail::Desc::Real_G:
                emit(detail::fmt_g(a.as_real(), d.w, d.d, d.e, scale_)); break;
            case detail::Desc::Char: {
                std::string s;
                if (a.kind() == FmtArg::Kind::Logical) s = a.as_logical() ? "T" : "F";
                else if (a.kind() == FmtArg::Kind::Str) s = a.as_str();
                else if (a.kind() == FmtArg::Kind::Int) s = std::to_string(a.as_int());
                else s = std::to_string(a.as_real());
                if (d.has_w) {
                    // Aw with source longer than w: Fortran prints the leftmost w chars.
                    if ((int)s.size() > d.w) s = s.substr(0, d.w);
                    emit(detail::right_justify(s, d.w));
                } else emit(s);
                break;
            }
            case detail::Desc::Logical: {
                // Lw prints T or F right-justified in width w (default w>=1).
                std::string s = a.as_logical() ? "T" : "F";
                int w = d.has_w ? d.w : 1;
                emit(detail::right_justify(s, w));
                break;
            }
            default: break;
        }
    }
};

// ---------------------------------------------------------------------------
// One-shot API.
// ---------------------------------------------------------------------------
inline std::string fwrite_fmt_v(std::string_view fmt, const std::vector<FmtArg>& args) {
    FormatWriter w(fmt);
    return w.write(args);
}

template <class... Args>
std::string fwrite_fmt(std::string_view fmt, Args&&... args) {
    std::vector<FmtArg> v{ FmtArg(std::forward<Args>(args))... };
    return fwrite_fmt_v(fmt, v);
}

inline std::string fwrite_fmt(std::string_view fmt) {
    return fwrite_fmt_v(fmt, {});
}

} // namespace x13

#endif // X13_FFORMAT_HPP
