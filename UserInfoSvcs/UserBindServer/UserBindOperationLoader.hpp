#ifndef USERBINDSERVER_USERBINDOPERATIONLOADER_HPP
#define USERBINDSERVER_USERBINDOPERATIONLOADER_HPP

#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>

#include <Generics/CompositeActiveObject.hpp>

#include "UserBindOperationSaver.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  // UserBindOperationLoader
  class UserBindOperationLoader:
    public virtual ReferenceCounting::AtomicImpl,
    public virtual Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::set<unsigned long> ChunkIdSet;

    UserBindOperationLoader(
      Generics::ActiveObjectCallback* callback,
      UserBindProcessor* user_bind_processor,
      const char* folder,
      const char* unprocessed_folder,
      const char* file_prefix,
      const Generics::Time& check_period,
      std::size_t threads_count,
      const ChunkIdSet& chunk_ids)
      noexcept;

  protected:
    virtual
    ~UserBindOperationLoader() noexcept;
  };

  typedef ReferenceCounting::SmartPtr<UserBindOperationLoader>
    UserBindOperationLoader_var;
}
}

#endif /*USERBINDSERVER_USERBINDOPERATIONLOADER_HPP*/
