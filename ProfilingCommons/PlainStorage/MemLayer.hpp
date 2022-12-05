/**
 * @file PlainStorage/MemLayer.hpp
 * MemLayer implementation of fixed size records layer (RAM based)
 */
#ifndef PLAINSTORAGE_MEMLAYER_HPP
#define PLAINSTORAGE_MEMLAYER_HPP

#include <functional>
#include <memory>

#include <Sync/SyncPolicy.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/Time.hpp>

#include <Commons/LockMap.hpp>
#include "BaseLayer.hpp"

namespace PlainStorage
{
  typedef u_int64_t MemBlockIndex;

  /**
   * WriteMemLayer
   */
  class WriteMemLayer: public virtual ExWriteBaseLayer<MemBlockIndex>
  {
  public:
    DECLARE_EXCEPTION(Exception, BaseException);

  public:
    /**
     * Constructor
     * @param block_size The size for Data block
     */
    WriteMemLayer(unsigned long block_size) /*throw(Exception)*/;

    /**
     * Empty virtual destructor
     */
    virtual ~WriteMemLayer() noexcept;

    virtual
    ReadBlock_var
    get_read_block(const MemBlockIndex& index) /*throw(BaseException)*/;

    virtual
    WriteBlock_var
    get_write_block(const MemBlockIndex& index) /*throw(BaseException)*/;

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

    unsigned long block_data_size() const noexcept;

    virtual MemBlockIndex max_block_index() const noexcept;

  protected:
    class WriteBlockImpl;
    friend class WriteBlockImpl;

  private:
    typedef std::map<MemBlockIndex, WriteBlock_var> BlockMap;

    typedef std::map<std::string, PropertyValue> PropertyMap;

    typedef Sync::Policy::PosixThread SyncPolicy;

  private:
    /// Requested Data block size
    const unsigned int block_size_;

    mutable SyncPolicy::Mutex blocks_lock_;
    BlockMap blocks_;

    mutable SyncPolicy::Mutex properties_lock_;
    PropertyMap properties_;
  };

  typedef ReferenceCounting::SmartPtr<WriteMemLayer> WriteMemLayer_var;
}

#endif // PLAINSTORAGE_MEMLAYER_HPP
