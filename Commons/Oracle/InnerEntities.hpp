#ifndef ORACLE_INNERENTITIES_HPP
#define ORACLE_INNERENTITIES_HPP

#include <assert.h>

#ifdef _USE_OCCI
#include <oci.h>
#endif

#include <iostream>

namespace AdServer
{
namespace Commons
{
  namespace Oracle
  {
    template<typename Type, unsigned long TypeId>
    class OCIHandlePtr
    {
    public:
      OCIHandlePtr();

      OCIHandlePtr(Type* handle);

      ~OCIHandlePtr();

      void reset(Type* handle, bool skip_oci_error = false);

      Type* release();

      Type* get();

      Type*& inout();

      Type*& fill();

    private:
      Type* handle_;
    };

    template<typename Type, unsigned long TypeId>
    class OCIDescriptorPtr
    {
    public:
      OCIDescriptorPtr();

      OCIDescriptorPtr(Type* handle);

      ~OCIDescriptorPtr();

      void reset(Type* handle);

      Type* release();

      Type* get();

      Type*& inout();

      Type*& fill();

    private:
      Type* handle_;
    };

    template<typename Type, bool PIN_FREE = false>
    class OCIObjectPtr
    {
    public:
      OCIObjectPtr(void* env_handle, void* error_handle, Type* obj = 0);

      ~OCIObjectPtr();

      void reset(Type* obj);

      Type* release();

      Type* get();

      Type*& inout();

      Type*& fill();

    private:
      void* env_handle_;
      void* error_handle_;
      Type* obj_;
    };
  }
}
}

namespace AdServer
{
namespace Commons
{
  namespace Oracle
  {
    // OCIHandlePtr
    template<typename Type, unsigned long TypeId>
    OCIHandlePtr<Type, TypeId>::OCIHandlePtr()
      : handle_(0)
    {}

    template<typename Type, unsigned long TypeId>
    OCIHandlePtr<Type, TypeId>::OCIHandlePtr(Type* handle)
      : handle_(handle)
    {}

    template<typename Type, unsigned long TypeId>
    OCIHandlePtr<Type, TypeId>::~OCIHandlePtr()
    {
      reset(0);
    }

    template<typename Type, unsigned long TypeId>
    void
    OCIHandlePtr<Type, TypeId>::reset(Type* handle, bool skip_oci_error)
    {
      if(handle_)
      {
        #ifdef _USE_OCCI
          sword result;
          if((result = OCIHandleFree(
                handle_,
                TypeId)) != OCI_SUCCESS &&
             !(result == OCI_ERROR && skip_oci_error))
          {
            std::cerr << "Can't make OCIHandleFree, result = " <<
              result <<
              (result == OCI_INVALID_HANDLE ? "(OCI_INVALID_HANDLE)" : "") <<
              (result == OCI_ERROR ? "(OCI_ERROR)" : "") <<
              std::endl;

            assert(0);
          }
        #else
          assert(0);
        #endif
      }

      handle_ = handle;
    }
 
    template<typename Type, unsigned long TypeId>
    Type*
    OCIHandlePtr<Type, TypeId>::release()
    {
      Type* ret = handle_;
      handle_ = 0;
      return ret;
    }
          
    template<typename Type, unsigned long TypeId>
    Type*
    OCIHandlePtr<Type, TypeId>::get()
    {
      return handle_;
    }
          
    template<typename Type, unsigned long TypeId>
    Type*&
    OCIHandlePtr<Type, TypeId>::inout()
    {
      return handle_;
    }

    template<typename Type, unsigned long TypeId>
    Type*&
    OCIHandlePtr<Type, TypeId>::fill()
    {
      reset(0);
      return handle_;
    }

    // OCIDescriptorPtr
    template<typename Type, unsigned long TypeId>
    OCIDescriptorPtr<Type, TypeId>::OCIDescriptorPtr()
      : handle_(0)
    {}

