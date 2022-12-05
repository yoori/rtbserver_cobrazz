#include <iostream>

#include <Generics/AppUtils.hpp>
#include <Generics/Time.hpp>
#include <TestCommons/ActiveObjectCallback.hpp>
#include <ProfilingCommons/PlainStorage/LayerFactory.hpp>
#include <ProfilingCommons/ProfileMap/MemIndexProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>

namespace
{
  const char USAGE[] =
    "PlainStorageUtil [OPTIONS] COMMAND file\n"
    "COMMAND:\n"
    "  print-info : print debug information about block sequences.\n"
    "  resave-mem-index : resave map index based storage file (only for string keys now).\n"
    "  reset-allocator : remove information about free blocks.\n"
    "  convert-mem-to-level : convert map to level map\n"
    "OPTIONS:\n"
    "  -h, --help : show this message.\n"
    "  -c, --print-content : print record binary content\n"
    "  --rw-level-max-size : rwlevel max size\n"
    "  --max-undumped-size : max undumped size\n"
    "  --max-levels0 : max levels0 count";
}

namespace
{
  using namespace PlainStorage;

  struct BinKey
  {
    BinKey()
      : size(0)
    {}

    BinKey(const BinKey& init)
    {
      size = init.size;
      buf.reset(init.size);
      ::memcpy(buf.get(), init.buf.get(), init.size);
    }

    unsigned long
    hash() const
    {
      if(size < 4)
      {
        return 0;
      }

      return *reinterpret_cast<uint32_t*>(buf.get());
    }

    BinKey&
    operator=(const BinKey& init)
    {
      size = init.size;
      buf.reset(init.size);
      ::memcpy(buf.get(), init.buf.get(), init.size);
      return *this;
    }

    bool
    operator<(const BinKey& right) const
    {
      int res = ::memcmp(buf.get(), right.buf.get(), std::min(size, right.size));
      if(res < 0)
      {
        return true;
      }
      else if(res == 0)
      {
        return right.size > size;
      }

      return false;
    }

    bool
    operator== (const BinKey& right) const
    {
      return (!(*this < right) && !(right < *this));
    }

    std::string str() const
    {
      return std::string(reinterpret_cast<const char*>(buf.get()), size);
    }

    friend
    std::ostream&
    operator<< (
      std::ostream& os,
      const BinKey& key)
      noexcept
    {
      os << key.str();
      return os;
    }

    unsigned long size;
    Generics::ArrayAutoPtr<unsigned char> buf;
  };

  struct BinKeySerializer
  {
    SizeType size(const BinKey& in) const
      /*throw(eh::Exception)*/
    {
      return in.size;
    }

    void
    save(const BinKey& in, void* buf, SizeType size) const
      /*throw(eh::Exception)*/
    {
      ::memcpy(buf, in.buf.get(), size);
    }

    SizeType
    load(const void* buf, SizeType size, BinKey& in) const
    {
      in.size = size;
      in.buf.reset(size);
      ::memcpy(in.buf.get(), buf, size);
      return size;
    }

    void
    read(BinKey& key, void* buf, unsigned long buf_size) const
    {
      load(buf, buf_size, key);
    }

    void
    write(void* buf, unsigned long buf_size, const BinKey& key) const
    {
      save(key, buf, buf_size);
    }
  };

  typedef
    PlainStorage::WriteFragmentLayer<PlainStorage::FileBlockIndex>
    WriteFragmentLayerT;

  typedef ReferenceCounting::SmartPtr<WriteFragmentLayerT> 
    WriteFragmentLayer_var;
  
  typedef 
    PlainStorage::WriteRecordLayer<
      BlockExIndex<PlainStorage::FileBlockIndex>,
      ExIndexSerializer<
        BlockExIndex<PlainStorage::FileBlockIndex> > >
    WriteRecordLayerT;

  typedef ReferenceCounting::SmartPtr<WriteRecordLayerT>
    WriteRecordLayer_var;

  typedef AdServer::ProfilingCommons::MemIndexProfileMap<
    BinKey,
    WriteRecordLayerT::NextBlockIndex,
    AdServer::ProfilingCommons::DefaultMapTraits<
      BinKey,
      BinKeySerializer,
      WriteRecordLayerT::NextBlockIndex,
      PlainStorage::ExIndexSerializer<
        WriteRecordLayerT::NextBlockIndex
    > > >
    MemIndexMapT;

  typedef ReferenceCounting::SmartPtr<MemIndexMapT>
    MemIndexMapT_var;
}

