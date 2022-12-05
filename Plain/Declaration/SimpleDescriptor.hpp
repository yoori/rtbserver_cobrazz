#ifndef _SIMPLEDESCRIPTOR_HPP_
#define _SIMPLEDESCRIPTOR_HPP_

#include <ReferenceCounting/DefaultImpl.hpp>
#include "BaseDescriptor.hpp"

namespace Declaration
{
  class SimpleDescriptor: public virtual BaseDescriptor
  {
  public:
    SimpleDescriptor(
      const char* name,
      bool is_fixed_val,
      SizeType fixed_size_val)
      noexcept;

    virtual bool is_fixed() const noexcept;

    virtual SizeType fixed_size() const noexcept;

    virtual SimpleDescriptor_var as_simple() noexcept;

  protected:
    virtual ~SimpleDescriptor() noexcept {}
    
  private:
    bool is_fixed_;
    SizeType fixed_size_;
  };

  typedef ReferenceCounting::SmartPtr<SimpleDescriptor>
    SimpleDescriptor_var;
}

namespace Declaration
{
  inline
  SimpleDescriptor::SimpleDescriptor(
    const char* name_val,
    bool is_fixed_val,
    SizeType fixed_size_val)
    noexcept
    : BaseType(name_val),
      BaseDescriptor(name_val),
      is_fixed_(is_fixed_val),
      fixed_size_(fixed_size_val)
  {}
  
  inline
  bool
  SimpleDescriptor::is_fixed() const noexcept
  {
    return is_fixed_;
  }

  inline
  SizeType
  SimpleDescriptor::fixed_size() const noexcept
  {
    return fixed_size_;
  }

  inline
  SimpleDescriptor_var
  SimpleDescriptor::as_simple() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }
}

#endif /*_SIMPLEDESCRIPTOR_HPP_*/
