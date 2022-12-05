#ifndef USERBINDSERVER_SIMPLEFIXEDALLOCATOR_HPP_
#define USERBINDSERVER_SIMPLEFIXEDALLOCATOR_HPP_

#include <memory>
#include <vector>
#include <list>

#include <Sync/SyncPolicy.hpp>
#include <Generics/Singleton.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  struct SimpleFixedAllocator
  {
  public:
    SimpleFixedAllocator(unsigned long alloc_size) noexcept;

    void*
    alloc() noexcept;

    void
    dealloc(void*) noexcept;

  protected:
    typedef std::vector<unsigned char> Buf;
    typedef std::list<Buf> BufList;

    typedef Sync::Policy::PosixSpinThread SyncPolicy;

  protected:
    void
    alloc_new_buf_(Buf& new_buf) noexcept;

  protected:
    const unsigned long alloc_size_;
    const unsigned long buf_size_;
    const unsigned long last_element_offset_in_buf_;

    mutable SyncPolicy::Mutex lock_;
    BufList buffers_;
    void* first_free_;
  };

  struct SimpleDistribAllocator
  {
  public:
    SimpleDistribAllocator(
      unsigned long min_alloc_size = sizeof(void*),
      unsigned long max_alloc_size = 257) noexcept;

    void*
    alloc(unsigned long size) noexcept;

    void
    dealloc(void* buf, unsigned long size) noexcept;

  protected:
    typedef std::vector<std::unique_ptr<SimpleFixedAllocator> > AllocArray;

  protected:
    const unsigned long min_alloc_size_;
    const unsigned long max_alloc_size_;
    AllocArray allocators_;
  };
}
}

#include "SimpleFixedAllocator.tpp"

#endif /*USERBINDSERVER_SIMPLEFIXEDALLOCATOR_HPP_*/
