#include "Hostname.hpp"

#include <unistd.h>

namespace AdServer::Commons
{
  const std::string& hostname()
  {
    static const std::string value = []() {
      char buffer[256];
      if (::gethostname(buffer, sizeof(buffer)) != 0)
      {
        return std::string();
      }
      buffer[sizeof(buffer) - 1] = 0;
      return std::string(buffer);
    }();

    return value;
  }
}