void print_plain_(
  std::ostream& ostr,
  const void* buf,
  unsigned long size,
  const char* prefix,
  bool print_chars = false)
  noexcept
{
  ostr << prefix;

  for(unsigned long i = 0; i < size; ++i)
  {
    unsigned char ch = *((const unsigned char*)buf + i);
    ostr << "0x" << std::hex << std::setfill('0') << std::setw(2) <<
      (int)ch;

    if(print_chars)
    {
      ostr << "(" << ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ? (char)ch : ' ') << ")";
    }
    ostr << " ";
    if(i && (i + 1) % 16 == 0)
    {
      ostr << std::endl << prefix;
    }
  }

  ostr << std::dec << std::setw(0);
}

void
print_block_links(std::ostream& /*out*/, const char* file, bool print_content)
{
  PlainStorage::WriteFileLayer_var file_layer(
    new PlainStorage::WriteFileLayer(file, ::getpagesize()*16));

  PlainStorage::PropertyValue ct;
  if(file_layer->get_property("FileLayer::CreateTime", ct))
  {
    std::cout << "FileLayer::CreateTime = " <<
      std::string(static_cast<const char*>(ct.value()), ct.size()) << std::endl;
  }
  
  {
    PlainStorage::ReadBlock_var property_block = file_layer->get_read_block(0);
    Generics::ArrayAutoPtr<char> buf(property_block->size());
    property_block->read(buf.get(), property_block->size());
    std::cout << "Property Block Content:" << std::endl;
    print_plain_(std::cout, buf.get(), property_block->size(), "  ", true);
    std::cout << std::endl;
    char* cursor = buf.get();
    while(cursor < buf.get() + property_block->size())
    {
      std::cout << "'" <<
        std::string(
          cursor + 2*sizeof(uint32_t), *reinterpret_cast<uint32_t*>(cursor)) << "': '" <<
        std::string(
          cursor + 2*sizeof(uint32_t) + *reinterpret_cast<uint32_t*>(cursor),
          *(reinterpret_cast<uint32_t*>(cursor) + 1)) << "'" << std::endl;
      cursor += 2*sizeof(uint32_t) + *reinterpret_cast<uint32_t*>(cursor) +
        *(reinterpret_cast<uint32_t*>(cursor) + 1);
    }
  }
  
  PlainStorage::DefaultAllocatorLayer_var default_allocator(
    new PlainStorage::DefaultAllocatorLayer(file_layer));

  WriteFragmentLayer_var fragment_layer(
    new WriteFragmentLayerT(file_layer, default_allocator));

  WriteRecordLayer_var record_layer(
    new WriteRecordLayerT(fragment_layer, fragment_layer));
  
  {
    /* read index */
    MemIndexMapT_var read_map(new MemIndexMapT(record_layer, record_layer));

    /* get free indexes */
    std::set<PlainStorage::FileBlockIndex> free_indexes;
    std::string allocator_error;
    
    try
    {
      default_allocator->free_indexes_(free_indexes);
    }
    catch(const eh::Exception& ex)
    {
      allocator_error = ex.what();
    }
    
    std::cout << "number of keys: " << read_map->size() << std::endl;
    
    std::list<BinKey> keys;
    read_map->copy_keys(keys);

    unsigned long sum_used_size = 0;

    for(std::list<BinKey>::const_iterator it =
          keys.begin(); it != keys.end(); ++it)
    {
      try
      {
        std::cout << it->str() << ":";

        Generics::ConstSmartMemBuf_var mb = read_map->get_profile(*it);

        /*
        PlainStorage::ReadBlock_var rb = it->second->create_reader();
        Generics::ArrayAutoPtr<char> buf(1024);
        PlainStorage::SizeType index_size = rb->read_index_(buf.get(), 1024);
        WriteRecordLayerT::BlockIndex index;
        WriteRecordLayerT::IndexSerializer index_serializer;
        index_serializer.load(buf.get(), index_size, index);

        if(free_indexes.find(index.base_index) != free_indexes.end())
        {
          std::cout << " ERROR this block marked as FREE" << std::endl;
        }
      
        rb->print_(std::cout, "  ");
        */

        sum_used_size += mb->membuf().size();

        if(print_content)
        {
          std::cout << std::endl;
          print_plain_(std::cout, mb->membuf().data(), mb->membuf().size(), "    ");
        }
        else
        {
          std::cout << mb->membuf().size();
        }

        std::cout << std::endl;
      }
      catch(const eh::Exception& ex)
      {
        std::cerr << it->str() << ":" << std::endl <<
          ex.what() << std::endl;
      }
    }

    /* print free indexes */
    std::cout <<
      "number of keys: " << read_map->size() << std::endl <<
      "sum used size: " << sum_used_size << std::endl <<
      "number of free blocks: " << free_indexes.size() << std::endl;

    for(std::set<PlainStorage::FileBlockIndex>::const_iterator free_it =
          free_indexes.begin();
        free_it != free_indexes.end(); ++free_it)
    {
      std::cout << *free_it << std::endl;
    }

    std::cout << "found allocator error: " << allocator_error << std::endl;
  }
}

