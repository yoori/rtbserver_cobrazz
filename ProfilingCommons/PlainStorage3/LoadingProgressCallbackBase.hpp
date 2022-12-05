#ifndef LOADINGPROGRESSCALLBACKBASE_HPP
#define LOADINGPROGRESSCALLBACKBASE_HPP

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

namespace AdServer
{
namespace ProfilingCommons
{
  class LoadingProgressCallbackBase :
    public ReferenceCounting::AtomicImpl
  {
  public:
    LoadingProgressCallbackBase() noexcept;

    virtual void post_progress(double value) noexcept;

    virtual void loading_is_finished() noexcept;

  protected:
    virtual ~LoadingProgressCallbackBase() noexcept;
  };
  typedef ReferenceCounting::SmartPtr<LoadingProgressCallbackBase>
    LoadingProgressCallbackBase_var;
}
}

#endif // LOADINGPROGRESSCALLBACKBASE_HPP
