#include <sys/socket.h>
#include <netdb.h>
#include <list>
#include <vector>
#include <deque>
#include <iterator>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <Generics/AppUtils.hpp>
#include <Generics/BitAlgs.hpp>
#include <String/Tokenizer.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>
#include <Frontends/UIDGeneratorAdapter/UIDGeneratorProtocol.pb.h>

#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "\nUsage: \n"
    "UIDGeneratorUtil generate-request\n"
    "UIDGeneratorUtil send\n";
}

// Application
Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

typedef const String::AsciiStringManip::Char2Category<',', '|'>
  ListSepType;

typedef const String::AsciiStringManip::Char1Category<':'>
  SubListSepType;

void
Application_::send_(
  const String::SubString& socket_str,
  const Generics::Time& timeout,
  unsigned long max_portion)
  noexcept
{
  const String::SubString::SizeType sep_pos = socket_str.find(':');
  if(sep_pos != String::SubString::NPOS)
  {
    unsigned long port;
    String::SubString host_str = socket_str.substr(0, sep_pos);
    String::SubString port_str = socket_str.substr(sep_pos + 1);

    if(!String::StringManip::str_to_int(port_str, port))
    {
      Stream::Error ostr;
      ostr << "invalid port '" << port_str << "'";
      throw Exception(ostr);
    }

    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    struct hostent* server = ::gethostbyname(host_str.str().c_str());
    struct sockaddr_in serv_addr;
    ::bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    ::bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if(::connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
      Stream::Error ostr;
      ostr << "Can't connect to '" << socket_str << "'";
      throw Exception(ostr);
    }

    std::deque<Generics::Time> times;

    std::vector<unsigned char> responses;
    const int RESPONSE_SIZE = 8;
    unsigned long total_sent = 0;

    std::cout << "to sending loop" << std::endl;

    while(true)
    {
      int portion = ::rand() % max_portion + 1;
      Generics::Time start_time = Generics::Time::get_time_of_day();

      for(int i = 0; i < portion; ++i)
      {
        std::vector<unsigned char> buf;
        std::string ids_str(::rand() % 1000, 'X');
        generate_request_buf_(buf, ids_str, String::SubString(), String::SubString());
        times.push_back(Generics::Time::get_time_of_day());
        int write_sz = ::write(sockfd, buf.data(), buf.size());
        if(write_sz < 0)
        {
          Stream::Error ostr;
          ostr << "write error";
          throw Exception(ostr);
        }
      }

      std::cout << "to read " << portion << " responses" <<
        std::endl;

      // reading loop
      while(true)
      {
        unsigned char BUF[1024];
        int read_sz = ::read(sockfd, BUF, sizeof(BUF));
        if(read_sz < 0)
        {
          Stream::Error ostr;
          ostr << "read error";
          throw Exception(ostr);
        }

        //std::cerr << "read_sz: " << read_sz << std::endl;
        responses.insert(responses.end(), BUF, BUF + read_sz);
        const int got_responses = responses.size() / RESPONSE_SIZE;
        const int tail_size = responses.size() - got_responses * RESPONSE_SIZE;
        std::vector<unsigned char> new_responses(
          responses.begin() + responses.size() - tail_size,
          responses.end());
        responses.swap(new_responses);

        Generics::Time now = Generics::Time::get_time_of_day();

        //std::cerr << "got_responses: " << got_responses << ", times: " << times.size() << std::endl;

        for(int i = 0; i < got_responses; ++i)
        {
          assert(!times.empty());
          Generics::Time val = times.front();
          times.pop_front();

          if(now - val > timeout)
          {
            Stream::Error ostr;
            ostr << now.gm_ft() << "." << std::setfill('0') << std::setw(3) << (now.tv_usec / 1000) <<
              "> timeout reached: " << (now - val);
            throw Exception(ostr);
          }
        }

        if(times.empty())
        {
          break;
        }
      }

      Generics::Time end_time = Generics::Time::get_time_of_day();
      const Generics::Time divider = end_time - start_time;

      const double delta_time_ms =
        1000.0 * divider.tv_sec + divider.tv_usec / 1000.0;

      total_sent += portion;

      std::cout << "sent " << total_sent << " requests, rate = " <<
        std::fixed << std::setprecision(3) << (1000.0 * portion / delta_time_ms) <<
        std::endl;
      //std::cout << "from reading loop: " << times.size() << ", responses.size = " << responses.size() << std::endl;
    }

    std::cout << "from sending loop" << std::endl;
  }
  else
  {
    Stream::Error ostr;
    ostr << "invalid socket ref '" << socket_str << "'";
    throw Exception(ostr);
  }
}

