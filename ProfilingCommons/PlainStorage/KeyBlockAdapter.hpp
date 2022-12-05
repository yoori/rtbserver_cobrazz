// @file ProfilingCommons/KeyBlockAdapter.hpp

#ifndef KEYBLOCKADAPTER_HPP
#define KEYBLOCKADAPTER_HPP

#include <eh/Exception.hpp>
#include "BaseLayer.hpp"

namespace PlainStorage
{
  namespace KeyBlockAdapter_
  {
    /// Marker value for used KeyRecords
    const u_int32_t MARK_VALID = 0;
    /// Marker value for don't used KeyRecords (deleted)
    const u_int32_t MARK_DELETED = 1;
    /// Minimum size of memory that allocated when memory requested,
    /// useful to avoid excess reallocations, minimize general count of
    /// allocations.
    const SizeType RESIZE_PORTION = 10 * 1024;
  }

  /**
   * Using the Write Block that stores pairs {Keys, BlockIndex}
   * We need a mapping the Key ---> the Block.
   * This class provides methods to work with the write_block which stores
   * such mapping
   */
  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  class KeyBlockAdapter
  {
  public:
    enum KeyBlockHeader
    {
      KBH_USEDSIZE = 0,   // Memory occupied by stored keys and header
      KBH_NEXTBLOCKINDEXSIZE,
      KBH_SIZE
    };

    enum KeyRecordHeader
    {
      KH_KEY_SIZE = 0,
      KH_BLOCKINDEXSIZE,
      KH_MARK,
      KH_SIZE
    };

    /**
     * Constructor stores the gave write_block in a smart pointer
     */
    KeyBlockAdapter(WriteBlock* write_block) noexcept;

    /**
     * init(): init block
     * Write KeyBlockHeader in write block
     * @param max_use_size Limitation for the block size (in bytes) that can
     * be used to store keys. Zero means no restrictions, that is used the all
     * available volume of the block
     */
    void
    init(SizeType max_use_size = 1000) /*throw(BaseException)*/;

    /**
     * size():
     * @return size of write block, with keys - pos after last key in block
     */
    SizeType
    size() const noexcept;

    /**
     * find(): Looking for a position (offset) key in the block
     * The keys are laid sequentially in write_block_,
     * iterate through the keys sequentially from the begin to the end
     * until it reaches the sample key. Check the key is removed or not
     * @param key The sample for search position
     * @return pos of key in block
     */
    SizeType
    find(const KeyType& key) const /*throw(BaseException)*/;

    /**
     * push_back(): add to end of block new key
     * @param key The key to be added to the end of the block
     * @param block_index The value of block index
     * @param allow_resize If true and the key do not fit in the block,
     * try increment size of storage at maximum from key size and
     * RESIZE_PORTION bytes
     * @return Used size of block or zero if cannot add key into block
     *   if key successfull added method return true
     *   if key can't be added (made max block size) it return false
     * @return Used size of block or zero if cannot add key into block
     */
    SizeType
    push_back(
      const KeyType& key, const BlockIndexType& block_index,
      bool allow_resize = false)
      /*throw(BaseException)*/;

    /**
     * rewrite(): rewrite block index for exist key
     * Write the index of block for key stored in write_block_ at position
     * @param pos The offset to the beginning of the key, wich will be
     * updated the index block
     * @param block_index The new value for block index
     */
    void
    rewrite(SizeType pos, const BlockIndexType& block_index)
      /*throw(BaseException)*/;

    /**
     * This method - "the iterator" allows you to move to the next key
     * @param pos The initial position from which to search
     * @param key Will contain resulting key if search successful
     * @param block_index Will contain resulting index of block if search successful
     * @param loop If true, iterate through keys until found valid or its exhausted,
     * false - make only one attempt to move to the next key
     * @return If found valid key (not marked as deleted) return its position
     * in write_block, if not found return zero.
     */
    SizeType
    get(
      SizeType pos,
      SizeType& key_pos,
      KeyType& key,
      BlockIndexType& block_index,
      bool loop = true) const
      /*throw(BaseException)*/;

    /**
     * erase(): erase from block key by its position, mark Key as deleted
     */
    void
    erase(SizeType pos) /*throw(BaseException)*/;

    /**
     * attach_next_block(): attach next block to this block,
     * update KeyBlockHeader.
     * Creates a list of KeyBlocks: KB -> KB -> ... -> KB
     */
    void
    attach_next_block(
      const BlockIndexType& next_block_index) /*throw(BaseException)*/;

    /**
     * next_block(): get next block index
     * @return false if next block isn't defined. Otherwise, the value of next
     * index returned in next_block_index
     */
    bool
    next_block(
      BlockIndexType& next_block_index) /*throw(BaseException)*/;

    void
    copy_keys(std::list<KeyType>& keys) const
      /*throw(BaseException)*/;

  private:
    /**
     * add_size_(): increment size of block
     * @param add_use_size The value in bytes which should increase
     * the size of the block
     * @return true if successfully allocated storage
     */
    bool
    add_size_(SizeType add_use_size) /*throw(BaseException)*/;

    /**
     * Read used size from Key Block header
     */
    SizeType
    used_size_() const /*throw(BaseException)*/;

    /**
     * Set used size value in Key Block header
     */
    void
    used_size_(SizeType val) /*throw(BaseException)*/;

  private:
    WriteBlock_var write_block_;
  };
}

#include "KeyBlockAdapter.tpp"

#endif // KEYBLOCKADAPTER_HPP
