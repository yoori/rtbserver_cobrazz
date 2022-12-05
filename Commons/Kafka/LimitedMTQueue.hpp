/* $Id: LimitedMTQueue.hpp 185978 2020-07-04 00:12:49Z jurij_kuznecov $
* @file LimitedMTQueue.hpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Limited thread safe queue
*/

#pragma once

#include <queue>
#include <Generics/Time.hpp>
#include <Commons/DelegateActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <Sync/PosixLock.hpp>
#include <Sync/Condition.hpp>
#include <Sync/SyncPolicy.hpp>

namespace AdServer
{
  namespace Commons
  {
    /**
     * @class MTQueue
     * @brief Thread-safe queue.
     */
    template<class T>
    class LimitedMTQueue
    {
    public:
      DECLARE_EXCEPTION(AlreadyClosed, eh::DescriptiveException);      
   
    public:
      /**
       * @brief Constructor
       *
       * @param max queue size (0 means unlimited).
       */
      explicit
      LimitedMTQueue(size_t max_size = 0);
      
      /**
       * @brief Destructor
       */
      ~LimitedMTQueue();

      /**
       * @brief Try emplace an element
       *
       * @param passing args as the arguments
                for the element constructor.
       * @return false if size() > maxSize
       */
      template <class... Args>
      bool
      try_emplace(Args&&... args);
     
      /**
       * @brief Try push new element
       *
       * @param element
       * @return false if size() > maxSize
       */
      bool
      try_push(const T& val);

      /**
       * @brief Emplace an element
       *
       * @param passing args as the arguments
                for the element constructor.
       * @return false if size() > maxSize
       */
      template <class... Args>
      void
      emplace(Args&&... args);

      /**
       * @brief Push new element
       * Blocking if size() > maxSize
       *
       * @param element
       */
      void
      push(const T& val); 
      
      /**
       * @brief Get an element from the top of the queue
       *
       * @param element
       */
      void
      peek(T& val) const;

      /**
       * @brief Wait & pop an element from the queue
       *
       * @param element
       */
      void
      pop(T& val);

      /**
       * @brief Remove an element from the queue
       */
      void
      pop();

      /**
       * @brief Remove all elements from the queue,
       * if queue empty - wait when any element will appear
       */
      void
      pop_all(std::list<T>& vals, const Generics::Time& timeout);

      /**
       * @brief Wait & pop an element from the queue (with timeout)
       *
       * @param element
       * @param timeout seconds
       * @param timeout nano seconds
       */
      bool
      pop(T& val, const Generics::Time& timeout);
      
      /**
       * @brief Get queue size
       *
       * @return queue size
       */
      size_t
      size() const;

      /**
       * @brief Wait empty queue
       * Wait (block) until queue become empty
       */
      void
      wait_empty();

      /**
       * @brief close queue
       * Stop all waits and exit
       */
      void
      close();

    protected:
      typedef Sync::Policy::PosixThread SyncPolicy;
      
    private:
      /**
       * @brief Check queue is inactive
       * Throw an exception in case queue is inactive
       */
      void
      check_close() /*throw(AlreadyClosed)*/;

    protected:
      const size_t max_size_;

      mutable SyncPolicy::Mutex lock_;
      mutable Sync::Conditional available_;
      Sync::Conditional removed_;

      std::list<T> queue_;
      size_t queue_size_;
      volatile sig_atomic_t closed_;
    };
  }
}

#include "LimitedMTQueue.ipp"
