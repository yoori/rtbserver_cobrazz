#ifndef COMMONS_CORBAOBJECT_HPP
#define COMMONS_CORBAOBJECT_HPP

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

namespace AdServer
{
namespace Commons
{
  template<typename ObjectType>
  class CorbaObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NullPointer, Exception);

    // not thread safe
    typedef TAO_Objref_Var_T<ObjectType> ObjectType_var;

  public:
    CorbaObject() noexcept;

    CorbaObject(
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      const CORBACommons::CorbaObjectRef& object_ref)
      noexcept;

    ObjectType* operator->() /*throw(NullPointer, Exception)*/;

    const CORBACommons::CorbaObjectRef&
    ref() const
      /*throw(NullPointer)*/;

    ObjectType_var
    get() const noexcept;

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;

    class Holder: public ReferenceCounting::AtomicImpl
    {
    public:
      Holder(
        const CORBACommons::CorbaClientAdapter* corba_client_adapter_val,
        const CORBACommons::CorbaObjectRef& object_ref_val)
        noexcept;

      const CORBACommons::CorbaClientAdapter_var corba_client_adapter;
      const CORBACommons::CorbaObjectRef object_ref;
      mutable SyncPolicy::Mutex lock;
      ObjectType_var object;

    protected:
      virtual ~Holder() noexcept {};
    };

    typedef ReferenceCounting::SmartPtr<Holder> Holder_var;

  private:
    Holder_var holder_;
  };
}
}

namespace AdServer
{
namespace Commons
{
  template<typename ObjectType>
  CorbaObject<ObjectType>::Holder::Holder(
    const CORBACommons::CorbaClientAdapter* corba_client_adapter_val,
    const CORBACommons::CorbaObjectRef& object_ref_val)
    noexcept
    : corba_client_adapter(ReferenceCounting::add_ref(corba_client_adapter_val)),
      object_ref(object_ref_val)
  {}

  template<typename ObjectType>
  CorbaObject<ObjectType>::CorbaObject()
    noexcept
  {}

  template<typename ObjectType>
  CorbaObject<ObjectType>::CorbaObject(
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    const CORBACommons::CorbaObjectRef& object_ref)
    noexcept
    : holder_(new Holder(corba_client_adapter, object_ref))
  {}

  template<typename ObjectType>
  ObjectType*
  CorbaObject<ObjectType>::operator->() /*throw(NullPointer, Exception)*/
  {
    Holder* holder = holder_;

    if(!holder)
    {
      throw NullPointer("");
    }

    SyncPolicy::WriteGuard lock(holder->lock);

    if(!holder->object.in())
    {
      try
      {
        holder->object =
          holder->corba_client_adapter->template resolve_object<ObjectType>(
            holder->object_ref);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "Can't resolve reference '" << holder->object_ref <<
          "', caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << "Can't resolve reference '" << holder->object_ref <<
          "', caught CORBA::SystemException: " << ex;
        throw Exception(ostr);
      }
    }

    return holder->object;
  }

  template<typename ObjectType>
  typename CorbaObject<ObjectType>::ObjectType_var
  CorbaObject<ObjectType>::get() const noexcept
  {
    Holder* holder = holder_;

    if(!holder)
    {
      return ObjectType_var();
    }

    SyncPolicy::ReadGuard lock(holder->lock);
    return holder->object;
  }

  template<typename ObjectType>
  const CORBACommons::CorbaObjectRef&
  CorbaObject<ObjectType>::ref() const
    /*throw(NullPointer)*/
  {
    const Holder* holder = holder_;

    if(!holder)
    {
      throw NullPointer("");
    }

    SyncPolicy::ReadGuard lock(holder->lock);
    return holder->object_ref;
  }
}
}

#endif /*COMMONS_CORBAOBJECT_HPP*/
