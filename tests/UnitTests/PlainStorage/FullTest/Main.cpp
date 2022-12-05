// @file FullTest/Main.cpp

#include <iostream>

#include <Generics/AppUtils.hpp>
#include <Generics/Time.hpp>
#include <ProfilingCommons/PlainStorage/LayerFactory.hpp>

#include "../TestCommons/TestLayer.hpp"
#include "../TestCommons/TestLayerFactory.hpp"

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char CHECK_ROOT[] = "/PlStFullTestDir/";
  const char TEST_FRAGMENT_FILE[] = "fragment_layer.plst";
  const char TEST_LAYER_FILE[] = "record_layer.plst";

  const char USAGE[] =
    "PlStFullTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";
}

/**
 * tests:
 *   record layer -> test layer
 *   record layer -> fragment layer -> cache layer -> file layer
 */
void
scan_record_layer()
{
  using namespace PlainStorage;

  TestWriteLayer_var test_layer(new TestWriteLayer);
  TestBlockSizeAllocator_var test_allocator(
    new TestBlockSizeAllocator(test_layer));

  typedef
    PlainStorage::WriteRecordLayer<TestIndex, TestIndexSerializer>
    WriteRecordLayerT;

  typedef
    ReferenceCounting::SmartPtr<WriteRecordLayerT>
    WriteRecordLayer_var;

  WriteRecordLayer_var record_layer(
    new WriteRecordLayerT(test_layer, test_allocator));

  WriteRecordLayerT::AllocatedBlock alloc_block;
  record_layer->allocate(alloc_block);
}

bool
test_fragment_layer(const char* file_name)
{
  try
  {
    typedef
      PlainStorage::WriteFragmentLayer<PlainStorage::FileBlockIndex>
      WriteFragmentLayerT;

    typedef
      ReferenceCounting::SmartPtr<WriteFragmentLayerT>
      WriteFragmentLayer_var;

    typedef
      PlainStorage::WriteCacheLayer<PlainStorage::FileBlockIndex>
      WriteCacheLayerT;

    typedef
      ReferenceCounting::SmartPtr<WriteCacheLayerT>
      WriteCacheLayer_var;

    PlainStorage::WriteFileLayer_var file_layer(
      new PlainStorage::WriteFileLayer(file_name, ::getpagesize()*16));

    PlainStorage::DefaultAllocatorLayer_var default_allocator(
      new PlainStorage::DefaultAllocatorLayer(file_layer));

    WriteCacheLayer_var cache_layer(
      new PlainStorage::WriteCacheLayer<PlainStorage::FileBlockIndex>(
        default_allocator, default_allocator, 10*1024*1024));

    WriteFragmentLayer_var fragment_layer(
      new WriteFragmentLayerT(cache_layer, cache_layer));

    /*
    {
      const unsigned long TEST_SIZE = 10000;
      typedef WriteFragmentLayerT::AllocatedBlockSeq AllocatedBlockSeqT;
      AllocatedBlockSeqT allocated_blocks;

      fragment_layer->allocate_seq(TEST_SIZE, allocated_blocks);

      std::cout << "Blocks allocated for size = " << TEST_SIZE << std::endl;

      unsigned int i = 0;
      for (AllocatedBlockSeqT::const_iterator it = allocated_blocks.begin();
          it != allocated_blocks.end(); ++it, ++i)
      {
        std::cout << "  "
                  << "index: " << it->block_index.base_index << ", "
                  << "available size: " << it->block->available_size()
                  << std::endl;
      }
    }
    */
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "test_fragment_layer(): Caught exception: "
      << ex.what() << std::endl;
    return false;
  }

  return true;
}

/**
 * RecordLayer testing
 */
struct WriteReadTest
{
  bool
  operator ()(
    PlainStorage::WriteBlock* write_block,
    PlainStorage::SizeType sz,
    char* buf,
    char* read_buf,
    bool test = true) const
  {
    for (std::size_t i = 0; i < sz; ++i)
    {
      buf[i] = i % 100;
    }

    write_block->write(buf, sz);

    PlainStorage::SizeType result_sz = 0;

    result_sz = write_block->size();

    if (sz != result_sz)
    {
      std::cerr << "ERROR: non correct size after writing." << std::endl;
      return false;
    }

    write_block->read(read_buf, sz);

    if (test)
    {
      for (size_t i = 0; i < sz; ++i)
      {
        if (buf[i] != read_buf[i])
        {
          std::cerr << "ERROR: non correct content. Diff in pos = "
            << i << ":'" << buf[i] << "' != '"
            << read_buf[i] << "'" << std::endl;
          return false;
        }
      }
    }

    return true;
  }
};

