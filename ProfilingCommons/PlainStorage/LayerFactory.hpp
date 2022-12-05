// @file PlainStorage/LayerFactory.hpp

#ifndef _PLAINSTORAGE_LAYERFACTORY_HPP_
#define _PLAINSTORAGE_LAYERFACTORY_HPP_

#include <unistd.h>

#include <ProfilingCommons/PlainStorage/FileLayer.hpp>
#include <ProfilingCommons/PlainStorage/MemLayer.hpp>
#include <ProfilingCommons/PlainStorage/CacheLayer.hpp>
#include <ProfilingCommons/PlainStorage/FragmentLayer.hpp>
#include <ProfilingCommons/PlainStorage/RecordLayer.hpp>
#include <ProfilingCommons/PlainStorage/DefaultAllocatorLayer.hpp>
#include "BlockCache.hpp"
#include "CacheDelegateLayer.hpp"

namespace PlainStorage
{
  typedef
    PlainStorage::BlockExIndex<PlainStorage::FileBlockIndex>
    FragmentBlockIndex;
  typedef
    PlainStorage::ExIndexSerializer<FragmentBlockIndex>
    FragmentBlockIndexSerializer;

  /** StringSerializer: simple serializer for string */
  struct StringSerializer
  {
    SizeType size(const Generics::StringHashAdapter& in) const
      /*throw(eh::Exception)*/
    {
      return in.text().size();
    }

    SizeType size(const std::string& in) const
      /*throw(eh::Exception)*/
    {
      return in.size();
    }

    void
    save(const std::string& in, void* buf, SizeType size) const
      /*throw(eh::Exception)*/
    {
      ::memcpy(buf, in.c_str(), size);
    }

    void
    save(const Generics::StringHashAdapter& in, void* buf, SizeType size) const
      /*throw(eh::Exception)*/
    {
      ::memcpy(buf, in.text().c_str(), size);
    }

    SizeType load(const void* buf, SizeType size, std::string& in) const
    {
      in.assign((char*)buf, (char*)buf + size);
      return size;
    }

    SizeType load(
      const void* buf, SizeType size, Generics::StringHashAdapter& in) const
    {
      std::string str;
      load(buf, size, str);
      in = Generics::StringHashAdapter(str);
      return str.size();
    }
  };

  /** NumberSerializer: simple serializer for numbers */
  template<typename NumberType>
  struct NumberSerializer
  {
    SizeType size(const NumberType&) const
      /*throw(eh::Exception)*/
    {
      return sizeof(NumberType);
    }

    SizeType size(const Generics::NumericHashAdapter<NumberType>&) const
      /*throw(eh::Exception)*/
    {
      return sizeof(NumberType);
    }

    void
    save(const NumberType& in, void* buf, SizeType size) const
      /*throw(eh::Exception)*/
    {
      (void)size;
      assert(size >= sizeof(NumberType));
      *static_cast<NumberType*>(buf) = in;
    }

    void
    save(const Generics::NumericHashAdapter<NumberType>& in, void* buf, SizeType size) const
      /*throw(eh::Exception)*/
    {
      (void)size;
      assert(size >= sizeof(NumberType));
      *static_cast<NumberType*>(buf) = in.value();
    }

    SizeType load(const void* buf, SizeType size, NumberType& in) const
    {
      (void)size;
      assert(size == sizeof(NumberType));
      in = *static_cast<const NumberType*>(buf);
      return sizeof(NumberType);
    }

    SizeType load(const void* buf, SizeType size, Generics::NumericHashAdapter<NumberType>& in) const
    {
      (void)size;
      assert(size == sizeof(NumberType));
      in = Generics::NumericHashAdapter<NumberType>(*static_cast<const NumberType*>(buf));
      return sizeof(NumberType);
    }
  };

  /**
   * Structure that construct layers hierarchy, through make-function
   */
  template<typename SyncPolicyType>
  struct LayerFactory
  {
    typedef WriteBlockCache<FileBlockIndex> CacheT;

    typedef
      PlainStorage::WriteFragmentLayer<PlainStorage::FileBlockIndex>
      WriteFragmentLayerT;

    typedef
      ReferenceCounting::SmartPtr<WriteFragmentLayerT>
      WriteFragmentLayer_var;

    typedef FragmentBlockIndex IndexT;
    typedef PlainStorage::ExIndexSerializer<IndexT> IndexSerializerT;

    typedef
      PlainStorage::WriteRecordLayer<
        IndexT,
        PlainStorage::ExIndexSerializer<IndexT>,
        SyncPolicyType>
      WriteRecordLayerT;

    typedef
      ReferenceCounting::SmartPtr<CacheT>
      Cache_var;

    typedef
      ReferenceCounting::SmartPtr<WriteRecordLayerT>
      WriteRecordLayer_var;

    /**
     * Block size (elemental smallest piece of file)
     * will be equal ::getpagesize()*16 = 65536 bytes
     *   Provides nesting layers into each other,
     * requests can be delegated down the hierarchy layers
     * 5. Record
     * 4. Fragment
     * 3. Cache
     * 2. Allocator
     * 1. File
     */
    static
    ReferenceCounting::SmartPtr<WriteRecordLayerT>
    create_write_record_layer(
      const char* file_name,
      CacheT* block_cache = 0)
      /*throw(eh::Exception)*/
    {
      ReferenceCounting::SmartPtr<
        PlainStorage::WriteBaseLayer<FileBlockIndex> > base_layer;
      ReferenceCounting::SmartPtr<
        BlockAllocator<FileBlockIndex> > base_allocator;

      {
        PlainStorage::WriteFileLayer_var file_layer(
          new PlainStorage::WriteFileLayer(file_name, ::getpagesize()*16));

        PlainStorage::DefaultAllocatorLayer_var default_allocator(
          new PlainStorage::DefaultAllocatorLayer(file_layer));

        if(block_cache)
        {
          ReferenceCounting::SmartPtr<
            PlainStorage::WriteCacheDelegateLayer<FileBlockIndex> > cache_layer(
              new PlainStorage::WriteCacheDelegateLayer<FileBlockIndex>(
                default_allocator,
                default_allocator,
                block_cache));

          base_layer = cache_layer;
          base_allocator = cache_layer;
        }
        else
        {
          base_layer = default_allocator;
          base_allocator = default_allocator;
        }
      }

      WriteFragmentLayer_var fragment_layer(
        new WriteFragmentLayerT(base_layer, base_allocator));

      WriteRecordLayer_var record_layer(
        new WriteRecordLayerT(fragment_layer, fragment_layer));

      return record_layer;
    }

    static
    ReferenceCounting::SmartPtr<WriteRecordLayerT>
    create_mem_write_record_layer()
      /*throw(eh::Exception)*/
    {
      PlainStorage::WriteMemLayer_var mem_layer(
        new PlainStorage::WriteMemLayer(::getpagesize()*16));

      PlainStorage::DefaultAllocatorLayer_var default_allocator(
        new PlainStorage::DefaultAllocatorLayer(mem_layer));

      WriteFragmentLayer_var fragment_layer(
        new WriteFragmentLayerT(default_allocator, default_allocator));

      WriteRecordLayer_var record_layer(
        new WriteRecordLayerT(fragment_layer, fragment_layer));

      return record_layer;
    }
  };
}

#endif //_PLAINSTORAGE_LAYERFACTORY_HPP_
