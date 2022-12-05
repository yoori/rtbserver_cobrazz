
#include "Utils.hpp"

namespace AutoTest
{
  namespace ORM
  {
    int
    update_(
      std::ostream& strm,
      int counter,
      void* object,
      const ORMObjectMember& var,
      bool set_defaults)
    {
      if(var.value(object).changed())
      {
        if(counter > 1) strm << ", ";
        strm << var.name << " = :" << counter++;
      }
      else if (var.value(object).is_null() && set_defaults && var.default_value)
      {
        if(counter > 1) strm << ", ";
        strm << var.name << " = " <<  var.default_value; counter++;
      }
      return counter;
    }

    int
    count_(
      int counter,
      void* object,
      const ORMObjectMember& var,
      bool set_defaults)
    {
      if(var.value(object).changed() || !var.value(object).is_null() || (set_defaults && var.default_value))
      {
        return counter + 1;
      }
      return counter;
    }

    int
    insert_(
      std::ostream& strm,
      int counter, void* object,
      const ORMObjectMember& var,
      bool set_defaults)
    {
      if(var.value(object).changed() || !var.value(object).is_null() || (set_defaults && var.default_value))
      {
        if(counter > 1) strm << ", ";
        strm << var.name;
        return counter + 1;
      }
      return counter;
    }

    void
    put_var_(
      std::ostream& strm,
      int counter,
      void* object,
      const ORMObjectMember& var,
      bool set_defaults)
    {
      if (var.value(object).changed() || !var.value(object).is_null())
      {
        strm << ":" << counter << ", ";
      }
      else if(set_defaults && var.default_value)
      {
        strm << var.default_value << ", ";
      }
    }
  }
}


