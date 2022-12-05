#ifndef PLAINSTORAGE_DEFAULTSYNCINDEXSTRATEGY_HPP
#define PLAINSTORAGE_DEFAULTSYNCINDEXSTRATEGY_HPP

#include <Sync/SyncPolicy.hpp>
#include <ProfilingCommons/PlainStorage/BaseLayer.hpp>
#include <ProfilingCommons/PlainStorage/KeyBlockAdapter.hpp>

namespace PlainStorage
{
  /**
   * DefaultReadIndexAccessor
   *   delegate load call to key object
   */
  template<typename KeyType>
  class DefaultReadIndexAccessor
  {
  public:
    void load(const void* buf, unsigned int size, KeyType& out)
      /*throw(eh::Exception)*/
    {
      out.load(buf, size);
    }
  };

  /**
   * DefaultWriteIndexAccessor
   *   delegate save & load calls to key object
   */
  template<typename KeyType>
  class DefaultWriteIndexAccessor: public DefaultReadIndexAccessor<KeyType>
  {
  public:
    unsigned long size(const KeyType& in)
      /*throw(eh::Exception)*/
    {
      return in.size();
    }

    void save(const KeyType& in, void* buf, unsigned int size)
      /*throw(eh::Exception)*/
    {
      in.save(buf, size);
    }
  };

  /**
   * DefaultWriteBlockIndexAccessor
   *   delegate save & load calls to block index object
   */
  template<typename BlockIndexType>
  class DefaultWriteBlockIndexAccessor
  {
  public:
    static unsigned long max_size()
    {
      return BlockIndexType::max_size();
    }

    unsigned long size(const BlockIndexType& in)
      /*throw(eh::Exception)*/
    {
      return in.size();
    }

    void save(const BlockIndexType& in, void* buf, unsigned int size)
      /*throw(eh::Exception)*/
    {
      in.save(buf, size);
    }

    unsigned long load(const void* buf, unsigned int size, BlockIndexType& in)
    {
      return in.load(buf, size);
    }
  };

  /**
   * DefaultSyncIndexStrategy
   * Intended to loading mapping Key ---> Block, stored in the sequence of
   * KeyBlocks in a file, to memory, to the faster container.
   * As result, for example, we can load whole sequence in std::map
   */
  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexSerializerType>
  class DefaultSyncIndexStrategy
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    /**
     * Additional information allow locate Key in KeyBlocks sequence
     * To do this, need 2 coordinates, a reference to the KeyBlock and
     * the offset in the KeyBlock.
     */
    struct KeyAddition
    {
      KeyAddition() {};

      /**
       * @param bi_val The index of block
       * @param bo_val The offset in block
       */
      KeyAddition(const BlockIndexType& bi_val, SizeType bo_val)
        : block_index(bi_val), block_offset(bo_val)
      {}

      void print(std::ostream& out) const noexcept
      {
        out << "block_index = (" << block_index << "), offset = " << block_offset;
      }
      
      BlockIndexType block_index;
      SizeType block_offset;
    };

    /**
     * Callback interface
     */
    class IndexLoadCallback
    {
    public:
      /**
       * virtual empty destructor
       */
      virtual ~IndexLoadCallback(){}

      virtual void load(
        const KeyType& key,
        const KeyAddition& key_add,
        const BlockIndexType& block_ref)
        /*throw(eh::Exception)*/ = 0;
    };

  public:
    /**
     * Constructor load Main Block if it saved in property
     * "DefaultSyncIndexStrategy::KeyBlocksIndex", otherwise create Main Block
     * and save property
     */
    DefaultSyncIndexStrategy(
      WriteBaseLayer<BlockIndexType>* write_layer,
      BlockAllocator<BlockIndexType>* block_allocator)
      /*throw(eh::Exception)*/;

    /**
     * Loading keys from KeyBlocks sequence determined by main block
     * Look through KeyBlocks and go over each valid key, call callback
     * for each found key.
     * @param index_load_callback The interface for the transfer of selected
     * pairs (Key -> Block) with additional coordinates of Key in KeyBlocks
     * sequence
     */
    void
    load(IndexLoadCallback* index_load_callback)
      /*throw(eh::Exception)*/;

    /**
     * Put pair Key -> Block to file (to KeyBlock sequence)
     * @param key The key value for some data block
     * @param data_block_index The index of data block is assigned a key
     * @return Coordinates of stored key in KeyBlock sequence
     */
    KeyAddition
    insert(
      const KeyType& key,
      const BlockIndexType& data_block_index)
      /*throw(eh::Exception, CorruptedRecord)*/;

    /**
     * Mark Key as deleted. Locate key in KeyBlock sequence and mark.
     * @param key The key value to be deleted
     * @param key_addition The information need to locate key
     *   in KeyBlocks sequence (index + offset)
     * @param block_ref Not used, formal
     */
    void erase(
      const KeyType& key,
      const KeyAddition& key_addition,
      const BlockIndexType& block_ref) /*throw(eh::Exception)*/;

    /**
     * call close_()
     */
    void close() /*throw(eh::Exception)*/;

  protected:
    /**
     * attach current block to main block
     */
    void close_() /*throw(eh::Exception)*/;

  protected:
    typedef KeyBlockAdapter<
      KeyType, KeyAccessorType, BlockIndexType, BlockIndexSerializerType>
      KeyBlock;

    typedef Sync::Policy::PosixThread SyncPolicy;

  protected:
    mutable SyncPolicy::Mutex lock_;

    ReferenceCounting::SmartPtr<WriteBaseLayer<BlockIndexType> > write_layer_;
    ReferenceCounting::SmartPtr<BlockAllocator<BlockIndexType> > block_allocator_;

    /// The starting point, the beginning of the sequence of KeyBlocks
    /// Must be empty, (key -> block) pairs stored in posterior KeyBlocks
    WriteBlock_var main_block_;
    bool need_close_;
    
    /// Current KeyBlock in KeyBlock sequence
    WriteBlock_var cur_block_;
    BlockIndexType cur_block_index_;
  };
}

#include "DefaultSyncIndexStrategy.tpp"

#endif /*PLAINSTORAGE_DEFAULTSYNCINDEXSTRATEGY_HPP*/
