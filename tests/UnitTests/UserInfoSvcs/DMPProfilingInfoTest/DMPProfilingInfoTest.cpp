#include <string>
#include <iostream>
#include <iomanip>

#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>
#include <Commons/Algs.hpp>
#include <Frontends/ProfilingServer/DMPProfilingInfo.hpp>

using namespace AdServer::Profiling;

void
print_hex(
  std::ostream& ostr,
  const void* buf,
  unsigned long size,
  const char* prefix)
{
  for(unsigned long i = 0; i < size; ++i)
  {
    ostr << "0x" << std::hex << std::setfill('0') << std::setw(2) <<
      (int)*((const unsigned char*)buf + i) << " ";

    if(i && (i + 1) % 16 == 0)
    {
      ostr << std::endl << prefix;
    }
  }
}

void
p1()
{
  DMPProfilingInfoWriter writer;
  writer.version() = 1;
  writer.time() = 2;
  writer.source() = "bln";
  writer.external_user_id() = "TEST";
  writer.url() = "http://e.com";
  writer.bind_user_ids() = "xaa";
  writer.longitude() = 111;
  writer.latitude() = 222;

  unsigned long size = writer.size();
  std::vector<unsigned char> buf_arr(size);
  writer.save(buf_arr.data(), size);

  const unsigned char* buf = buf_arr.data();
  print_hex(std::cout, buf, size, "  ");
}

void
p2()
{
  const uint32_t time = 2;
  const std::string source = "bln";
  const std::string user_id = "TEST";
  const std::string url = "http://e.com";
  const std::string bind_key = "xaa";

  uint32_t MSG_HEAD[9] = {
    1, // version
    static_cast<uint32_t>(time),
    sizeof(MSG_HEAD) - 8, // source offset
    static_cast<uint32_t>(sizeof(MSG_HEAD) - 12 + source.size() + 1), // external_user_id offset
    static_cast<uint32_t>(
      sizeof(MSG_HEAD) - 16 + source.size() + 1 + user_id.size() + 1), // bind_user_ids offset
    static_cast<uint32_t>(
      sizeof(MSG_HEAD) - 20 + source.size() + 1 + user_id.size() + 1 + bind_key.size() + 1), // url offset
    static_cast<uint32_t>(
      sizeof(MSG_HEAD) - 24 + source.size() + 1 + user_id.size() + 1 + bind_key.size() + 1 + url.size() + 1),
      // keywords offset
    111,
    222
  };

  std::vector<unsigned char> message(
    sizeof(MSG_HEAD) +
    source.size() + 1 + // source
    user_id.size() + 1 + // external_user_id
    bind_key.size() + 1 + // bind_user_ids
    url.size() + 1 + // url
    1, // keywords
    0xFA);

  unsigned char* buf = message.data();
  ::memcpy(buf, MSG_HEAD, sizeof(MSG_HEAD));
  buf += sizeof(MSG_HEAD);

  // source
  ::memcpy(buf, "bln", 4);
  buf += 4;

  // external_user_id
  ::memcpy(buf, user_id.data(), user_id.size());
  buf += user_id.size();
  *buf = 0;
  ++buf;

  // bind_user_ids
  ::memcpy(buf, bind_key.data(), bind_key.size());
  buf += bind_key.size();
  *buf = 0;
  ++buf;

  // url
  ::memcpy(buf, url.data(), url.size());
  buf += url.size();
  *buf = 0;
  ++buf;

  // keywords
  *buf = 0;
  ++buf;

  std::cout << std::dec << "l: " << (buf - message.data()) << std::endl;
  std::cout << "r: " << (sizeof(MSG_HEAD) + source.size() + 1 + user_id.size() + 1 + 1 + url.size() + 1 + 1) << std::endl;

  print_hex(std::cout, message.data(), message.size(), "  ");

  assert(static_cast<unsigned long>(buf - message.data()) ==
    sizeof(MSG_HEAD) + source.size() + 1 + user_id.size() + 1 + bind_key.size() + 1 + url.size() + 1 + 1);
}

int
main(int /*argc*/, char** /*argv*/)
  noexcept
{
  int ret = 0;
  try
  {
    p1();
    std::cout << "============" << std::endl;
    p2();

    return ret;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return -1;
}