    template<typename Type, unsigned long TypeId>
    OCIDescriptorPtr<Type, TypeId>::OCIDescriptorPtr(Type* handle)
      : handle_(handle)
    {}

    template<typename Type, unsigned long TypeId>
    OCIDescriptorPtr<Type, TypeId>::~OCIDescriptorPtr()
    {
      reset(0);
    }

    template<typename Type, unsigned long TypeId>
    void
    OCIDescriptorPtr<Type, TypeId>::reset(Type* handle)
    {
      if(handle_)
      {
      #ifdef _USE_OCCI
        sword result;
        if((result = OCIDescriptorFree(
              handle_,
              TypeId)) != OCI_SUCCESS)
        {
          std::cerr << "Can't make OCIDescriptorFree" << std::endl;
        }
      #else
        assert(0);
      #endif
      }

      handle_ = handle;
    }

    template<typename Type, unsigned long TypeId>
    Type*
    OCIDescriptorPtr<Type, TypeId>::release()
    {
      Type* ret = handle_;
      handle_ = 0;
      return ret;
    }

    template<typename Type, unsigned long TypeId>
    Type*
    OCIDescriptorPtr<Type, TypeId>::get()
    {
      return handle_;
    }

    template<typename Type, unsigned long TypeId>
    Type*&
    OCIDescriptorPtr<Type, TypeId>::inout()
    {
      return handle_;
    }

    template<typename Type, unsigned long TypeId>
    Type*&
    OCIDescriptorPtr<Type, TypeId>::fill()
    {
      reset(0);
      return handle_;
    }

    // OCIObjectPtr
    template<typename Type, bool PIN_FREE>
    OCIObjectPtr<Type, PIN_FREE>::OCIObjectPtr(
      void* env_handle,
      void* error_handle,
      Type* obj)
      : env_handle_(env_handle),
        error_handle_(error_handle),
        obj_(obj)
    {}

    template<typename Type, bool PIN_FREE>
    OCIObjectPtr<Type, PIN_FREE>::~OCIObjectPtr()
    {
      reset(0);
    }

    template<typename Type, bool PIN_FREE>
    void
    OCIObjectPtr<Type, PIN_FREE>::reset(Type* obj)
    {
      if(obj_)
      {
      #ifdef _USE_OCCI
        sword result;

        if(PIN_FREE)
        {
          if((result = OCIObjectUnpin(
            static_cast<OCIEnv*>(env_handle_),
            static_cast<OCIError*>(error_handle_),
            obj_)) != OCI_SUCCESS)
          {
            assert(0);
            std::cerr << "Can't make OCIObjectUnpin" << std::endl;
          }
        }
        else
        {              
          if((result = OCIObjectFree(
            static_cast<OCIEnv*>(env_handle_),
            static_cast<OCIError*>(error_handle_),
            obj_,
            OCI_OBJECTFREE_FORCE)) != OCI_SUCCESS)
          {
            std::cerr << "Can't make OCIObjectFree" << std::endl;
          }
        }

      #else
        assert(0);
      #endif
      }

      obj_ = obj;
    }

    template<typename Type, bool PIN_FREE>
    Type*
    OCIObjectPtr<Type, PIN_FREE>::release()
    {
      Type* ret = obj_;
      obj_ = 0;
      return ret;
    }

    template<typename Type, bool PIN_FREE>
    Type*
    OCIObjectPtr<Type, PIN_FREE>::get()
    {
      return obj_;
    }

    template<typename Type, bool PIN_FREE>
    Type*&
    OCIObjectPtr<Type, PIN_FREE>::inout()
    {
      return obj_;
    }

    template<typename Type, bool PIN_FREE>
    Type*&
    OCIObjectPtr<Type, PIN_FREE>::fill()
    {
      reset(0);
      return obj_;
    }
  }
}
}

#endif /*ORACLE_INNERENTITIES_HPP*/
