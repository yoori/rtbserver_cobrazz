// @file PlainStorage/DefaultAllocatorLayer.hpp

#ifndef PLAINSTORAGE_DEFAULTALLOCATOR_HPP
#define PLAINSTORAGE_DEFAULTALLOCATOR_HPP

#include <set>

#include "BaseLayer.hpp"
#include "FileLayer.hpp"

namespace PlainStorage
{
//  template<typename _NEXT_INDEX>
  /**
   * DefaultAllocatorLayer
   *   layer delegate get_.._block calls into next layer and
   *   provide additional BlockAllocator interface for
   *   allocate blocks by one
   */
  class DefaultAllocatorLayer:
    public virtual BlockAllocator<FileBlockIndex>,
    public virtual WriteBaseLayer<FileBlockIndex>
  {
  public:
    /**
     * Constructor gets the "AllocatorType" property
     * If property don't exist in file (e.g. new file), initialize
     * this property with value="Default", as the control block take
     * with the maximum possible index and save this index value in
     * "DefaultAllocatorLayer.ControlBlockIndex" property.
     * Otherwise, take allocator name from "AllocatorType" property.
     * If the name of allocator differ from "Default" throw BaseException,
     * for "Default" named allocator, get
     * "DefaultAllocatorLayer.ControlBlockIndex" value and restore index
     * of the allocator control block from this value.
     */
    DefaultAllocatorLayer(ExWriteBaseLayer<FileBlockIndex>* next_layer)
      /*throw(BaseException)*/;

    /**
     * Allocate provide free block and "mark" it as used.
     * All blocks in a file, except Propeties block, linked to each other
     * in a chain (a list). The allocator store first_free_block index into
     * control_block_, and allocate method return block with free block index
     * and move control_block_ to next free block. If next reference in
     * control_block_ equal zero, that means the file exhausted and the allocator
     * must resize file to be able to provide the ALLOCATE_PORTION blocks, after
     * resizing all new blocks linked to each other and set control_block_ to
     * rest of sequence of free blocks.
     */
    virtual void
    allocate(AllocatedBlock& alloc_result) /*throw(BaseException)*/;

    /* WriteLayer interface */
    virtual
    ReadBlock_var
    get_read_block(const FileBlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual
    WriteBlock_var
    get_write_block(const FileBlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual
    bool
    get_property(const char* name, PropertyValue& property)
      /*throw(BaseException)*/;

    virtual
    void
    set_property(const char* name, const PropertyValue& property)
      /*throw(BaseException)*/;

    virtual
    unsigned long
    area_size() const noexcept;

    void print_(std::ostream& ostr) noexcept;

    void free_indexes_(std::set<FileBlockIndex>& indexes)
      /*throw(BaseException)*/;

  protected:
    virtual
    ~DefaultAllocatorLayer() noexcept;

    /**
     * Read next index from FREE block. Check and raise error if size of user
     * data in read_block not equal zero.
     * @param read_block The block to get link to next block
     * @return The index to next block
     */
    FileBlockIndex
    read_next_index_(ReadBlock* read_block)
      /*throw(BaseException)*/;

    /**
     * Link block to other, write next index in block.
     * @param write_block The block to renew next index value
     * @param next_index The value of block index which will be
     * connected this block
     */
    void write_next_index_(WriteBlock* write_block, FileBlockIndex next_index)
      /*throw(BaseException)*/;

    /**
     * Insert this block between control block
     * and first_free_block. Two stage: 1) link this block to current
     * first_free_block 2) link allocator control block to this block
     * @param write_block The block to be returned in free state
     * @param index The index of returned block to heap
     */
    void deallocate_(WriteBlock* write_block, FileBlockIndex index)
      /*throw(BaseException)*/;

    class WriteBlockImpl;
    friend class WriteBlockImpl;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

    typedef std::list<AllocatedBlock> AllocatedBlockList;

  protected:
    /// File layer reference
    ReferenceCounting::SmartPtr<ExWriteBaseLayer<FileBlockIndex> > next_layer_;

    /// This entity knows index of free block, in other word, ordinary data block
    /// (region of file) is containing pointer to free block. Zero means no free
    /// blocks in a file.
    WriteBlock_var control_block_;

    SyncPolicy::Mutex lock_;
    FileBlockIndex first_free_block_index_;
    AllocatedBlockList allocate_blocks_;
    unsigned long allocate_blocks_size_;
  };

  typedef
    ReferenceCounting::SmartPtr<DefaultAllocatorLayer>
    DefaultAllocatorLayer_var;
}

#endif // PLAINSTORAGE_DEFAULTALLOCATOR_HPP
