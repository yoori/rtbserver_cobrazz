#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <list>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace AdServer::Commons
{
  class MonoAllocatorArena final
  {
  public:
    static constexpr std::size_t DEFAULT_INITIAL_SIZE = 128 * sizeof(void*);

    explicit
    MonoAllocatorArena(
      std::size_t initial_size = DEFAULT_INITIAL_SIZE) noexcept;

    MonoAllocatorArena(
      void* buffer,
      std::size_t buffer_size,
      std::size_t next_buffer_size = 0) noexcept;

    ~MonoAllocatorArena() noexcept;

    MonoAllocatorArena(const MonoAllocatorArena&) = delete;
    MonoAllocatorArena& operator=(const MonoAllocatorArena&) = delete;
    MonoAllocatorArena(MonoAllocatorArena&&) = delete;
    MonoAllocatorArena& operator=(MonoAllocatorArena&&) = delete;

    void
    release() noexcept;

  private:
    template<typename>
    friend class MonoAllocator;

    [[nodiscard]]
    void*
    allocate_(std::size_t bytes, std::size_t alignment);

    void
    add_chunk_(std::size_t bytes, std::size_t alignment);

    static
    std::size_t
    normalize_buffer_size_(std::size_t size) noexcept;

    static
    unsigned char*
    buffer_end_(unsigned char* buffer, std::size_t size) noexcept;

    static
    std::size_t
    calc_next_buffer_size_(std::size_t size) noexcept;

    struct Chunk
    {
      Chunk* next;
      unsigned char* memory;
      std::size_t size;
      std::size_t alignment;
    };

    unsigned char* const initial_buffer_ = nullptr;
    const std::size_t initial_buffer_size_ = 0;
    const std::size_t initial_next_buffer_size_;
    unsigned char* current_ = nullptr;
    unsigned char* end_ = nullptr;
    std::size_t next_buffer_size_;
    Chunk* chunks_ = nullptr;
  };

  template<typename T>
  class MonoAllocator
  {
  public:
    template<typename>
    friend class MonoAllocator;

    using value_type = T;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::false_type;

    template<typename U>
    struct rebind
    {
      using other = MonoAllocator<U>;
    };

    MonoAllocator() noexcept = delete;

    MonoAllocator(MonoAllocatorArena& arena) noexcept;

    MonoAllocator(MonoAllocatorArena* arena) noexcept;

    template<typename U>
    MonoAllocator(const MonoAllocator<U>& init) noexcept;

    [[nodiscard]]
    T*
    allocate(std::size_t count);

    void
    deallocate(T*, std::size_t) noexcept;

    MonoAllocatorArena*
    arena() const noexcept;

  private:
    MonoAllocatorArena* arena_ = nullptr;
  };

  inline
  MonoAllocatorArena::MonoAllocatorArena(std::size_t initial_size) noexcept
    : initial_next_buffer_size_(normalize_buffer_size_(initial_size)),
      next_buffer_size_(initial_next_buffer_size_)
  {}

  inline
  MonoAllocatorArena::MonoAllocatorArena(
    void* buffer,
    std::size_t buffer_size,
    std::size_t next_buffer_size) noexcept
    : initial_buffer_(static_cast<unsigned char*>(buffer)),
      initial_buffer_size_(buffer_size),
      initial_next_buffer_size_(
        next_buffer_size ?
        normalize_buffer_size_(next_buffer_size) :
        calc_next_buffer_size_(buffer_size)),
      current_(initial_buffer_),
      end_(buffer_end_(initial_buffer_, initial_buffer_size_)),
      next_buffer_size_(initial_next_buffer_size_)
  {}

  inline
  MonoAllocatorArena::~MonoAllocatorArena() noexcept
  {
    release();
  }

  inline
  void
  MonoAllocatorArena::release() noexcept
  {
    while(chunks_)
    {
      Chunk* chunk = chunks_;
      chunks_ = chunk->next;

      ::operator delete(
        chunk->memory,
        std::align_val_t(chunk->alignment));
      delete chunk;
    }

    current_ = initial_buffer_;
    end_ = buffer_end_(initial_buffer_, initial_buffer_size_);
    next_buffer_size_ = initial_next_buffer_size_;
  }

  inline
  void*
  MonoAllocatorArena::allocate_(std::size_t bytes, std::size_t alignment)
  {
    if(bytes == 0)
    {
      bytes = 1;
    }

    void* aligned = nullptr;
    if(current_)
    {
      void* ptr = current_;
      std::size_t space = static_cast<std::size_t>(end_ - current_);
      aligned = std::align(alignment, bytes, ptr, space);
    }

    if(!aligned)
    {
      add_chunk_(bytes, alignment);

      void* ptr = current_;
      std::size_t space = static_cast<std::size_t>(end_ - current_);
      aligned = std::align(alignment, bytes, ptr, space);
    }

    if(!aligned)
    {
      throw std::bad_alloc();
    }

    current_ = static_cast<unsigned char*>(aligned) + bytes;
    return aligned;
  }

  inline
  void
  MonoAllocatorArena::add_chunk_(std::size_t bytes, std::size_t alignment)
  {
    const std::size_t chunk_size = std::max(bytes, next_buffer_size_);
    const std::size_t chunk_alignment =
      std::max(alignment, alignof(std::max_align_t));

    void* memory = ::operator new(
      chunk_size,
      std::align_val_t(chunk_alignment));

    try
    {
      chunks_ = new Chunk{
        chunks_,
        static_cast<unsigned char*>(memory),
        chunk_size,
        chunk_alignment};
    }
    catch(...)
    {
      ::operator delete(memory, std::align_val_t(chunk_alignment));
      throw;
    }

    current_ = chunks_->memory;
    end_ = chunks_->memory + chunks_->size;
    next_buffer_size_ = calc_next_buffer_size_(chunk_size);
  }

  inline
  std::size_t
  MonoAllocatorArena::normalize_buffer_size_(std::size_t size) noexcept
  {
    return size ? size : 1;
  }

  inline
  unsigned char*
  MonoAllocatorArena::buffer_end_(
    unsigned char* buffer,
    std::size_t size) noexcept
  {
    return buffer ? buffer + size : nullptr;
  }

  inline
  std::size_t
  MonoAllocatorArena::calc_next_buffer_size_(std::size_t size) noexcept
  {
    size = normalize_buffer_size_(size);

    const std::size_t increment = (size + 1) / 2;
    if(std::numeric_limits<std::size_t>::max() - size < increment)
    {
      return std::numeric_limits<std::size_t>::max();
    }

    return size + increment;
  }

  template<typename T>
  inline
  MonoAllocator<T>::MonoAllocator(MonoAllocatorArena& arena) noexcept
    : arena_(&arena)
  {}

  template<typename T>
  inline
  MonoAllocator<T>::MonoAllocator(MonoAllocatorArena* arena) noexcept
    : arena_(arena)
  {}

  template<typename T>
  template<typename U>
  inline
  MonoAllocator<T>::MonoAllocator(const MonoAllocator<U>& init) noexcept
    : arena_(init.arena_)
  {}

  template<typename T>
  inline
  T*
  MonoAllocator<T>::allocate(std::size_t count)
  {
    if(count > std::numeric_limits<std::size_t>::max() / sizeof(T))
    {
      throw std::bad_array_new_length();
    }

    if(!arena_)
    {
      throw std::bad_alloc();
    }

    return static_cast<T*>(
      arena_->allocate_(count * sizeof(T), alignof(T)));
  }

  template<typename T>
  inline
  void
  MonoAllocator<T>::deallocate(T*, std::size_t) noexcept
  {}

  template<typename T>
  inline
  MonoAllocatorArena*
  MonoAllocator<T>::arena() const noexcept
  {
    return arena_;
  }

  template<typename T>
  inline
  MonoAllocator<T>
  mono_allocator(MonoAllocatorArena* arena) noexcept
  {
    return MonoAllocator<T>(arena);
  }

  template<typename T>
  inline
  MonoAllocator<T>
  mono_allocator(MonoAllocatorArena& arena) noexcept
  {
    return MonoAllocator<T>(arena);
  }

  template<typename Left, typename Right>
  inline
  bool
  operator==(
    const MonoAllocator<Left>& left,
    const MonoAllocator<Right>& right) noexcept
  {
    return left.arena() == right.arena();
  }

  template<typename Left, typename Right>
  inline
  bool
  operator!=(
    const MonoAllocator<Left>& left,
    const MonoAllocator<Right>& right) noexcept
  {
    return !(left == right);
  }

  template<typename T>
  using MonoVector = std::vector<T, MonoAllocator<T>>;

  template<typename T>
  using MonoList = std::list<T, MonoAllocator<T>>;

  template<typename Key, typename Compare = std::less<Key>>
  using MonoSet = std::set<Key, Compare, MonoAllocator<Key>>;

  template<
    typename Key,
    typename Value,
    typename Compare = std::less<Key>>
  using MonoMap = std::map<
    Key,
    Value,
    Compare,
    MonoAllocator<std::pair<const Key, Value>>>;

  template<
    typename Key,
    typename Value,
    typename Compare = std::less<Key>>
  using MonoMultiMap = std::multimap<
    Key,
    Value,
    Compare,
    MonoAllocator<std::pair<const Key, Value>>>;

  template<
    typename Key,
    typename Hash = std::hash<Key>,
    typename Equal = std::equal_to<Key>>
  using MonoUnorderedSet = std::unordered_set<
    Key,
    Hash,
    Equal,
    MonoAllocator<Key>>;

  template<
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename Equal = std::equal_to<Key>>
  using MonoUnorderedMap = std::unordered_map<
    Key,
    Value,
    Hash,
    Equal,
    MonoAllocator<std::pair<const Key, Value>>>;

  using MonoString = std::basic_string<
    char,
    std::char_traits<char>,
    MonoAllocator<char>>;
} // namespace AdServer::Commons
