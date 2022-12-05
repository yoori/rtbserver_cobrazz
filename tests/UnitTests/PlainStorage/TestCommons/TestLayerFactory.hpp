// @file PlainStorage/TestCommons/TestLayerFactory.hpp

#ifndef COMMONS_TESTLAYERFACTORY_HPP
#define COMMONS_TESTLAYERFACTORY_HPP

#include <ProfilingCommons/PlainStorage/FileLayer.hpp>
#include <ProfilingCommons/PlainStorage/CacheLayer.hpp>
#include <ProfilingCommons/PlainStorage/FragmentLayer.hpp>
#include <ProfilingCommons/PlainStorage/RecordLayer.hpp>
#include <ProfilingCommons/PlainStorage/DefaultAllocatorLayer.hpp>

#include <ProfilingCommons/PlainStorage/LayerFactory.hpp>
#include <ProfilingCommons/PlainStorage/DefaultSyncIndexStrategy.hpp>
#include <ProfilingCommons/ProfileMap/MemIndexProfileMap.hpp>

#include "TestLayer.hpp"

using namespace PlainStorage;

/**
 *  TestLayerFactory
 */
class TestLayerFactory
{
public:
  typedef Sync::Policy::PosixThread SyncPolicy;
  typedef PlainStorage::FileBlockIndex TestBlockIndex;
  
  typedef
    PlainStorage::WriteFragmentLayer<TestBlockIndex>
    WriteFragmentLayerT;

  typedef
    ReferenceCounting::SmartPtr<WriteFragmentLayerT>
    WriteFragmentLayer_var;

  typedef
    PlainStorage::WriteCacheLayer<TestBlockIndex>
    WriteCacheLayerT;

  typedef
    ReferenceCounting::SmartPtr<WriteCacheLayerT>
    WriteCacheLayer_var;
    
  typedef
    WriteRecordLayer<
      BlockExIndex<TestBlockIndex>,
      ExIndexSerializer<
        BlockExIndex<TestBlockIndex> > >
    WriteRecordLayerT;

  typedef
    ReferenceCounting::SmartPtr<WriteRecordLayerT>
    WriteRecordLayer_var;

  typedef LayerFactory<SyncPolicy>::WriteRecordLayerT::NextBlockIndex
    TestIndex;
  
  typedef
    DefaultSyncIndexStrategy<
      std::string, StringSerializer, TestIndex, TestIndexSerializer>
    SyncIndexStrategy;

  typedef AdServer::ProfilingCommons::MemIndexProfileMap<
    std::string,
    PlainStorage::FragmentBlockIndex,
    AdServer::ProfilingCommons::DefaultMapTraits<
      std::string,
      PlainStorage::StringSerializer,
      PlainStorage::FragmentBlockIndex,
      PlainStorage::FragmentBlockIndexSerializer> >
    MapT;

  WriteRecordLayer_var open(TestWriteLayer* test_layer)
    /*throw(BaseException)*/
  {
    try
    {
      default_allocator_ = new DefaultAllocatorLayer(test_layer);
      cache_layer_ = 
        new WriteCacheLayer<TestBlockIndex>(
          test_layer, default_allocator_, 128);

      fragment_layer_ =
        new WriteFragmentLayerT(cache_layer_, cache_layer_);
        
      return new WriteRecordLayerT(fragment_layer_, fragment_layer_);
    }
    catch (const BaseException& ex)
    {
      throw;
    }
  }
    
  WriteRecordLayer_var open(
    TestWriteLayer* test_layer,
    BlockAllocator<TestBlockIndex>* test_allocator)
    /*throw(BaseException)*/
  {
    try
    {
      default_allocator_ = ReferenceCounting::add_ref(test_allocator);

      cache_layer_ = 
        new WriteCacheLayer<TestBlockIndex>(
          test_layer, default_allocator_, 128);

      fragment_layer_ =
        new WriteFragmentLayerT(cache_layer_, cache_layer_);

      return new WriteRecordLayerT(fragment_layer_, fragment_layer_);
    }
    catch (const BaseException& ex)
    {
      throw;
    }
  }

  ReferenceCounting::SmartPtr<BlockAllocator<TestBlockIndex> >
    default_allocator_;
  WriteCacheLayer_var cache_layer_;
  WriteFragmentLayer_var fragment_layer_;
  WriteRecordLayer_var record_layer_;
};

#endif // COMMONS_TESTLAYERFACTORY_HPP