struct WriteReadRewriteReadTest
{
  bool
  operator ()(
    PlainStorage::WriteBlock* write_block,
    PlainStorage::SizeType sz,
    char* buf,
    char* read_buf,
    bool test = true) const
  {
    WriteReadTest op;
    return
      op(write_block, sz / 2, buf, read_buf, test) &&
      op(write_block, sz, buf, read_buf, test);
  }
};

struct FullRewriteTest
{
  bool
  operator ()(
    PlainStorage::WriteBlock* write_block,
    PlainStorage::SizeType sz,
    char* buf,
    char* read_buf,
    bool test = true) const
  {
    bool ret = true;
    WriteReadTest op;
    for (std::size_t i = 0; i < sz; ++i)
    {
      bool sub_ret = op(write_block, i, buf, read_buf, test);
      ret &= sub_ret;
    }

    for (int i = sz; i >= 0; --i)
    {
      bool sub_ret = op(write_block, i, buf, read_buf, test);
      ret &= sub_ret;
    }

    bool sub_ret = op(write_block, 4, buf, read_buf, test);
    ret &= sub_ret;

    sub_ret = op(write_block, 44, buf, read_buf, test);
    ret &= sub_ret;

    sub_ret = op(write_block, 44, buf, read_buf, test);
    ret &= sub_ret;

    return ret;
  }
};

template <typename _T, typename _OP>
struct AllocateOp
{
  bool
  operator ()(
    _T* layer,
    PlainStorage::SizeType sz,
    char* buf,
    char* read_buf,
    bool test) const
  {
    typename _T::AllocatedBlock all;
    layer->allocate(all);
    return op(all.block, sz, buf, read_buf, test);
  }

  _OP op;
};

template <typename _T, typename _OP>
bool
record_layer_one_check(
  const char* NAME, _T* layer, const _OP& op)
{
  const unsigned long STOP_SIZE = 320; // 32000
  Generics::ArrayAutoPtr<char> buf(STOP_SIZE);
  Generics::ArrayAutoPtr<char> read_buf(STOP_SIZE);

  try
  {
    if (!op(layer, STOP_SIZE, buf.get(), read_buf.get(), true))
    {
      std::cerr << NAME << ": ERROR" << std::endl;

      return false;
    }
  }
  catch (...)
  {
    std::cerr << NAME << ": ERROR" << std::endl;
    throw;
  }

  std::cout << NAME << " success." << std::endl;
  return true;
}

template <typename _T, typename _OP>
bool
test_record_layer_constraints(
  const char* NAME, _T* layer, const _OP& op)
{
  const int RECORDS_COUNT = 1000;
  const int SIZE_START = 0;
  const int SIZE_STEP = 100;

  Generics::ArrayChar buf(SIZE_START + RECORDS_COUNT*SIZE_STEP);
  Generics::ArrayChar read_buf(SIZE_START + RECORDS_COUNT*SIZE_STEP);

  for (size_t i = 0; i < SIZE_START + RECORDS_COUNT*SIZE_STEP; ++i)
  {
    buf.get()[i] = i % ('Z' - 'A') + 'A';
  }

  for (size_t sz = SIZE_START;
     sz < SIZE_START + RECORDS_COUNT*SIZE_STEP;
     sz += SIZE_STEP)
  {
    try
    {
      if (!op(layer, sz, buf.get(), read_buf.get(), true))
      {
        std::cerr << NAME
          << ": ERROR of allocate & read & write testing for size = "
          << sz << std::endl;

        return false;
      }
    }
    catch (...)
    {
      std::cerr << NAME
        << ": ERROR of allocate & read & write testing for size = "
        << sz << std::endl;
      throw;
    }
  }

  std::cout << NAME << " success." << std::endl;
  return true;
}

