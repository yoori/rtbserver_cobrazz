#ifndef _COMMONS_CONTAINERS_HPP_
#define _COMMONS_CONTAINERS_HPP_

namespace AdServer
{
  namespace Commons
  {
    template<typename ObjectType>
    class Optional
    {
    public:
      Optional(): defined_(false), val_() {}

      Optional(const Optional<ObjectType>& val)
        : defined_(val.defined_),
          val_(val.val_)
      {}

      Optional(Optional<ObjectType>&& val)
        : defined_(val.defined_),
          val_(std::move(val.val_))
      {}

      template<typename...Args>
      explicit Optional(bool defined, Args... data)
        : defined_(defined),
          val_(std::forward<Args>(data)...)
      {}

      explicit Optional(const ObjectType& val)
        : defined_(true),
          val_(val)
      {}

      Optional(const ObjectType* val)
        : defined_(val),
          val_(val ? *val : ObjectType())
      {}

      const ObjectType*
      operator->() const noexcept
      {
        return &val_;
      }

      ObjectType*
      operator->() noexcept
      {
        return &val_;
      }

      const ObjectType&
      operator*() const noexcept
      {
        return val_;
      }

      ObjectType&
      operator*() noexcept
      {
        return val_;
      }

      bool
      present() const noexcept
      {
        return defined_;
      }

      bool
      has_value() const noexcept
      {
        return defined_;
      }

      void set(const ObjectType& val)
      {
        val_ = val;
        defined_ = true;
      }

      ObjectType&
      fill()
      {
        defined_ = true;
        return val_;
      }

      Optional&
      operator=(const ObjectType& val)
      {
        set(val);
        return *this;
      }

      Optional&
      operator=(const Optional<ObjectType>& val)
      {
        if(val.present())
        {
          set(*val);
        }
        else
        {
          clear();
        }

        return *this;
      }

      template<typename RightType>
      Optional&
      operator=(const Optional<RightType>& val)
      {
        if(val.present())
        {
          set(*val);
        }
        else
        {
          clear();
        }

        return *this;
      }

      /*
      template<typename RightType>
      Optional&
      operator=(const RightType& val)
      {
        set(val);
        return *this;
      }
      */

      template<typename RightType>
      bool
      operator==(const Optional<RightType>& right) const
      {
        return &right == this || (present() == right.present() &&
          (!present() || **this == *right));
      }

      template <typename RightType>
      bool
      operator !=(const Optional<RightType>& right) const
      {
        return !(*this == right);
      }

      void clear()
      {
        defined_ = false;
        val_ = ObjectType();
      }

    protected:
      template <typename CompatibleType>
      Optional(const CompatibleType& val, bool defined)
        : defined_(defined), val_(defined ? ObjectType(val) : ObjectType())
      {}

      void
      present_(bool new_state) noexcept
      {
        defined_ = new_state;
      }

    private:
      bool defined_;
      ObjectType val_;
    };

    template <typename ObjectType>
    class ValueStateHolder
    {
    public:
      typedef ObjectType ValueType;
      enum State
      {
        S_NOT_INITED,
        S_GOOD,
        S_FAIL
      };

      ValueStateHolder()
      : val_(), state_(S_NOT_INITED)
      {}

      template<typename...Args>
      explicit ValueStateHolder(State state, Args... data): val_(std::forward<Args>(data)...), state_(state) {}
      explicit ValueStateHolder(const ObjectType& val): val_(val), state_(S_GOOD) {}
      ValueStateHolder(const ObjectType* val): val_(val ? *val : ObjectType()), state_(val ? S_GOOD : S_NOT_INITED) {}

      const ObjectType*
      operator->() const noexcept
      {
        return &val_;
      }

      ObjectType*
      operator->() noexcept
      {
        return &val_;
      }

      const ObjectType&
      operator*() const noexcept
      {
        return val_;
      }

      ObjectType&
      operator*() noexcept
      {
        return val_;
      }

      bool
      present() const noexcept
      {
        return state_ == S_GOOD;
      }

      void set(const ObjectType& val)
      {
        val_ = val;
        state_ = S_GOOD;
      }

      ValueStateHolder&
      operator=(const ObjectType& val)
      {
        set(val);
        return *this;
      }

      ValueStateHolder&
      operator=(const ValueStateHolder<ObjectType>& val)
      {
        if(val.present())
        {
          set(*val);
        }
        else
        {
          clear();
        }

        return *this;
      }

      template<typename RightType>
      ValueStateHolder&
      operator=(const ValueStateHolder<RightType>& val)
      {
        if(val.present())
        {
          set(*val);
        }
        else
        {
          clear();
        }

        return *this;
      }

      template<typename RightType>
      ValueStateHolder&
      operator=(const RightType& val)
      {
        set(val);
        return *this;
      }

      template<typename RightType>
      bool
      operator==(const ValueStateHolder<RightType>& right) const
      {
        return &right == this || (present() == right.present() &&
          (!present() || **this == *right));
      }

      template <typename RightType>
      bool
      operator !=(const ValueStateHolder<RightType>& right) const
      {
        return !(*this == right);
      }

      void clear()
      {
        state_ = S_NOT_INITED;
        val_ = ObjectType();
      }

      void set_state(State s) noexcept
      {
        state_ = s;
      }

      State state() const noexcept
      {
        return state_;
      }

      bool fail() const noexcept
      {
        return state_ == S_FAIL;
      }

      bool good() const noexcept
      {
        return state_ == S_GOOD;
      }

      bool empty() const noexcept
      {
        return state_ == S_NOT_INITED;
      }

    protected:
      template <typename CompatibleType>
      ValueStateHolder(const CompatibleType& val, State state)
        : val_(state == S_GOOD ? ObjectType(val) : ObjectType()), state_(state)
      {}

    protected:
      ValueType val_;
      State state_;
    };


  }
}

#endif /*_COMMONS_CONTAINERS_HPP_*/
