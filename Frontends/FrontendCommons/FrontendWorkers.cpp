#include "FrontendWorkers.hpp"

namespace FrontendCommons
{
  FrontendWorkers::FrontendWorkers(
    Generics::ActiveObjectCallback* callback,
    unsigned long threads)
    : AdServer::Commons::ExecutorPool(callback, threads)
  {}
}
