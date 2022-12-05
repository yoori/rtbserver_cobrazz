/**
 * @file PlainStorage/FileLayer.hpp
 * FileLayer implementation is resolving of blocks with equal indexes,
 * return pointer to one ReadBlock in ReadFileLayer
 * (or WriteBlock in WriteFileLayer)
 */
#ifndef PLAINSTORAGE_FILELAYER_HPP
#define PLAINSTORAGE_FILELAYER_HPP

#include "File.hpp"

#include <functional>
#include <memory>

#include <Sync/SyncPolicy.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/Time.hpp>

#include <Commons/LockMap.hpp>
#include <Commons/AtomicInt.hpp>
#include "BaseLayer.hpp"

namespace PlainStorage
{
  typedef u_int64_t FileBlockIndex;
}

namespace Generics
{
  /**
   * Hash functor, needs because R/W FileLayers store
   * opened blocks indices in hash
   */
  template<>
  struct HashFunForHashAdapter<PlainStorage::FileBlockIndex>
  {
    /**
     * Hash functor
     * @param ind The file block index value argue of hash function
     * @return The value of hash function
     */
    size_t
    operator ()(const PlainStorage::FileBlockIndex& ind) const
    {
      return static_cast<size_t>((ind >> 4) ^ (ind & 0xFFFFFFFF));
    }
  };
}

namespace PlainStorage
{
  typedef u_int64_t FileOffset;


  /**
   * ReadFileLayer
   * File divided on some parts, precisely, class calculate size of parts
   * by next equation:
   * PageSize = ::getpagesize() * ceil(RequestedBlockSize / ::getpagesize())
   * This layer allow read file by some portions in several stages
   */
  class ReadFileLayer: public ReadBaseLayer<FileBlockIndex>
  {
  public:
    DECLARE_EXCEPTION(Exception, BaseException);

    /**
     * Constructor open file and create properties reader
     * @param file_name The name of file to be open
     * @param block_size The size of Data block
     */
    ReadFileLayer(
      const char* file_name,
      unsigned long block_size) /*throw(Exception)*/;

    /**
     * Destructor close the file
     */
    virtual ~ReadFileLayer() noexcept;

    /**
     * Get up Block in memory by index
     * @param index The index of the Data block to be put in memory
     * @return The smart pointer to accessor to mapped Data block
     */
    virtual
    ReadBlock_var
    get_read_block(const FileBlockIndex& index) /*throw(BaseException)*/;

    /**
     * Get property object by its name
     * @param name The name for property search
     * @param property The reference to return property when found
     * @return false if property not found.
     */
    virtual
    bool
    get_property(const char* name, PropertyValue& property)
      /*throw(BaseException)*/;

    virtual
    unsigned long
    area_size() const noexcept;

    /**
     * @return Shared memory block size - size of reserved fields
     */
    SizeType block_data_size() const noexcept;

    /**
     * @return file size divided on size of elemental portion of shared memory
     * used to store file. I.e. (maximum index of block that can be received and used
     * without file resize) + 1
     */
    FileBlockIndex max_block_index() const noexcept;

  public:
    class PropertyReader;

  protected:
    class ReadBlockImpl;
    friend class ReadBlockImpl;

  protected:
    const void* read_resolve_block_(const FileBlockIndex& index) /*throw(Exception)*/;

    /**
     * deletes specified block from opened blocks and
     * deletes the mappings for the specified address pointer
     * @param block_index The block index to be deleted from opened blocks
     * @param content_ The address of mapped memory to be free
     */
    void read_unresolve_block_(
      const FileBlockIndex& block_index,
      const void* content_) /*throw(Exception)*/;

    /**
     * Open file in read only mode
     */
    void open_file_(const char* filename) /*throw(Exception)*/;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;
    typedef Generics::GnuHashTable<FileBlockIndex, ReadBlockImpl*>
      ReadBlockHashTable;

    /// Wrapper on opened file descriptor
    Sys::File file_;
    /// The size of shared memory used to store one Data block
    unsigned int map_page_size_;
    /// Requested Data block size
    unsigned int block_size_;
    /// Total size, in blocks of opened file
    unsigned int file_in_blocks_size_;

    /*
    /// Total size, in bytes of opened file
    FileOffset file_size_;
    */

    mutable SyncPolicy::Mutex opened_blocks_lock_;
    ReadBlockHashTable opened_blocks_;

    std::unique_ptr<PropertyReader> property_reader_;
  };

  typedef ReferenceCounting::SmartPtr<ReadFileLayer> ReadFileLayer_var;

  /**
   * WriteFileLayer like ReadFileLayer, but allow read/write
   * portions of files in several stages
   */
  class WriteFileLayer: public virtual ExWriteBaseLayer<FileBlockIndex>
  {
  public:
    DECLARE_EXCEPTION(Exception, BaseException);

