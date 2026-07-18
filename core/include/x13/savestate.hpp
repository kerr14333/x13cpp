// savestate.hpp -- in-memory capture of "save" tables (the library data surface).
//
// Every table the Fortran writes to a <base>.<ext> save file (savtbl.f/punch.f)
// is ALSO captured here numerically BEFORE any text formatting: a list of
// (date, value) pairs keyed by the table pointer. The tab-separated save-file
// text (byte-identical to the oracle's, including its fixed sp,e-format values)
// is captured alongside so the driver can serialize it to disk exactly like the
// CLI would.
#ifndef X13_SAVESTATE_HPP
#define X13_SAVESTATE_HPP

#include <string>
#include <utility>
#include <vector>

namespace x13 {

// One saved table.
struct SaveTable {
    int itbl = 0;                                   // 1-based table pointer (mdltbl.i)
    std::string ext;                                // trimmed extension, e.g. "a1"
    std::string filename;                           // "<base>.<ext>"
    std::string label;                              // header label, e.g. "a1save.a1 "
    std::vector<std::pair<int, double>> data;       // (rdb date, value); date=YYYYMM or YYYY
    std::string text;                               // full save-file text (savtbl.f output)
};

// All saved tables produced by a run, in production order.
struct SaveState {
    std::vector<SaveTable> tables;

    void clear() { tables.clear(); }
    SaveTable& add() { tables.emplace_back(); return tables.back(); }
};

}  // namespace x13

#endif  // X13_SAVESTATE_HPP
