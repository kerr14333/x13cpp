// channels.hpp -- output channel registry mapping Fortran unit numbers to buffers.
//
// The Fortran writes to logical unit numbers held in COMMON /units/ (units.cmn):
//   Mt   input spec file        Mtm  model file
//   Mt1  main printout          Mt2  error file
//   Ng   run summary/log        Nform seasonal-adjustment diagnostics (udg)
//   Mtprof profiling
//
// This registry lets ported WRITE(unit,...) statements resolve a unit int to an
// OutBuffer. OutBuffer carries a carriage-control awareness stub: Fortran
// formatted output historically interpreted column 1 of each record as a
// carriage-control character ('1'=new page, '0'=double space, '+'=overprint,
// ' '=single space). Modern x13as emits plain text, so the stub records the
// control column but defaults to passthrough.
#ifndef X13_CHANNELS_HPP
#define X13_CHANNELS_HPP

#include <map>
#include <sstream>
#include <string>
#include <string_view>

namespace x13 {

class OutBuffer {
public:
    // Append a fully formatted record (no trailing newline expected).
    // If carriage_control is enabled, the first character is interpreted.
    void put_record(std::string_view rec) {
        if (carriage_control_ && !rec.empty()) {
            char cc = rec.front();
            std::string_view body = rec.substr(1);
            switch (cc) {
                case '1': os_ << '\f'; break;   // form feed / new page
                case '0': os_ << '\n'; break;   // double space (extra blank line)
                case '+': /* overprint: no leading newline */ break;
                case ' ':
                default:  break;                // single space
            }
            os_ << body << '\n';
        } else {
            os_ << rec << '\n';
        }
    }

    // Append raw text with no record/carriage-control processing.
    void put(std::string_view s) { os_ << s; }

    std::string str() const { return os_.str(); }
    void clear() { os_.str(std::string()); os_.clear(); }

    void set_carriage_control(bool on) { carriage_control_ = on; }
    bool carriage_control() const { return carriage_control_; }

private:
    std::ostringstream os_;
    bool carriage_control_ = false;
};

// Registry keyed by Fortran unit number.
class ChannelRegistry {
public:
    OutBuffer& unit(int u) { return buffers_[u]; }        // creates on first use
    bool has(int u) const { return buffers_.count(u) != 0; }
    void reset() { buffers_.clear(); }

private:
    std::map<int, OutBuffer> buffers_;
};

} // namespace x13

#endif // X13_CHANNELS_HPP
