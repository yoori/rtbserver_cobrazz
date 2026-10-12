#pragma once

#include <eh/Exception.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace AdServer::Commons
{
  struct DbStateChanger: public virtual ReferenceCounting::Interface
  {
    DECLARE_EXCEPTION(NotSupported, eh::DescriptiveException);

    virtual void change_db_state(bool) = 0;
    virtual bool get_db_state() = 0;
  };

  using DbStateChanger_var = ReferenceCounting::SmartPtr<DbStateChanger>;
}