    enum OpenType
    {
      OT_OPEN,
      OT_OPEN_OR_CREATE
    };

  public:
    /**
     * Constructor open file in read/write mode
     * @param file_name The name of file to be open
     * @param block_size The size for Data block
     * @param open_type Traits for opening file, by default if the file does not
     * exist it will be created
     */
    WriteFileLayer(
      const char* file_name,
      unsigned long block_size,
      OpenType open_type = OT_OPEN_OR_CREATE) /*throw(Exception)*/;

    /**
     * Empty virtual destructor
     */
    virtual ~WriteFileLayer() noexcept;

    virtual
    ReadBlock_var
    get_read_block(const FileBlockIndex& index) /*throw(BaseException)*/;

    virtual
    WriteBlock_var
    get_write_block(const FileBlockIndex& index) /*throw(BaseException)*/;

    virtual
    bool
    get_property(const char* name, PropertyValue& property) /*throw(BaseException)*/;

    virtual
    void
    set_property(const char* name, const PropertyValue& property)
      /*throw(BaseException)*/;

    virtual
    unsigned long
    area_size() const noexcept;

    /**
     * @return Shared memory block size - size of reserved fields
     */
    unsigned long block_data_size() const noexcept;

    /**
     * @return file size divided on size of elemental portion of shared memory
     * used to store file. I.e. maximum number of blocks need to hold file in
     * memory
     */
    virtual FileBlockIndex max_block_index() const noexcept;

  protected:
    class WriteBlockImpl;
    friend class WriteBlockImpl;

    class PropertyWriter;

  protected:
    /**
     * Load the part of opened file into shared memory references by index.
     * Do resize of the file if requested to the block outside the file
     * @param index The number of Data block to be located in memory
     * @param need_to_init true mean file was enlarge and we should init data
     * @return Pointer to shared memory with Data block by index
     */
    void*
    write_resolve_block_(
      const FileBlockIndex& index, bool& need_to_init) /*throw(Exception)*/;

    /**
     * Deallocate shared memory by pointer. All allocated shared memory
     * blocks have equal size = map_page_size_, and we able to free memory
     * by pointer
     * @param block_index The reference to data block to be removed from
     *  opened blocks container - to be closed
     * @param block_content The pointer to shared memory to do unmap
     */
    void
    write_unresolve_block_i_(
      const FileBlockIndex& block_index,
      void* block_content) /*throw(Exception)*/;

    /**
     * Find block by index in opened blocks container and
     * erase it from container
     */
    void
    close_write_block_i_(
      const FileBlockIndex& block_index) /*throw(Exception)*/;

    /**
     * Open file in read-write mode and with given type
     */
    void
    open_file_(const char* filename, OpenType open_type)
      /*throw(Exception)*/;

    /**
     * Use to extend file while allocate shared memory for writing data.
     * @param new_size_in_blocks Usually, currently allocated block index + 1,
     * to resize file to current size + block size.
     */
    void resize_file_(const FileBlockIndex& new_size_in_blocks)
      /*throw(Exception)*/;

    void unresolve_block_content_(void* ptr)
      /*throw(Exception)*/;

  private:
    typedef Sync::Policy::PosixThreadRW ResolveSyncPolicy;
    typedef Sync::Policy::PosixThreadRW FileSizeSyncPolicy;
    typedef Sync::Policy::PosixThread ResizeSyncPolicy;

    typedef AdServer::Commons::StrictLockMap<
      Generics::NumericHashAdapter<FileBlockIndex>,
      Sync::Policy::PosixThread,
      AdServer::Commons::Hash2Args>
      BlockResolveLockMap;

    typedef Generics::GnuHashTable<
      Generics::NumericHashAdapter<FileBlockIndex>,
      WriteBlockImpl*>
      WriteBlockHashTable;

  private:
    /* TODO: m.b. do common struct - FileTraits */
    Sys::File file_;
    /// The size of shared memory used to store one Data block
    unsigned int map_page_size_;
    /// Requested Data block size
    unsigned int block_size_;
    /// Total size, in blocks of opened file
    Algs::AtomicInt file_in_blocks_size_;

    mutable ResizeSyncPolicy::Mutex resize_lock_;

    /// Container of blocks mapped into memory
    mutable BlockResolveLockMap block_resolve_locks_;
    mutable ResolveSyncPolicy::Mutex opened_blocks_lock_;
    WriteBlockHashTable opened_blocks_;

    std::unique_ptr<PropertyWriter> property_writer_;
  };

  typedef ReferenceCounting::SmartPtr<WriteFileLayer> WriteFileLayer_var;
}

#endif // PLAINSTORAGE_FILELAYER_HPP
