// version.hpp -- version identification for the X13cpp port.
#ifndef X13_VERSION_HPP
#define X13_VERSION_HPP

namespace x13 {

// Upstream Census X-13ARIMA-SEATS release this port is faithful to.
const char* upstream_version();
// This port's own version string.
const char* port_version();

}  // namespace x13

#endif  // X13_VERSION_HPP