template <typename _T, typename _OP>
bool
test_record_layer_performance_(
  const char* NAME,
  _T* layer,
  int RECORDS_COUNT,
  int SIZE_START,
  int SIZE_STEP,
  const _OP& op,
  unsigned long* res_sum_size = 0)
{
  Generics::ArrayChar buf(SIZE_START + RECORDS_COUNT*SIZE_STEP);
  Generics::ArrayChar read_buf(SIZE_START + RECORDS_COUNT*SIZE_STEP);

  for (int i = 0; i < SIZE_START + RECORDS_COUNT*SIZE_STEP; ++i)
  {
    buf.get()[i] = i % ('Z' - 'A') + 'A';
  }

  Generics::Timer timer;
  timer.start();

  for (int i = 0; i < RECORDS_COUNT; ++i)
  {
    if (res_sum_size)
    {
      *res_sum_size += SIZE_START + i*SIZE_STEP;
    }

    if (!op(layer,
      SIZE_START + i*SIZE_STEP, buf.get(), read_buf.get(), false))
    {
      std::cerr << "ERROR: allocate_read_write_test for record with size: "
        << i << std::endl;
      return false;
    }
  }

  timer.stop();
  Generics::Time avg_elapsed_time = timer.elapsed_time() / RECORDS_COUNT;

  std::cout
    << "'" << NAME << "':" << std::endl
    << "  read count: " << RECORDS_COUNT << std::endl
    << "  start size: " << SIZE_START << std::endl
    << "  step size: " << SIZE_STEP << std::endl
    << "  average time: " << avg_elapsed_time << std::endl;

  return true;
}

template <typename _T, typename _OP>
bool
test_record_layer_performance(
  _T* layer, const _OP& /*op*/, unsigned long* res_sum_size = 0)
{
  /* low sizes testing */
//unsigned long all_size = 0;

  bool ret = test_record_layer_performance_(
    "low size", layer, 1000, 1, 1, _OP(), res_sum_size);

  ret &= test_record_layer_performance_(
    "medium size", layer, 1000, 10, 10, _OP(), res_sum_size);

  ret &= test_record_layer_performance_(
    "big size", layer, 1000, 1000, 1000, _OP(), res_sum_size);

  return ret;
}

bool
create_and_test_record_layer(const char* file_name)
{
  try
  {
    typedef
      PlainStorage::LayerFactory<
        Sync::Policy::PosixThreadRW> LayerFactoryT;

    LayerFactoryT::WriteRecordLayer_var record_layer(
      LayerFactoryT::create_write_record_layer(file_name));

    return
      record_layer_one_check(
        "record_layer_one_check<FullRewriteTest>",
        record_layer.in(),
        AllocateOp<LayerFactoryT::WriteRecordLayerT, FullRewriteTest>()) &&
      test_record_layer_constraints(
        "test_record_layer_constraints<WriteReadTest>",
        record_layer.in(),
        AllocateOp<LayerFactoryT::WriteRecordLayerT, WriteReadTest>()) &&
      test_record_layer_constraints(
        "test_record_layer_constraints<WriteReadRewriteReadTest>",
        record_layer.in(),
        AllocateOp<LayerFactoryT::WriteRecordLayerT,
          WriteReadRewriteReadTest>()) &&
      test_record_layer_performance(
        record_layer.in(),
        AllocateOp<LayerFactoryT::WriteRecordLayerT, WriteReadTest>());
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "test_record_layer(): Caught exception: "
      << ex.what() << std::endl;
    return false;
  }

  return true;
}

bool
size_test_record_layer()
{
  try
  {
    TestWriteLayer_var test_layer(new TestWriteLayer(false, 0));
    TestBlockAllocator_var test_allocator(
      new TestBlockAllocator(test_layer, 0));

    TestLayerFactory factory;

    TestLayerFactory::WriteRecordLayer_var record_layer =
      factory.open(test_layer, test_allocator);

    unsigned long sum_res_size = 0;

    bool res = test_record_layer_performance(
      record_layer.in(),
      AllocateOp<TestLayerFactory::WriteRecordLayerT, WriteReadTest>(),
      &sum_res_size);

    std::cout << "size test result: allocated size = "
      << sum_res_size << ", real allocated size = "
      << test_allocator->allocated_size() << std::endl;

    return res;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "test_record_layer(): Caught exception: "
      << ex.what() << std::endl;
  }

  return false;
}


bool
init(int& argc, char**& argv) /*throw(eh::Exception)*/
{
  using namespace Generics::AppUtils;
  Args args;
  CheckOption opt_help;

  args.add(equal_name("path") || short_name("p"), root_path);
  args.add(equal_name("help") || short_name("h"), opt_help);

  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return false;
  }

  return true;
}


int
main(int argc, char* argv[])
{
  int result = 0;
  try
  {
    if (!init(argc, argv))
    {
      return 0;
    }

    system(("mkdir -p " + *root_path + CHECK_ROOT).c_str());

    test_fragment_layer(
      (*root_path + CHECK_ROOT + TEST_FRAGMENT_FILE).c_str());

    result += create_and_test_record_layer(
      (*root_path + CHECK_ROOT + TEST_LAYER_FILE).c_str());

    result += size_test_record_layer();

    std::cout << "============================" << std::endl;
    scan_record_layer();

  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    result = -1;
  }
  return result;
}
