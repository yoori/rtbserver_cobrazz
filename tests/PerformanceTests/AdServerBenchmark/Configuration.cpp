
#include "Configuration.hpp"

// class BenchmarkConfig

BenchmarkConfig::BenchmarkConfig(
  const char* server_,
  unsigned long size_,
  ClientType client_type_,
  bool initial_,
  const char* description_,
  FrontendType frontend_type_,
  RequestConfig* config_) :
  server(server_),
  size(size_),
  client_type(client_type_),
  initial(initial_),
  description(description_),
  frontend_type(frontend_type_),
  config(RequestConfig_var(config_))
{
}

BenchmarkConfig::~BenchmarkConfig() noexcept
{
}

// class Configuration
const char* Configuration::URL = "referer";
const char* Configuration::REFERER_KW = "referer-kw";
const char* Configuration::FT         = "ft";
const char* Configuration::VERSION    = "version";
const char* Configuration::BROWSER    = "browser";
const char* Configuration::USER_AGENT = "user_agent";
const char* Configuration::OS = "os";
const char* Configuration::TID = "tid";
const char* Configuration::COUNTRY = "country";
const char* Configuration::FORMAT = "format";
const char* Configuration::CT_PSWND_COOKIE = "ct_pswnd_cookie";
const char* Configuration::SIZENOTSUP = "sizenotsup";
const char* Configuration::COLO_ID = "colo_id";


Configuration::Configuration(const char* config_path) :
  config_path_(config_path),
  threads_number_(DEFAULT_THREADS_NUMBER)
{
}

Configuration::~Configuration()
{
}

void Configuration::read() /*throw(InvalidConfiguration)*/
{
  Config::ErrorHandler error_handler;

  std::unique_ptr<AdServerBenchmarkType> configuration;
  
  try
  {
    configuration = Test(config_path_, error_handler);
    
    if(error_handler.has_errors())
    {
      std::string error_string;
      throw InvalidConfiguration(error_handler.text(error_string));
    }
    
    // read global config
    const GlobalSettingsType& global_config = configuration->GlobalSettings();
    
    // 1. global params
    threads_number_ = global_config.ThreadsNumber();
    std::string server_url = global_config.ServerURL();
    
    // 2. data lists for making requests
    if (global_config.URLsFile().present())
    {
      request_params_[URL] =
        ConfigList_var(new ConfigFileList(global_config.URLsFile().get().c_str(),
                                            config_path_.c_str()));
    }

    if (global_config.RefererKWsFile().present())
    {
      request_params_[REFERER_KW] =
        ConfigList_var(new ConfigFileList(global_config.RefererKWsFile().get().c_str(),
                                            config_path_.c_str()));
    }

    if (global_config.ftPath().present())
    {
      request_params_[FT] =
        ConfigList_var(new ConfigDirFilesList(global_config.ftPath().get().c_str(),
            "ft",
            config_path_.c_str()));
    }

    GlobalSettingsType::Set_const_iterator sit = global_config.Set().begin();
    GlobalSettingsType::Set_const_iterator send = global_config.Set().end();
    for (; sit != send; ++sit)
    {
      request_params_[sit->name()] =
        ConfigList_var(new ConfigXmlList(sit->item()));
    }

    BenchmarksType::Benchmark_const_iterator
       bit = configuration->Benchmarks().Benchmark().begin();
    BenchmarksType::Benchmark_const_iterator
       bend = configuration->Benchmarks().Benchmark().end();
    for (; bit != bend; ++bit)
    {
      std::string benchmark_server_url = server_url;

      if (bit->server_url().present())
      {
        benchmark_server_url = *bit->server_url();
      }

      benchmarks_.push_back(
        BenchmarkConfig_var(
            new BenchmarkConfig(benchmark_server_url.c_str(),
                                bit->size(),
                                bit->type(),
                                bit->initial(),
                                bit->description().c_str(),
                                bit->frontend(),
                                new ParamsRequestConfig(0,
                                                        bit->request(),
                                                        request_params_)
                                )));
    }

  }
  catch (const SelectorPolicy::InvalidConfigRequestData& e)
  {
    Stream::Error ostr;
    ostr << "Can't create request params: " << e.what();
    throw InvalidConfiguration(ostr);
  }
  catch (const ConfigListErrors::InvalidList& e)
  {
    Stream::Error ostr;
    ostr << "Can't read config list: " << e.what();
    throw InvalidConfiguration(ostr);
  }
  catch (const xml_schema::parsing& e)
  {
    Stream::Error ostr;
    ostr << "Can't parse config file '"
         << config_path_ << "'."
         << ": ";
    if(error_handler.has_errors())
    {
      std::string error_string;
      ostr << error_handler.text(error_string);
    }
    throw InvalidConfiguration(ostr);
  }
}

unsigned long Configuration::threads_number() const
{
  return threads_number_;
}

const Configuration::BenchmarkList&
Configuration::benchmarks() const
{
  return  benchmarks_;
}
