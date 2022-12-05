// @file PlainStorage/FragmentLayer.hpp

#ifndef _PLAINSTORAGE_WRITEFRAGMENTLAYER_HPP_
#define _PLAINSTORAGE_WRITEFRAGMENTLAYER_HPP_

#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <iostream>

#include <Stream/MemoryStream.hpp>
#include <Commons/LockMap.hpp>

#include "BaseLayer.hpp"
#include "FileLayer.hpp"
#include "TransientPropertyLayerImpl.hpp"

//#define DEBUG_ALLOC_CONFLICT_CHECK_

namespace PlainStorage
{
  /// Used as a criterion for the index indicates a fragment
  const u_int32_t SUB_INDEX_RESERVED = ~static_cast<u_int32_t>(0);

  /**
   * BlockExIndex - helper for adding sub index to some index struct
   */
  template<typename IndexType>
  struct BlockExIndex
  {
    typedef IndexType BaseIndex;
    typedef u_int32_t SubIndex;

    BlockExIndex()
      : base_index(0), sub_index(SUB_INDEX_RESERVED)
    {}

    BlockExIndex(
      IndexType base_index_val,
      SubIndex sub_index_val = SUB_INDEX_RESERVED)
      : base_index(base_index_val),
        sub_index(sub_index_val)
    {}

    bool has_sub_index() const
    {
      return sub_index != SUB_INDEX_RESERVED;
    }

    bool operator==(const BlockExIndex<IndexType>& right) const
    {
      return base_index == right.base_index && sub_index == right.sub_index;
    }

    bool operator!=(const BlockExIndex<IndexType>& right) const
    {
      return !(*this == right);
    }

    bool operator<(const BlockExIndex<IndexType>& right) const
    {
      return base_index < right.base_index ||
        (base_index == right.base_index && sub_index < right.sub_index);
    }

    unsigned long
    hash() const
    {
      return (base_index << 8) ^ sub_index;
    }

    /// Serial number of the ordinary data block
    IndexType base_index;
    /// Serial number of fragment in the body of the block
    SubIndex sub_index;
  };

  template<typename IndexType>
  std::ostream&
  operator<<(std::ostream& ostr, const BlockExIndex<IndexType>& right)
  {
    ostr << right.base_index << ", ";
    if(right.sub_index == SUB_INDEX_RESERVED)
    {
      ostr << "none";
    }
    else
    {
      ostr << right.sub_index;
    }
    return ostr;
  }

  /**
   * Extended index = pair index of block and index of fragment
   * Extended index serializer to/from memory buffers
   */
  template<typename ExIndexType,
           typename SubIndexSerializerType =
             PlainSerializer<typename ExIndexType::BaseIndex> >
  struct ExIndexSerializer
  {
    /**
     * @return sum of index size and maximum subindex size
     */
    static SizeType max_size() noexcept
    {
      return
        sizeof(typename ExIndexType::SubIndex) +
        SubIndexSerializerType::max_size();
    }

    /**
     * @param val The object of extended index to evaluate size of index
     * @return sum of index size and subindex size
     */
    static SizeType size(const ExIndexType& val) noexcept
    {
      return
        sizeof(typename ExIndexType::SubIndex) +
        SubIndexSerializerType::size(val.sub_index);
    };

    /**
     * Put index value in memory buffer (serialize)
     * @param val The extended index value
     * @param buf The pointer to allocated storage
     * @param sz The size of buffer, must be greater than index size
     */
    void
    save(const ExIndexType& val, void* buf, SizeType sz) const
      /*throw(CorruptedRecord)*/
    {
      static const char* FUN = "ExIndexSerializer::save()";

      if (sz < size(val))
      {
        Stream::Error ostr;
        ostr << FUN << ": Try save ExIndex size=" << size(val) <<
          ", into buffer size=" << sz;
        throw CorruptedRecord(ostr);
      }

      *(typename ExIndexType::SubIndex*)(char*)buf = val.sub_index;
      SizeType base_index_size = sub_index_serializer_.size(val.base_index);
      sub_index_serializer_.save(
        val.base_index,
        (typename ExIndexType::SubIndex*)buf + 1,
        base_index_size);
    }

