
#ifndef _AUTOTEST_COMMONS_ORM_UTILS_HPP
#define _AUTOTEST_COMMONS_ORM_UTILS_HPP

#include "ORM.hpp"

namespace AutoTest
{
  namespace ORM
  {
    template<typename T>
    void
    Unused(const T& x);
    
    int
    update_(
      std::ostream& strm,
      int counter,
      void* object,
      const ORMObjectMember& var,
      bool set_defaults = false);

    template<typename T>
    void
    setin_(
      DB::Query& query,
      T& var);

    int
    count_(
      int counter,
      void* object,
      const ORMObjectMember& var,
      bool set_defaults = false);

    int
    insert_(
      std::ostream& strm,
      int counter, void* object,
      const ORMObjectMember& var,
      bool set_defaults = false);

    void
    put_var_(
      std::ostream& strm,
      int counter,
      void* object,
      const ORMObjectMember& var,
      bool set_defaults = false);

    template<typename T>
    void
    insertin_(
      DB::Query& query,
      T& var);
  }
}

#include "Utils.ipp"

#endif  // _AUTOTEST_COMMONS_ORM_UTILS_HPP