void
resave_mem_index(const char* source_file, const char* target_file)
{
  PlainStorage::WriteFileLayer_var source_file_layer;
  PlainStorage::WriteFileLayer_var target_file_layer;
  MemIndexMapT_var source_map;
  MemIndexMapT_var target_map;

  {
    // init source map
    source_file_layer =
      new PlainStorage::WriteFileLayer(source_file, ::getpagesize()*16);

    PlainStorage::DefaultAllocatorLayer_var default_allocator(
      new PlainStorage::DefaultAllocatorLayer(source_file_layer));

    WriteFragmentLayer_var fragment_layer(
      new WriteFragmentLayerT(source_file_layer, default_allocator));

    WriteRecordLayer_var record_layer(
      new WriteRecordLayerT(fragment_layer, fragment_layer));

    source_map = new MemIndexMapT(record_layer, record_layer);
  }

  {
    // init target map
    target_file_layer =
      new PlainStorage::WriteFileLayer(target_file, ::getpagesize()*16);

    PlainStorage::DefaultAllocatorLayer_var default_allocator(
      new PlainStorage::DefaultAllocatorLayer(target_file_layer));

    WriteFragmentLayer_var fragment_layer(
      new WriteFragmentLayerT(target_file_layer, default_allocator));

    WriteRecordLayer_var record_layer(
      new WriteRecordLayerT(fragment_layer, fragment_layer));

    target_map = new MemIndexMapT(record_layer, record_layer);
  }

  // copy content
  {
    std::list<BinKey> keys;
    source_map->copy_keys(keys);

    for(std::list<BinKey>::const_iterator it =
          keys.begin(); it != keys.end(); ++it)
    {
      try
      {
        Generics::ConstSmartMemBuf_var mb = source_map->get_profile(*it);
        target_map->save_profile(*it, mb);
      }
      catch(const eh::Exception& ex)
      {
        std::cerr << it->str() << ":" << std::endl <<
          ex.what() << std::endl;
      }
    }
  }
}

void
convert_mem_index_to_level(
  const char* source_base_path,
  const char* source_prefix,
  const char* target_base_path,
  const char* target_prefix,
  unsigned long rw_buffer_size,
  unsigned long rwlevel_max_size,
  unsigned long max_undumped_size,
  unsigned long max_levels0)
{
  PlainStorage::WriteFileLayer_var source_file_layer;
  ReferenceCounting::SmartPtr<AdServer::ProfilingCommons::ProfileMap<BinKey> >
    source_map =
      AdServer::ProfilingCommons::ProfileMapFactory::open_transaction_expire_map<
        BinKey,
        BinKeySerializer>(
        source_base_path,
        source_prefix,
        Generics::Time(1),
        (AdServer::ProfilingCommons::ProfileMapFactory::Cache*)0);

  // copy content
  ReferenceCounting::SmartPtr<
    AdServer::ProfilingCommons::LevelProfileMap<BinKey, BinKeySerializer> >
    target_map(
      new AdServer::ProfilingCommons::LevelProfileMap<BinKey, BinKeySerializer>(
        Generics::ActiveObjectCallback_var(
          new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, "PlainStorageUtil")),
        target_base_path,
        target_prefix,
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          rw_buffer_size,
          rwlevel_max_size,
          max_undumped_size,
          max_levels0,
          Generics::Time::ZERO)));
  target_map->activate_object();

  {
    std::list<BinKey> keys;
    source_map->copy_keys(keys);
    Generics::Time now = Generics::Time::get_time_of_day();

    for(std::list<BinKey>::const_iterator it =
          keys.begin(); it != keys.end(); ++it)
    {
      try
      {
        Generics::ConstSmartMemBuf_var mb = source_map->get_profile(*it);
        target_map->save_profile(*it, mb, now);
      }
      catch(const eh::Exception& ex)
      {
        std::cerr << it->str() << ":" << std::endl <<
          ex.what() << std::endl;
      }
    }
  }

  target_map->deactivate_object();
  target_map->wait_object();
}

