#include "MultiLangSearchRefererParsingTest.hpp"
#include <String/InterConvertion.hpp>

REFLECT_UNIT(MultiLangSearchRefererParsingTest) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

namespace 
{
  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  const char query_marker[] = "%%QUERY%%";

  struct ConvPair
  {
    const char* channel;
    const char* encoding;
  };

  const ConvPair INVALID_CONVERTINGS[] =
  {
    // Korean channel
    { "Channel/01", "koi8-r" },
    { "Channel/01", "iso-8859-1" },
    { "Channel/01", "gb2312" },
    // Russian channel
    { "Channel/03", "iso-8859-1" },
    { "Channel/03", "gb2312" }
  };

  bool 
  valid_converting(
    const std::string& channel,
    const std::string& encoding)
  {
    for (size_t i = 0; i < countof(INVALID_CONVERTINGS); ++i)
    {
      if (channel == INVALID_CONVERTINGS[i].channel &&
        encoding == INVALID_CONVERTINGS[i].encoding)
      {
        return false;
      }
    }
    return true;
  };
}

                             
bool 
MultiLangSearchRefererParsingTest::run_test()
{
  add_descr_phrase("Starting <https://jira.ocslab.com/browse/ADSC-224>");
  
  String::International::Convertion converter;
  std::string sbuf;

  bool pass = false;
  
  DataElemObjectPtr search_engine;
  while (next_list_item(search_engine, "Search Engines"))
  {
    DataElemObjectPtr channel;
    while (next_list_item(channel, "Channels"))
    {      
      sbuf.clear();
      add_descr_phrase(((search_engine->Name() + ", ") + 
                        channel->Name() + " are gathered").c_str());
      
      //mime-encoding of trigger word + pre-encoding in original code if needed
      if (search_engine->Description() != "utf-8" &&
        valid_converting(channel->Name(), search_engine->Description()))
      {
        try
        {
          converter.set_encodings(search_engine->Description().c_str(), "utf-8");

          converter.encode(channel->Description().c_str(),
                           channel->Description().length(), sbuf);
        }
        catch (eh::Exception& e)
        {
          Stream::Error error;
          error << "Can't convert characters utf8 char sequence '" <<
            channel->Description() << "' to " + search_engine->Description() <<
            ": " << e.what();
          throw Exception(error);
        }
        pass = true;
        {
          std::string buf_buf = sbuf;
          String::StringManip::mime_url_encode(buf_buf, sbuf);
        }
      }
      else
      {
        String::StringManip::mime_url_encode(channel->Description(), sbuf);

        // Double encoding to implement 'ajax' behaviour
        if (search_engine->Name() == "ajax")
        {
          std::string buf_buf = sbuf;
          String::StringManip::mime_url_encode(buf_buf, sbuf);
        }

      }


      //adding trigger word instead of marker <query_marker> or adding
      //trigger word to the end if <query_marker> doesn't exist
      std::string full_url = search_engine->Value();
      std::string::size_type start = 0;
      bool replaced = false;
      while ((start = full_url.find(query_marker)) != std::string::npos)
      {
        full_url.replace(start, sizeof(query_marker) - 1, sbuf.c_str());
        replaced = true;
      }
      if (!replaced)
      {
        full_url += sbuf;
      }
              
      AdClient client(AdClient::create_user(this));

      client.process_request(NSLookupRequest().tid(tid).referer(full_url), sbuf.c_str());
      
      FAIL_CONTEXT(
        AutoTest::entry_checker(
          channel->Value(), 
          client.debug_info.trigger_channels +
            client.debug_info.trigger_channels).check(), 
        "must have channel");                                
    }
  }

  FAIL_CONTEXT(
    AutoTest::predicate_checker(pass),
    "Not a single encoding convertion is success in the test.");  
  
  return true;
}