    /**
     * Restore index value from memory buffer (deserialize)
     * @param buf The pointer to allocated storage with saved index
     * @param sz The size of buffer
     * @param val The extended index value
     * @return The size of restored index
     */
    SizeType load(const void* buf, SizeType sz, ExIndexType& val) const
      /*throw(BaseException)*/
    {
      static const char* FUN = "ExIndexSerializer::load()";

      try
      {
        val.sub_index = *(typename ExIndexType::SubIndex*)buf;
        return
          sizeof(typename ExIndexType::SubIndex) +
          sub_index_serializer_.load(
            (const typename ExIndexType::SubIndex*)buf + 1,
            sz - sizeof(typename ExIndexType::SubIndex),
            val.base_index);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception: " << ex.what();
        throw BaseException(ostr);
      }
    }

  protected:
    /// The serializer able to work with subindex part of ExIndexType
    SubIndexSerializerType sub_index_serializer_;
  };

  /**
   * WriteFragmentLayer produce Fragmented data blocks and manage it
   */
  template<
    typename NextIndexType,
    typename NextIndexSerializerType = PlainSerializer<NextIndexType>,
    typename SyncPolicyType = Sync::Policy::PosixThreadRW>
  class WriteFragmentLayer:
    public virtual BlockSizeAllocator<BlockExIndex<NextIndexType> >,
    public virtual TransientPropertyWriteLayerImpl<
      BlockExIndex<NextIndexType>, NextIndexType>
  {
  public:
    typedef NextIndexSerializerType NextIndexSerializer;
    typedef BlockExIndex<NextIndexType> BlockIndex;
    typedef
      ExIndexSerializer<BlockIndex, NextIndexSerializerType>
      IndexSerializer;

    typedef NextIndexType NextBlockIndex;
    typedef
      typename BlockSizeAllocator<
        BlockExIndex<NextIndexType> >::AllocatedBlock
      AllocatedBlock;

    /**
     * Constructor init members,
     * Get property "RecordAllocatorType" from next_layer,
     * get main control block index from Main Control Block property
     * and load main_control_block_ through its index
     * @param next_layer The pointer to lower layer in hierarhy
     * @param allocator The allocator layer pointer, should provide
     * ordinary data blocks (fragments allocation implement in layer)
     */
    WriteFragmentLayer(
      WriteBaseLayer<NextBlockIndex>* next_layer,
      BlockAllocator<NextBlockIndex>* allocator)
      /*throw(BaseException)*/;

    // SequenceAllocator interface

    /**
     * @return Available size of Main Control Block
     */
    virtual SizeType
    max_block_size() const noexcept;

    /**
     * Allocate space for user data.
     * Allocator knows the maximum size of a piece of memory for user data.
     * If you are requested less than half the maximum size is allocated a
     * fragment of the block, otherwise allocated a new block.
     * @param size The memory size that should be provided for user, must be
     *   less than max_block_size()
     * @param allocated_blocks Get allocation result
     */
    virtual void
    allocate_size(
      SizeType size, AllocatedBlock& allocated_blocks) /*throw(BaseException)*/;

    // WriteLayer interface

    virtual
    ReadBlock_var
    get_read_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Construct fragment or data block by index
     */
    virtual
    WriteBlock_var
    get_write_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual
    unsigned long
    area_size() const noexcept;

    /**
     * Method need for debug aims
     */
    void print_control_block_(std::ostream& out) noexcept;

  protected:
    /**
     * Object store references to next_index and Data block of Control block.
     * Also synchronization primitive, used in the map:
     * fragment_size -> control_block, through smart pointer
     */
    struct ControlBlock
    {
      ControlBlock() {};

      ControlBlock(const NextIndexType& ind, WriteBlock* write_block)
        : index(ind),
          block(ReferenceCounting::add_ref(write_block))
      {}

      NextIndexType index;
      WriteBlock_var block;
    };

    class ReadFragmentImpl;
    friend class ReadFragmentImpl;

    class WriteFragmentImpl;
    friend class WriteFragmentImpl;

    typedef std::list<ControlBlock> ControlBlockList;

    struct AllocatePool: public ReferenceCounting::AtomicImpl
    {
      AllocatePool(WriteBaseLayer<NextBlockIndex>* next_layer)
        noexcept;

      ControlBlock
      get() /*throw(BaseException)*/;

      void
      release(const ControlBlock& control_block)
        noexcept;

      void
      add_free(const NextIndexType& index, WriteBlock* write_block)
        /*throw(BaseException)*/;

      const ControlBlockList&
      control_blocks_i() const noexcept
      {
        return control_blocks_;
      }

      const ControlBlock&
      next_control_block_i() const noexcept
      {
        return next_control_block_;
      }

    protected:
      virtual ~AllocatePool() noexcept {}

    private:
      typedef Sync::Policy::PosixThread ControlBlockSyncPolicy;
      typedef Sync::Policy::PosixThread NextControlBlockSyncPolicy;

      ReferenceCounting::SmartPtr<WriteBaseLayer<NextBlockIndex> > next_layer_;

      typename ControlBlockSyncPolicy::Mutex control_blocks_lock_;
      ControlBlockList control_blocks_;

      typename NextControlBlockSyncPolicy::Mutex next_control_block_lock_;
      ControlBlock next_control_block_;
    };

    typedef ReferenceCounting::SmartPtr<AllocatePool> AllocatePool_var;

    typedef std::vector<AllocatePool_var> AllocatePoolArray;

    typedef AdServer::Commons::StrictLockMap<
      Generics::NumericHashAdapter<NextBlockIndex>,
      Sync::Policy::PosixThread,
      AdServer::Commons::Hash2Args>
      FragmentAllocateLockMap;

  protected:
    /**
     * Empty virtual destructor
     */
    virtual ~WriteFragmentLayer() noexcept;

    // top level util methods

    /**
     * Selects the smallest suitable size of fragment, that can accommodate
     * size_rest data volume.
     * @param size_rest The volume of required memory size
     * @param safe_block_size Usually equal max_block_size() = available size in
     * main control block, initial seed value for the size of the fragment
     * @return fits fragment size, positive number - power of two.
     */
    unsigned int
    eval_fragment_size_(
      SizeType& fragment_size,
      SizeType size_rest,
      SizeType safe_block_size)
      /*throw(BaseException)*/;

    /**
     * @param fragment_size The size of fragment to be allocated
     * @param result_block The block with allocated space
     */
    void
    allocate_fragment_(
      unsigned int fragment_size_index,
      SizeType fragment_size,
      AllocatedBlock& result_block)
      /*throw(BaseException)*/;

    /**
     * Make choice what we should deallocate fragment or ordinary data block
     * and perform appropriate deallocation
     */
    void
    deallocate_(WriteBlock* write_block, const BlockIndex& index)
      /*throw(BaseException)*/;

    // fragmentation util methods
    /**
     * Turns a block of Data in a Fragmented Block.
     * Calculates the number of fragments in the Data block and
     * increases the size of the block up, so to put the whole number
     * of fragments, header and busy markers. Write Fragmented Block Header,
     * Busy markers after it, and mark all fragments as free
     * @param write_block The data block to be fragmented block
     * @param fragment_size The size of fragment, in bytes,
     *   for the partitioning block
     */
    void
    init_fragmentation_i_(
      const NextIndexType& block_index,
      WriteBlock* write_block,
      SizeType fragment_size)
      /*throw(BaseException)*/;

    /**
     * Allocate fragment
     * The essence of the fragments allocator is the same as that of the block
     * allocator. There is a pointer to a free fragment, it comes back as
     * a free fragment. Only fragments are not linked to the list,
     * and we kept busy/free flag for each fragment separately. The free
     * block indicator change by brute force inspection of flags until
     * free fragment found.
     *
     * Algorithm: Get fragment size and count of fragments from control_block.
     * Check the integrity of the data structure, throwing BaseException if not.
     * Get first free fragment from header of FB and mark it as used.
     * Look by a sequential scan of busy flags from the header for
     * the first free fragment. If not fount index of first free fragment,
     * put into header special value NO_FREE_FRAGMENT and set last_fragment
     * to true.
     *
     * First free fragment index in FBH is minimal index of fragment in the
     * block, always! This fact allow search for free fragment from just busy
     * fragment, not from beginning of busy markers.
     *
     * @param control_block The fragmented block that should provide fragment
     * @param last_fragment The value sets to true if no more free fragments
     * available, false if block is able to provide more fragments
     */
    typename BlockIndex::SubIndex
    allocate_fragment_i_(
      ControlBlock& control_block, bool& last_fragment)
      /*throw(BaseException)*/;

    /**
     * Mark fragment as free. Control integrity of block, and handle 2 cases:
     * all fragments in block are busy and not fulfilled block. If block isn't
     * fulfilled - update first free fragment reference in FBH.
     * @param write_block The fragmented block to processing
     * @param index The fragment index
     */
    void
    deallocate_fragment_(
      WriteBlock* write_block, const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Allocate new control block with free fragments references.
     * CB is a fragmented block
     */
    ControlBlock
    alloc_control_block_i_(SizeType fragment_size)
      /*throw(BaseException)*/;

    // main control block helpers

    /**
     * Replace or insert new reference to fragment in Main Control Block,
     * all references are ordered and order is maintaining. Algorithm:
     * 1. Remember the current size of MCB
     * 2. Calculate the record size in the MCB:
     *      Key size = sizeof (index_size_value) + index body size
     *      Record size = sizeof (fragment_size_value) + Key size
     * 3. Allocate memory and MCB load into it.
     * 4. Take a pair of steps iterators at the beginning and
     *    end of the old MCB.
     * 5. Find a position where you can insert a record about fragment_size
     *    without violating the order
     * 6. Create a record = (fragment_size, index_size, index_body) based
     *    on block_index
     * 7. If not found fragment_size - save record to end of block, or insert
     *    record without violating the order.
     *    If found fragment_size record into MCB replace pointed record to
     *    new record
     * Main control block can be empty (size = 0)
     * @param fragment_size The size of fragment
     * @param block_index The new block index to be referenced in MCB
     * @param old_block_index The variable for replaced index by block_index
     * @return true if replaced some index (replaced index put in old_block_index),
     *   false if inserted new index or replaced empty
     */
    bool
    replace_control_block_ref_(
      SizeType fragment_size,
      const NextIndexType& block_index,
      NextIndexType* old_block_index)
      /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Make reference of fragment_size in MCB empty.
     * Perform about the same steps as in the replace_control_block_ref_,
     * steps 1 - 5. If you find a reference, make it empty - put zero to
     * index size field.
     */
    void
    remove_control_block_ref_(SizeType fragment_size)
      /*throw(BaseException)*/;

    void
    sync_control_block_()
      /*throw(BaseException)*/;

  protected:
    /// Serializer for index of ordinary data block, use for MCB
    /// index serialization
    NextIndexSerializer next_index_serializer_;

    /// File layer
    ReferenceCounting::SmartPtr<WriteBaseLayer<NextBlockIndex> > next_layer_;
    /// Ordinary data blocks allocator
    ReferenceCounting::SmartPtr<BlockAllocator<NextBlockIndex> > allocator_;

    AllocatePoolArray allocate_pools_;

    FragmentAllocateLockMap fragment_alloc_locks_;

    /**
     * main_control_block_ block that contains control blocks references
     */
    ControlBlock main_control_block_;

    typename SyncPolicyType::Mutex deallocate_fragment_lock_;

# ifdef DEBUG_ALLOC_CONFLICT_CHECK_

  private:
    typedef std::set<BlockIndex> AllocatedSet;

  private:
    typename SyncPolicyType::Mutex debug_alloc_control_lock_;
    AllocatedSet debug_allocated_indexes_;

# endif // DEBUG_ALLOC_CONFLICT_CHECK_
  };
}

#include "FragmentLayer.tpp"

#endif //_PLAINSTORAGE_WRITEFRAGMENTLAYER_HPP_