void
reset_allocator(const char* target_file)
{
  std::cout << "Reset allocator of '" << target_file << "'" << std::endl;

  PlainStorage::WriteFileLayer_var target_file_layer;

  // init source map
  target_file_layer =
    new PlainStorage::WriteFileLayer(target_file, ::getpagesize()*16);

  {
    PlainStorage::FileBlockIndex max_block_index =
      target_file_layer->max_block_index() + 1;

    std::cout << "New DefaultAllocatorLayer.ControlBlockIndex : " <<
      max_block_index << std::endl;

    PlainStorage::WriteBlock_var control_block =
      target_file_layer->get_write_block(max_block_index);

    FileBlockIndex null_index = 0;
    control_block->write(&null_index, sizeof(null_index));

    std::ostringstream ostr;
    ostr << max_block_index;
    const std::string& str = ostr.str();
    PropertyValue prop(str.c_str(), str.length() + 1);
 
    target_file_layer->set_property(
      "DefaultAllocatorLayer.ControlBlockIndex",
      prop);  
  }

  {
    PlainStorage::FileBlockIndex max_block_index =
      target_file_layer->max_block_index() + 1;

    std::cout << "New WriteFragmentLayer::MainControlBlock : " <<
      max_block_index << std::endl;

    PlainStorage::WriteBlock_var control_block =
      target_file_layer->get_write_block(max_block_index);
    control_block->resize(0);

    WriteFragmentLayerT::NextIndexSerializer next_index_serializer;
    unsigned long sz = next_index_serializer.size(max_block_index);
    Generics::ArrayAutoPtr<unsigned char> buf(sz);
    next_index_serializer.save(max_block_index, buf.get(), sz);
    PropertyValue prop(buf.get(), sz);
    target_file_layer->set_property(
      "WriteFragmentLayer::MainControlBlock",
      prop);
  }
}

int main(int argc, char* argv[]) 
{
  int result = 0;

  try
  {
    Generics::AppUtils::CheckOption opt_help;
    Generics::AppUtils::CheckOption opt_print_content;
    Generics::AppUtils::Option<unsigned long> opt_rw_level_max_size(104857600);
    Generics::AppUtils::Option<unsigned long> opt_max_undumped_size(262144000);
    Generics::AppUtils::Option<unsigned long> max_levels0(20);
    Generics::AppUtils::Args args(-1);

    args.add(
      Generics::AppUtils::equal_name("help") ||
      Generics::AppUtils::short_name("h"),
      opt_help);
    args.add(
      Generics::AppUtils::equal_name("print-content") ||
      Generics::AppUtils::short_name("c"),
      opt_print_content);
    args.add(
      Generics::AppUtils::equal_name("rw-level-max-size"),
      opt_rw_level_max_size);
    args.add(
      Generics::AppUtils::equal_name("max-undumped-size"),
      opt_max_undumped_size);
    args.add(
      Generics::AppUtils::equal_name("max-levels0"),
      max_levels0);

    args.parse(argc - 1, argv + 1);

    const Generics::AppUtils::Args::CommandList& commands = args.commands();

    if(commands.empty() || opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 1;
    }

    std::string command = *commands.begin();

    if(command == "print-info")
    {
      if(commands.size() < 2)
      {
        std::cout << "File not defined. See usage: " << std::endl <<
          USAGE << std::endl;
      }

      std::string file = *++commands.begin();
      print_block_links(std::cout, file.c_str(), opt_print_content.enabled());
    }
    else if(command == "resave-mem-index")
    {
      if(commands.size() < 3)
      {
        std::cout << "source and target file not defined. See usage: " << std::endl <<
          USAGE << std::endl;
      }

      Generics::AppUtils::Args::CommandList::const_iterator cit =
        ++commands.begin();
      std::string source_file = *cit++;
      std::string target_file = *cit;

      resave_mem_index(
        source_file.c_str(),
        target_file.c_str());
    }
    else if(command == "reset-allocator")
    {
      if(commands.size() < 2)
      {
        std::cout << "target file not defined. See usage: " << std::endl <<
          USAGE << std::endl;
      }

      Generics::AppUtils::Args::CommandList::const_iterator cit =
        ++commands.begin();
      std::string target_file = *cit++;
      reset_allocator(target_file.c_str());
    }
    else if(command == "convert-mem-to-level")
    {
      if(commands.size() < 2)
      {
        std::cout << "target file not defined. See usage: " << std::endl <<
          USAGE << std::endl;
      }
      
      Generics::AppUtils::Args::CommandList::const_iterator cit =
        commands.begin();
      std::string source_base_path = *++cit;
      std::string source_prefix = *++cit;
      std::string target_base_path = *++cit;
      std::string target_prefix = *++cit;
      convert_mem_index_to_level(
        source_base_path.c_str(),
        source_prefix.c_str(),
        target_base_path.c_str(),
        target_prefix.c_str(),
        10*1024*1024,
        *opt_rw_level_max_size,
        *opt_max_undumped_size,
        *max_levels0);
    }
    else
    {
      std::cerr << "unknown command" << std::endl;
      result = -1;
    }
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    result = -1;
  }

  return result;
}
