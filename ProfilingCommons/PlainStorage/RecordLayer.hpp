/**
 * @file PlainStorage/RecordLayer.hpp
 * Layer for fixed size block sequencing to unlimited size record
 * do fragmentation control
 */
#ifndef _PLAINSTORAGE_RECORDLAYER_HPP_
#define _PLAINSTORAGE_RECORDLAYER_HPP_

#include <Generics/GnuHashTable.hpp>

#include "BaseLayer.hpp"

namespace PlainStorage
{
  /**
   * All synchronization works at a lower level than the Record
   * However, you can change this by strategy substitution
   */
  struct NullSyncPolicy
  {
    struct Mutex
    {
      void lock_read() noexcept {};
      void lock_write() noexcept {};
      void unlock() noexcept {};
    };

    struct ReadGuard
    {
      ReadGuard(const Mutex&) {}
    };

    struct WriteGuard
    {
      WriteGuard(const Mutex&) {}
    };
  };

  /**
   * ReadRecordLayer
   */
  template<typename NextIndexType,
           typename NextIndexSerializerType = PlainSerializer<NextIndexType>,
           typename SyncPolicyType = PlainStorage::NullSyncPolicy>
  class ReadRecordLayer:
    public virtual ReadBaseLayer<NextIndexType>,
    public virtual TransientPropertyReadLayerImpl<NextIndexType, NextIndexType>
  {
  public:
    typedef NextIndexType NextBlockIndex;
    typedef NextIndexType BlockIndex;
    typedef NextIndexSerializerType NextIndexSerializer;
    typedef NextIndexSerializerType IndexSerializer;

    DECLARE_EXCEPTION(Exception, BaseException);

    /**
     * Constructor do nothing, simply store next_layer pointer
     */
    ReadRecordLayer(
      ReadBaseLayer<NextIndexType>* next_layer) /*throw(Exception)*/;

    /**
     * Empty virtual destructor
     */
    virtual ~ReadRecordLayer() noexcept;

    /**
     * @param index The index of block that we need
     * @return The smart pointer to block that able work with Record
     */
    virtual
    ReadBlock_var
    get_read_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual
    unsigned long
    area_size() const noexcept;

  protected:
    class ReadRecordImpl;
    friend class ReadRecordImpl;
  };

  /**
   * WriteRecordLayer
   */
  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType = NullSyncPolicy>
  class WriteRecordLayer:
    public virtual TransientPropertyWriteLayerImpl<NextIndexType, NextIndexType>,
    public virtual WriteAllocateBaseLayer<NextIndexType>,
    public virtual ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>
  {
  public:
    typedef NextIndexType NextBlockIndex;
    typedef NextIndexType BlockIndex;
    typedef typename ReadRecordLayer<NextIndexType,
      NextIndexSerializerType, SyncPolicyType>::Exception
      Exception;
    typedef
      typename BlockAllocator<NextIndexType>::AllocatedBlock
      AllocatedBlock;

    /**
     * Constructor do nothing, simply store next_layer pointer and allocator
     * Layer factory create WriteRecordLayer on the basis of the fragment_layer
     */
    WriteRecordLayer(
      WriteBaseLayer<NextIndexType>* next_layer,
      BlockSizeAllocator<NextIndexType>* allocator)
      /*throw(Exception)*/;

    /**
     * Empty virtual destructor
     */
    virtual ~WriteRecordLayer() noexcept;

    //  Allocator interface

    /**
     * Allocate block with empty Record: zero full size and null next index
     * @param alloc_result Result of allocation.
     */
    virtual void
    allocate(AllocatedBlock& alloc_result) /*throw(BaseException)*/;

    // Layer interface
    /**
     * Call (delegate to) get_write_block
     * @param index The index of block that we need
     * @return The block able to read data
     */
    virtual
    ReadBlock_var
    get_read_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Searches for an entry in the already opened records, if there are return it.
     * (Records are tuck in memory).
     * Otherwise, map Record in memory by index, second time check opened records
     * container, insert new opened Record if need.
     * @param index The index of block that we need
     * @return The block able to read/write Record
     */
    virtual
    WriteBlock_var
    get_write_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual
    unsigned long
    area_size() const noexcept;

    /**
     * Accessor to size_allocator_
     */
    BlockSizeAllocator<BlockIndex>* allocator() noexcept;

  protected:
    class WriteRecordImpl;
    friend class WriteRecordImpl;

    /**
     * Erase record from opened records container
     */
    void close_record_i_(const WriteRecordImpl* record) noexcept;

  protected:
    typedef Generics::GnuHashTable<BlockIndex, WriteRecordImpl*> OpenedMap;

    typedef AdServer::Commons::LockMap<BlockIndex> OpeningLockMap;

  protected:
    /// Allocator can be from fragment layer
    ReferenceCounting::SmartPtr<BlockSizeAllocator<BlockIndex> > size_allocator_;

    /// serialize one key opening
    OpeningLockMap open_locks_;

    /// serialize opened_records_ operations
    typename SyncPolicyType::Mutex opened_records_lock_;
    /// Stores open Records, not in a hurry to close
    OpenedMap opened_records_;
  };
}

#include "RecordLayer.tpp"

#endif //_PLAINSTORAGE_RECORDLAYER_HPP_
