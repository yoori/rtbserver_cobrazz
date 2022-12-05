#ifndef ZMQBALANCER_ZMQSTREAMSTATS_HPP_
#define ZMQBALANCER_ZMQSTREAMSTATS_HPP_

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Commons/AtomicInt.hpp>

namespace AdServer
{
  class ZmqStreamStats: public ReferenceCounting::AtomicImpl
  {
  public:
    ZmqStreamStats() noexcept;

    void
    add_dropped_messages(unsigned long dropped_count)
      noexcept;

    void
    add_sent_messages(
      unsigned long sent_count,
      unsigned long sent_size)
      noexcept;

    void
    add_received_messages(
      unsigned long received_count,
      unsigned long received_size)
      noexcept;

    unsigned long
    dropped_messages() const noexcept;

    unsigned long
    sent_messages() const noexcept;

    unsigned long
    sent_messages_size() const noexcept;

    unsigned long
    received_messages() const noexcept;

    unsigned long
    received_messages_size() const noexcept;

    template <typename Functor>
    void
    enumerate_all(Functor& functor) const /*throw(eh::Exception)*/;

  protected:
    static const Generics::StringHashAdapter SENT_MESSAGES_COUNT_;
    static const Generics::StringHashAdapter SENT_MESSAGES_SIZE_;
    static const Generics::StringHashAdapter RECEIVED_MESSAGES_COUNT_;
    static const Generics::StringHashAdapter RECEIVED_MESSAGES_SIZE_;
    static const Generics::StringHashAdapter DROPPED_MESSAGES_COUNT_;

  protected:
    Algs::AtomicUInt dropped_messages_count_;
    Algs::AtomicUInt sent_messages_count_;
    Algs::AtomicUInt sent_messages_size_;
    Algs::AtomicUInt received_messages_count_;
    Algs::AtomicUInt received_messages_size_;
  };

  typedef ReferenceCounting::SmartPtr<ZmqStreamStats>
    ZmqStreamStats_var;
}

namespace AdServer
{
  template <typename Functor>
  void
  ZmqStreamStats::enumerate_all(Functor& functor) const
    /*throw(eh::Exception)*/
  {
    functor(5);
    functor(DROPPED_MESSAGES_COUNT_, static_cast<int>(dropped_messages_count_));
    functor(SENT_MESSAGES_COUNT_, static_cast<int>(sent_messages_count_));
    functor(SENT_MESSAGES_SIZE_, static_cast<int>(sent_messages_size_));
    functor(RECEIVED_MESSAGES_COUNT_, static_cast<int>(received_messages_count_));
    functor(RECEIVED_MESSAGES_SIZE_, static_cast<int>(received_messages_size_));
  }
}

#endif
