#ifndef COMMONS_DELEGATEACTIVEOBJECT_HPP_
#define COMMONS_DELEGATEACTIVEOBJECT_HPP_

#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace AdServer
{
  namespace Commons
  {
    class DelegateActiveObject :
      public Generics::ActiveObjectCommonImpl
    {
    public:
      template<typename Delegate>
      DelegateActiveObject(
        const Delegate& delegate,
        Generics::ActiveObjectCallback* callback,
        unsigned threads_number = 1,
        unsigned stack_size = 0)
        noexcept
        : Generics::ActiveObjectCommonImpl(
            SingleJob_var(new DelegateJob<Delegate>(delegate, callback)),
            threads_number,
            stack_size)
      {}

    private:
      template<typename Delegate>
      class DelegateJob : public SingleJob
      {
      public:
        DelegateJob(
          const Delegate& delegate,
          Generics::ActiveObjectCallback* callback)
          noexcept
          : SingleJob(callback),
            delegate_(delegate)
        {}

        virtual void
        work() noexcept
        {
          while (!this->is_terminating())
          {
            delegate_();
          }
        }

        virtual void
        terminate() noexcept
        {}

      private:
        Delegate delegate_;

      protected:
        virtual
        ~DelegateJob() noexcept
        {}
      };

      class SelfDelegateJob : public SingleJob
      {
      public:
        SelfDelegateJob(
          DelegateActiveObject& delegate_active_object,
          Generics::ActiveObjectCallback* callback)
          noexcept
          : SingleJob(callback),
            delegate_active_object_(delegate_active_object)
        {}

        virtual void
        work() noexcept
        {
          delegate_active_object_.work_();
        }

        virtual void
        terminate() noexcept
        {
          delegate_active_object_.terminate_();
        }

      private:
        DelegateActiveObject& delegate_active_object_;

      protected:
        virtual
        ~SelfDelegateJob() noexcept
        {}
      };

    protected:
      DelegateActiveObject(
        Generics::ActiveObjectCallback* callback,
        unsigned threads_number = 1,
        unsigned stack_size = 0)
        noexcept
        : Generics::ActiveObjectCommonImpl(
            SingleJob_var(new SelfDelegateJob(*this, callback)),
            threads_number,
            stack_size)
      {}

      virtual
      ~DelegateActiveObject() noexcept
      {}

      virtual void
      work_() noexcept
      {}

      virtual void
      terminate_() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<DelegateActiveObject>
      DelegateActiveObject_var;

    template<typename Delegate>
    DelegateActiveObject_var
    make_delegate_active_object(
      const Delegate& delegate,
      Generics::ActiveObjectCallback* callback,
      unsigned threads_number = 1)
      noexcept
    {
      return DelegateActiveObject_var(
        new DelegateActiveObject(
          delegate,
          callback,
          threads_number));
    }
  }
}

#endif /* COMMONS_DELEGATEACTIVEOBJECT_HPP_ */
