// @file DiagnosisTest/Main.cpp
 
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <Generics/CRC.hpp>

#include <ProfilingCommons/PlainStorage/FileLayer.hpp>
#include <ProfilingCommons/PlainStorage/CacheLayer.hpp>
#include <ProfilingCommons/PlainStorage/FragmentLayer.hpp>
#include <ProfilingCommons/PlainStorage/RecordLayer.hpp>
#include <ProfilingCommons/PlainStorage/DefaultAllocatorLayer.hpp>

#include "../TestCommons/TestLayer.hpp" 
#include "../TestCommons/TestLayerFactory.hpp" 

unsigned long
check_sum(const void* buf, unsigned long sz)
{
  return Generics::CRC::quick(0, buf, sz);
}

template<typename MapType>
u_int32_t
insert_test(MapType& test_map, const char* key, size_t record_size)
{
  Generics::SmartMemBuf_var buf(new Generics::SmartMemBuf(record_size));
  for(size_t i = 0; i < record_size; ++i)
  {
    buf->membuf().get<unsigned char>()[i] = rand() % RAND_MAX;
  }
  u_int32_t cs = check_sum(buf->membuf().data(), record_size);
  test_map.save_profile(key, Generics::transfer_membuf(buf));
  return cs;
}

template<typename MapType>
bool
read_test(MapType& test_map, const char* key, u_int32_t check_sum_val)
{
  Generics::ConstSmartMemBuf_var rb = test_map.get_profile(key);
  if(!rb.in())
  {
    return false;
  }
  
  u_int32_t cs = check_sum(rb->membuf().data(), rb->membuf().size());
  return cs == check_sum_val;
}

void
diagnosis_map(const char* NAME)
{
  std::cout << "** " << NAME << " (initialize)" << std::endl;
  
  ReferenceCounting::SmartPtr<TestWriteLayer> diagnosis_layer(
    new TestWriteLayer(false, &std::cout));
  ReferenceCounting::SmartPtr<TestBlockAllocator> diagnosis_allocator(
    new TestBlockAllocator(diagnosis_layer, &std::cout));
  
  TestLayerFactory factory;
  
  TestLayerFactory::WriteRecordLayer_var record_layer =
    factory.open(diagnosis_layer, diagnosis_allocator);

  ReferenceCounting::SmartPtr<TestLayerFactory::MapT> test_map(
    new TestLayerFactory::MapT(record_layer, record_layer));

  std::cout << "** " << NAME << " (start scenario)" << std::endl;
  std::cout << "** " << NAME << " (insert - 100)" << std::endl;

  u_int32_t crc = insert_test(*test_map, "key1", 57537);

  std::cout << "** " << NAME << " (read - 100)" << std::endl;

  read_test(*test_map, "key1", crc);

  std::cout << "** " << NAME << " (read - 20(9) )" << std::endl;
}

void
large_diagnosis_map(const char* NAME)
{
  std::cout << "** " << NAME << " (initialize)" << std::endl;
  
  ReferenceCounting::SmartPtr<TestWriteLayer> diagnosis_layer(
    new TestWriteLayer(false, &std::cout));
  ReferenceCounting::SmartPtr<TestBlockAllocator> diagnosis_allocator(
    new TestBlockAllocator(diagnosis_layer, &std::cout));
  
  TestLayerFactory factory;
  
  TestLayerFactory::WriteRecordLayer_var record_layer =
    factory.open(diagnosis_layer, diagnosis_allocator);

  ReferenceCounting::SmartPtr<TestLayerFactory::MapT> test_map(
    new TestLayerFactory::MapT(record_layer, record_layer));

  std::cout << "** " << NAME << " (start scenario)" << std::endl;
  std::cout << "** " << NAME << " (insert - 80000)" << std::endl;

  u_int32_t crc = insert_test(*test_map, "key1", 80000);

  std::cout << "** " << NAME << " (read - 80000)" << std::endl;

  read_test(*test_map, "key1", crc);

  std::cout << "** " << NAME << " (read - 79801(100) )" << std::endl;
}

void
fragment_diagnosis_map(const char* NAME)
{
  std::cout << "** " << NAME << " (initialize)" << std::endl;
  
  ReferenceCounting::SmartPtr<TestWriteLayer> diagnosis_layer(
    new TestWriteLayer(false, &std::cout));
  ReferenceCounting::SmartPtr<TestBlockAllocator> diagnosis_allocator(
    new TestBlockAllocator(diagnosis_layer, &std::cout));
  
  TestLayerFactory factory;
  
  TestLayerFactory::WriteRecordLayer_var record_layer =
    factory.open(diagnosis_layer, diagnosis_allocator);

  ReferenceCounting::SmartPtr<TestLayerFactory::MapT> test_map(
    new TestLayerFactory::MapT(record_layer, record_layer));

  std::cout << "** " << NAME << " (start scenario)" << std::endl;
  std::cout << "** " << NAME << " (insert - 80, 80)" << std::endl;

  u_int32_t crc = insert_test(*test_map, "key1", 80);
  u_int32_t crc2 = insert_test(*test_map, "key2", 80);

  std::cout << "** " << NAME << " (read - 80, 80)" << std::endl;

  read_test(*test_map, "key1", crc);
  read_test(*test_map, "key2", crc2);

  std::cout << "** " << NAME << " (rewrite - 200)" << std::endl;

  insert_test(*test_map, "key1", 200);

  std::cout << "** " << NAME << " (scenario fin)" << std::endl;
}

void 
diagnosis_insert_erase(const char* NAME)
{
  std::cout << "** " << NAME << " (initialize)" << std::endl;
  
  ReferenceCounting::SmartPtr<TestWriteLayer> diagnosis_layer(
    new TestWriteLayer(false, &std::cout));
  ReferenceCounting::SmartPtr<TestBlockAllocator> diagnosis_allocator(
    new TestBlockAllocator(diagnosis_layer, &std::cout));
  
  TestLayerFactory factory;
  
  TestLayerFactory::WriteRecordLayer_var record_layer =
    factory.open(diagnosis_layer, diagnosis_allocator);

  ReferenceCounting::SmartPtr<TestLayerFactory::MapT> test_map(
    new TestLayerFactory::MapT(record_layer, record_layer));

  /*u_int32_t crc = */insert_test(*test_map, "key1", 80000);
  test_map->remove_profile("key1");
}

int
main(int /*argc*/, char** /*argv*/) noexcept
{
  try
  {
    diagnosis_map("INSERT READ");
    diagnosis_insert_erase("INSERT ERASE");
    large_diagnosis_map("LARGE INSERT READ");
    fragment_diagnosis_map("SMALL TWICE REWRITE");
    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return -1;
}