void
Application_::generate_request_(
  std::ostream& out,
  const String::SubString& ids_str,
  const String::SubString& buckets_str,
  const String::SubString& obuckets_str)
  noexcept
{
  std::vector<unsigned char> buf;
  generate_request_buf_(buf, ids_str, buckets_str, obuckets_str);
  out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
}

void
Application_::generate_request_buf_(
  std::vector<unsigned char>& buf,
  const String::SubString& ids_str,
  const String::SubString& buckets_str,
  const String::SubString& obuckets_str)
  noexcept
{
  ru::madnet::enrichment::protocol::DmpRequest request;

  {
    String::StringManip::Splitter<ListSepType> tokenizer(ids_str);
    String::SubString token;
    while(tokenizer.get_token(token))
    {
      ru::madnet::enrichment::protocol::Identifier* ids = request.add_ids();

      String::StringManip::Splitter<SubListSepType> sub_tokenizer(token);

      String::SubString value;
      sub_tokenizer.get_token(value);
      ids->set_value(value.str());

      String::SubString type;
      sub_tokenizer.get_token(type);
      ids->set_type(type.str());

      String::SubString source;
      sub_tokenizer.get_token(source);
      ids->set_source(source.str());
    }
  }

  ru::madnet::enrichment::protocol::Data* data = request.mutable_data();

  {
    String::StringManip::Splitter<ListSepType> tokenizer(buckets_str);
    String::SubString token;
    while(tokenizer.get_token(token))
    {
      ru::madnet::enrichment::protocol::Bucket* bucket = data->add_buckets();
      bucket->set_name(token.str());
    }
  }

  {
    String::StringManip::Splitter<ListSepType> tokenizer(obuckets_str);
    String::SubString token;
    while(tokenizer.get_token(token))
    {
      ru::madnet::enrichment::protocol::Bucket* obucket = data->add_obuckets();
      obucket->set_name(token.str());
    }
  }

  // write little endian size
  std::string strbuf;
  strbuf.reserve(10*1024);
  request.SerializeToString(&strbuf);

  uint32_t sz = strbuf.size();
  buf.resize(sz + 4);
  ::memcpy(&buf[0], &sz, sizeof(sz));
  ::memcpy(reinterpret_cast<unsigned char*>(&buf[0]) + 4, strbuf.data(), strbuf.size());
}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::StringOption opt_ids("");
  Generics::AppUtils::StringOption opt_buckets("");
  Generics::AppUtils::StringOption opt_obuckets("");
  Generics::AppUtils::StringOption opt_socket("");
  Generics::AppUtils::Option<unsigned long> opt_timeout(100);
  Generics::AppUtils::Option<unsigned long> opt_max_portion(100);
  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);
  args.add(
    Generics::AppUtils::equal_name("ids"),
    opt_ids);
  args.add(
    Generics::AppUtils::equal_name("buckets"),
    opt_buckets);
  args.add(
    Generics::AppUtils::equal_name("obuckets"),
    opt_obuckets);
  args.add(
    Generics::AppUtils::equal_name("socket"),
    opt_socket);
  args.add(
    Generics::AppUtils::equal_name("timeout"),
    opt_timeout);
  args.add(
    Generics::AppUtils::equal_name("max-portion"),
    opt_max_portion);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  std::string command = *commands.begin();

  try
  {
    if(command == "generate-request")
    {
      generate_request_(
        std::cout,
        *opt_ids,
        *opt_buckets,
        *opt_obuckets);
    }
    else if(command == "send")
    {
      send_(
        *opt_socket,
        Generics::Time(*opt_timeout) / 1000,
        *opt_max_portion);
    }
    else
    {
      Stream::Error ostr;
      ostr << "unknown command '" << command << "', "
        "see help for more info" << std::endl;
      throw Exception(ostr);
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
}

int main(int argc, char** argv)
{
  Application_* app = 0;

  try
  {
    app = &Application::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  assert(app);

  try
  {
    app->main(argc, argv);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}


