#include "ZmqStreamStats.hpp"

namespace AdServer
{
  const Generics::StringHashAdapter
  ZmqStreamStats::SENT_MESSAGES_COUNT_("sentMessagesCount");

  const Generics::StringHashAdapter
  ZmqStreamStats::SENT_MESSAGES_SIZE_("sentMessagesSize");

  const Generics::StringHashAdapter
  ZmqStreamStats::DROPPED_MESSAGES_COUNT_("droppedMessagesCount");

  const Generics::StringHashAdapter
  ZmqStreamStats::RECEIVED_MESSAGES_COUNT_("recvMessagesCount");

  const Generics::StringHashAdapter
  ZmqStreamStats::RECEIVED_MESSAGES_SIZE_("recvMessagesSize");


  ZmqStreamStats::ZmqStreamStats() noexcept
    : dropped_messages_count_(0),
      sent_messages_count_(0),
      sent_messages_size_(0),
      received_messages_count_(0),
      received_messages_size_(0)
  {}

  void
  ZmqStreamStats::add_dropped_messages(unsigned long dropped_count)
    noexcept
  {
    dropped_messages_count_ += dropped_count;
  }

  void
  ZmqStreamStats::add_sent_messages(
    unsigned long sent_count,
    unsigned long sent_size)
    noexcept
  {
    sent_messages_count_ += sent_count;
    sent_messages_size_ += sent_size;
  }

  void
  ZmqStreamStats::add_received_messages(
    unsigned long received_count,
    unsigned long received_size)
    noexcept
  {
    received_messages_count_ += received_count;
    received_messages_size_ += received_size;
  }

  unsigned long
  ZmqStreamStats::dropped_messages() const noexcept
  {
    return dropped_messages_count_;
  }

  unsigned long
  ZmqStreamStats::sent_messages() const noexcept
  {
    return sent_messages_count_;
  }

  unsigned long
  ZmqStreamStats::sent_messages_size() const noexcept
  {
    return sent_messages_size_;
  }

  unsigned long
  ZmqStreamStats::received_messages() const noexcept
  {
    return received_messages_count_;
  }

  unsigned long
  ZmqStreamStats::received_messages_size() const noexcept
  {
    return received_messages_size_;
  }
}
