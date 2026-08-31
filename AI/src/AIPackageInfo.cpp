#include <iostream>

#ifndef AI_PACKAGE_VERSION
#define AI_PACKAGE_VERSION "unknown"
#endif

int main()
{
  std::cout << "foros-server-ai " << AI_PACKAGE_VERSION << '\n';
  return 0;
}
