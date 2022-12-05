#ifndef COUNT_ACTIVE_OBJECT_HPP
#define COUNT_ACTIVE_OBJECT_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Sync/Condition.hpp>
#include <Generics/ActiveObject.hpp>

#include <Commons/AtomicInt.hpp>

namespace AdServer
{
namespace Commons
{
  /* CountActiveObject
   *   ActiveObject implementation
   *   wait active_count dropping(<= 0) on wait_object
   */
  class CountActiveObject:
    public Generics::SimpleActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    CountActiveObject() noexcept;

    bool
    add_active_count(int inc, bool ignore_state = false) noexcept;

    int
    active_count() const noexcept;

  protected:
    virtual
    ~CountActiveObject() noexcept = default;

    virtual
    bool
    wait_more_() noexcept;

  private:
    Algs::AtomicInt active_count_;
  };
  typedef ReferenceCounting::QualPtr<CountActiveObject>
    CountActiveObject_var;
}
}

#endif /*COUNT_ACTIVE_OBJECT_HPP*/
