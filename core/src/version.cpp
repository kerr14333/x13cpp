// version.cpp -- placeholder translation unit for the x13core static library.
//
// Gives the library at least one compiled object while the port is scaffolded.
// Reports the upstream Census X-13ARIMA-SEATS release this port targets.
#include "x13/version.hpp"

namespace x13 {

const char* upstream_version() { return "1.1 b61"; }
const char* port_version() { return "0.0.0-scaffold"; }

}  // namespace x13
