#include "GeoLogger.hpp"

namespace AdServer {
namespace LogProcessing {

template <> const char*
GeoLoggerTraits::B::base_name_ = "GeoLogger";
template <> const char*
GeoLoggerTraits::B::signature_ = "<UNDEFINED>";
template <> const char*
GeoLoggerTraits::B::current_version_ = "<UNDEFINED>";

} // namespace LogProcessing
} // namespace AdServer
