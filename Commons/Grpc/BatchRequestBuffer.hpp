#pragma once

#include <cstddef>
#include <new>
#include <vector>

#include <google/protobuf/arena.h>

#include <Commons/Grpc/Batch.pb.h>

namespace AdServer::Grpc::Detail
{
  class BatchRequestBuffer
  {
  public:
    explicit BatchRequestBuffer(std::size_t max_retained_bytes = 1024 * 1024) noexcept;

    adserver::grpc::BatchRequest& get();
    void clear() noexcept;

  private:
    void reset_() noexcept;

    const std::size_t max_retained_bytes_;
    google::protobuf::Arena arena_;
    adserver::grpc::BatchRequest* request_ = nullptr;
    std::vector<std::size_t> item_string_capacities_;
    std::size_t string_capacity_ = 0;
  };

  inline BatchRequestBuffer::BatchRequestBuffer(std::size_t max_retained_bytes) noexcept
    : max_retained_bytes_(max_retained_bytes)
  {}

  inline adserver::grpc::BatchRequest& BatchRequestBuffer::get()
  {
    if (!request_)
    {
      request_ = google::protobuf::Arena::CreateMessage<adserver::grpc::BatchRequest>(&arena_);
    }

    return *request_;
  }

  inline void BatchRequestBuffer::clear() noexcept
  {
    if (!request_)
    {
      return;
    }

    const auto arena_bytes = arena_.SpaceAllocated();
    if (arena_bytes > max_retained_bytes_)
    {
      reset_();
      return;
    }

    const auto item_count = static_cast<std::size_t>(request_->items_size());
    if (item_string_capacities_.size() < item_count)
    {
      try
      {
        item_string_capacities_.resize(item_count);
      }
      catch (const std::bad_alloc&)
      {
        reset_();
        return;
      }
    }

    // Clear preserves item order and string capacity, including inactive tail items.
    // Track capacities by position to avoid reflection in SpaceUsedLong on every write.
    for (std::size_t index = 0; index < item_count; ++index)
    {
      const auto& item = request_->items(static_cast<int>(index));
      auto& previous_capacity = item_string_capacities_[index];
      string_capacity_ -= previous_capacity;
      previous_capacity = item.full_method().capacity() + item.payload().capacity();
      string_capacity_ += previous_capacity;
    }

    const auto tracking_bytes = item_string_capacities_.capacity() * sizeof(std::size_t);
    const auto remaining_bytes = max_retained_bytes_ - arena_bytes;
    if (tracking_bytes > remaining_bytes || string_capacity_ > remaining_bytes - tracking_bytes)
    {
      reset_();
    }
    else
    {
      request_->Clear();
    }
  }

  inline void BatchRequestBuffer::reset_() noexcept
  {
    request_ = nullptr;
    arena_.Reset();
    std::vector<std::size_t>().swap(item_string_capacities_);
    string_capacity_ = 0;
  }
}
