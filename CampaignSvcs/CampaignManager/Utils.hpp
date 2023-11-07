#ifndef RTBSERVER_COBRAZZ_CAMPAIGNMANAGER_UTILS_HPP
#define RTBSERVER_COBRAZZ_CAMPAIGNMANAGER_UTILS_HPP

// STD
#include <iostream>
#include <string>

namespace std
{

inline ostream &operator<<(ostream &os, const char8_t *str)
{
  return os << reinterpret_cast<const char*>(str);
}

} // namespace std

#endif // RTBSERVER_COBRAZZ_CAMPAIGNMANAGER_UTILS_HPP
