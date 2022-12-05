#ifndef ACCESSACTIVEOBJECT_HPP_
#define ACCESSACTIVEOBJECT_HPP_

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <Generics/ActiveObject.hpp>

#include "CountActiveObject.hpp"

namespace AdServer
{
namespace Commons
{
  // AccessActiveObject: wrapper around _var that allow to
  // stop ptr resolve(Accessor providing) after deactivation
  // and garantee that wait_object will be returned only
  // if all Accessor's released
  //
  // ControlSmartPtrType controllable object
  // SmartPtrType pointer that will be seen by Accessor
  // ControlSmartPtrType must be convertable to SmartPtrType
  //
  template<typename SmartPtrType, typename ControlSmartPtrType = SmartPtrType>
  class AccessActiveObject:
    public Generics::SimpleActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  protected:
    struct ObjectHolder: public ReferenceCounting::AtomicImpl
    {
      ObjectHolder(typename SmartPtrType::Type* obj_val);

      AdServer::Commons::CountActiveObject_var usage_counter;
      SmartPtrType obj;
    };

    typedef ReferenceCounting::SmartPtr<ObjectHolder>
      ObjectHolder_var;

  public:
    class GuardHolder
    {
    public:
      GuardHolder(ObjectHolder* object_holder);

      GuardHolder(GuardHolder&& init);

      virtual ~GuardHolder() noexcept;

      const SmartPtrType&
      get();

      typename SmartPtrType::Type*
      operator->() noexcept;

      const typename SmartPtrType::Type*
      operator->() const noexcept;

      typename SmartPtrType::Type&
      operator*() noexcept;

      const typename SmartPtrType::Type&
      operator*() const noexcept;

    protected:
      ReferenceCounting::SmartPtr<ObjectHolder> object_holder_;
    };

    typedef GuardHolder Accessor;

    //typedef ReferenceCounting::SmartPtr<GuardHolder> Accessor;

  public:
    AccessActiveObject() noexcept;

    Accessor
    get_accessor() noexcept;

    AccessActiveObject&
    operator=(const ControlSmartPtrType& obj);

    const ControlSmartPtrType&
    get_object() noexcept;

  protected:
    virtual
    ~AccessActiveObject() noexcept = default;

    virtual
    void
    activate_object_() /*throw(Exception, eh::Exception)*/;

    virtual
    void
    deactivate_object_() /*throw(Exception, eh::Exception)*/;

    virtual
    void
    wait_object_() /*throw(Exception, eh::Exception)*/;

  private:
    // mediator for deactivated or non initialized states
    ReferenceCounting::FixedPtr<ObjectHolder> null_object_holder_;
    ReferenceCounting::PtrHolder<ObjectHolder_var> ptr_holder_;
    ControlSmartPtrType obj_;
  };
}
}

namespace AdServer
{
namespace Commons
{
  template<typename SmartPtrType, typename ControlSmartPtrType>
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::
  ObjectHolder::ObjectHolder(typename SmartPtrType::Type* obj_val)
    : usage_counter(new AdServer::Commons::CountActiveObject()),
      obj(ReferenceCounting::add_ref(obj_val))
  {}

  // AccessActiveObject<SmartPtrType, ControlSmartPtrType>::GuardHolder
  template<typename SmartPtrType, typename ControlSmartPtrType>
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::
  GuardHolder::GuardHolder(ObjectHolder* object_holder)
    : object_holder_(ReferenceCounting::add_ref(object_holder))
  {}

  template<typename SmartPtrType, typename ControlSmartPtrType>
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::
  GuardHolder::GuardHolder(GuardHolder&& init)
  {
    object_holder_.swap(init.object_holder_);
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::
  GuardHolder::~GuardHolder() noexcept
  {
    if(object_holder_->obj)
    {
      object_holder_->usage_counter->add_active_count(-1);
    }
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  const SmartPtrType&
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::
  GuardHolder::get()
  {
    return object_holder_->obj;
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  typename SmartPtrType::Type*
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::
  GuardHolder::operator->() noexcept
  {
    return object_holder_->obj;
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  const typename SmartPtrType::Type*
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::
  GuardHolder::operator->() const noexcept
  {
    return object_holder_->obj;
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  typename SmartPtrType::Type&
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::
  GuardHolder::operator*() noexcept
  {
    return *(object_holder_->obj);
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  const typename SmartPtrType::Type&
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::
  GuardHolder::operator*() const noexcept
  {
    return *(object_holder_->obj);
  }

  // AccessActiveObject impl
  template<typename SmartPtrType, typename ControlSmartPtrType>
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::AccessActiveObject()
    noexcept
    : null_object_holder_(new ObjectHolder(SmartPtrType()))
  {}

  template<typename SmartPtrType, typename ControlSmartPtrType>
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>&
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::operator=(
    const ControlSmartPtrType& obj_val)
  {
    ControlSmartPtrType new_obj = obj_val;
    ObjectHolder_var current_object_holder;

    {
      // active object state lock (serialize relative activate_object)
      Sync::PosixGuard guard(cond_);
      current_object_holder = ptr_holder_.get(); // save for destroy outside lock
      obj_.swap(new_obj);
      if(active())
      {
        activate_object_();
      }
      else
      {
        ptr_holder_ = ObjectHolder_var();
      }
    }

    // destroy current_object_holder
    if(current_object_holder)
    {
      try
      {
        current_object_holder->usage_counter->deactivate_object();
        current_object_holder->usage_counter->wait_object();
      }
      catch(...)
      {}
    }

    return *this;
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  typename AccessActiveObject<SmartPtrType, ControlSmartPtrType>::Accessor
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::get_accessor() noexcept
  {
    ObjectHolder_var cur_holder = ptr_holder_.get();
    if(cur_holder.in() && cur_holder->usage_counter->add_active_count(1))
    {
      return GuardHolder(cur_holder);
    }

    return GuardHolder(null_object_holder_.in());
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  const ControlSmartPtrType&
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::get_object() noexcept
  {
    return obj_;
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  void
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::activate_object_()
    /*throw(Exception, eh::Exception)*/
  {
    // assert(!ptr_holder_.get().in());
    if(obj_)
    {
      ObjectHolder_var new_holder = new ObjectHolder(obj_);
      new_holder->usage_counter->activate_object();
      ptr_holder_ = new_holder;
    }
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  void
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::deactivate_object_()
    /*throw(Exception, eh::Exception)*/
  {
    ObjectHolder_var cur_holder = ptr_holder_.get();
    if(cur_holder.in())
    {
      cur_holder->usage_counter->deactivate_object();
    }
  }

  template<typename SmartPtrType, typename ControlSmartPtrType>
  void
  AccessActiveObject<SmartPtrType, ControlSmartPtrType>::wait_object_()
    /*throw(Exception, eh::Exception)*/
  {
    ObjectHolder_var cur_holder = ptr_holder_.get();
    if(cur_holder.in())
    {
      cur_holder->usage_counter->wait_object();
    }

    ptr_holder_ = ObjectHolder_var();
  }
}
}

#endif /*ACCESSACTIVEOBJECT_HPP_*/
