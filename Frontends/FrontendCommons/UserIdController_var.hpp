#ifndef FRONTENDS_USERIDCONTROLLER_VAR_HPP
#define FRONTENDS_USERIDCONTROLLER_VAR_HPP

#include <ReferenceCounting/SmartPtr.hpp>

namespace AdServer
{
  class UserIdController;

  typedef ReferenceCounting::SmartPtr<UserIdController>
    UserIdController_var;
}

#endif /*FRONTENDS_USERIDCONTROLLER_VAR_HPP*/
