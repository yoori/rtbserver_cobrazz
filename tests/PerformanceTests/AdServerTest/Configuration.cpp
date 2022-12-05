
#include "Configuration.hpp"
#include <Stream/MemoryStream.hpp>

// Utils

template <class T> ConstraintConfig* ref_2_constraint(const T& constraint_ref)
{
  ConstraintConfig* constraint(0);
  if (constraint_ref.ref().present())
    {
      typedef typename T::ref_optional constraint_opt;
      const constraint_opt& req_ref ( constraint_ref.ref() );
      if (req_ref.present())
        {
          const requestConstraintType&  req_constraint(*req_ref.get());
          constraint = new ConstraintConfig(req_constraint);
        }
    }
  return constraint;
};


// OptOutRequestConfig class
OptOutRequestConfig::OptOutRequestConfig(ConstraintConfig* _constraint,
                                         const char* _url) :
  RequestConfig(_constraint, _url)
{}


// ClientConfig class
ClientConfig::ClientConfig(const AdClientType& client_config,
                           const RequestLists& request_lists) /*throw(SelectorPolicy::InvalidConfigRequestData)*/ :
  count(client_config.count().get()),
  lifetime(client_config.lifetime().get()),
  optout_rate(client_config.optout_rate().get()),
  action_rate(client_config.action_rate().get()),
  click_rate(client_config.click_rate().get()),
  passback_rate(client_config.passback_rate().get()),
  incr_count_step(client_config.incr_count_step()),
  step_interval(client_config.step_interval()),
  ad_all_optouts(client_config.ad_all_optouts()),
  ns_request_(),
  action_request_(),
  click_request_(),
  passback_request_(),
  optout_request_()
{

  // Ns-lookup request
  if (client_config.NSLookup().present())
  {
    const ClientRequestType& ns_request_config = client_config.NSLookup().get();
    const client_constraint& constraint_ref(ns_request_config.requestConstraint());
    ns_request_ =
        RequestConfig_var(new ParamsRequestConfig(ref_2_constraint(constraint_ref),
                                                  ns_request_config.request(),
                                                  request_lists));
  }

  // Click request
  if (client_config.Click().present())
  {
    const GeneratedRequestType& click_config = client_config.Click().get();
    const generated_request_constraint& constraint_ref(click_config.requestConstraint());
    click_request_ =
        RequestConfig_var(new RequestConfig(ref_2_constraint(constraint_ref)));
  }

  // Action request
  if (client_config.ActionTracking().present())
  {
    const GeneratedRequestType& action_config = client_config.ActionTracking().get();
    const generated_request_constraint& constraint_ref(action_config.requestConstraint());
    action_request_ =
        RequestConfig_var(new RequestConfig(ref_2_constraint(constraint_ref)));
  }

  // Passback request
  if (client_config.Passback().present())
  {
    const GeneratedRequestType& passback_config = client_config.Passback().get();
    const generated_request_constraint& constraint_ref(passback_config.requestConstraint());
    passback_request_ =
        RequestConfig_var(new RequestConfig(ref_2_constraint(constraint_ref)));
  }

  // OptOut request
  if (client_config.OptOut().present())
  {
    const OptOutType& optout_config = client_config.OptOut().get();
    const optout_constraint& constraint_ref(optout_config.requestConstraint());
    std::string url = optout_config.url().get();
    optout_request_ =
        RequestConfig_var(new OptOutRequestConfig(ref_2_constraint(constraint_ref),
                                                  url.c_str()));
  }
}

const RequestConfig_var& ClientConfig::ns_request() const
{
  return ns_request_;
}

const RequestConfig_var& ClientConfig::action_request() const
{
  return action_request_;
}

const RequestConfig_var& ClientConfig::click_request() const
{
  return click_request_;
}

const RequestConfig_var& ClientConfig::passback_request() const
{
  return passback_request_;
}

const RequestConfig_var& ClientConfig::optout_request() const
{
  return optout_request_;
}

// Configuration class

const char* Configuration::EXECUTION_TIME = "executionTime";
const char* Configuration::THREADS_NUMBER = "threadsNumber";

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


void Configuration::read() /*throw(InvalidConfiguration)*/
{
  Config::ErrorHandler error_handler;

  std::unique_ptr<testParamsType> configuration;

  try
    {
      configuration = testParams(config_path_, error_handler);

      if(error_handler.has_errors())
        {
          std::string error_string;
          throw InvalidConfiguration(error_handler.text(error_string));
        }


      description_ = configuration->description();

      // read global config
      const GlobalTestingOptions& global_config = configuration->global();

      if (global_config.confluenceReportPath().present())
      {
        confluence_report_path_ = global_config.confluenceReportPath().get();

      }

      // 1. global params
      execution_time_ = global_config.executionTime();
      threads_number_ = global_config.threadsNumber();
      server_urls_   = ConfigList_var(new ConfigXmlList(global_config.serverBase().url()));

      statistics_interval_dump_ = global_config.requestAggregatedStatistics()->statisticsIntervalDump();

      // 2. data lists for making requests
      request_params_[URL] =
        ConfigList_var(new ConfigFileList(global_config.urlsListFile().c_str(),
                                          config_path_.c_str()));
      request_params_[REFERER_KW] =
        ConfigList_var(new ConfigFileList(global_config.RefererKWsListFile().c_str(),
                                          config_path_.c_str()));

      if (global_config.ftPath().present())
      {
        request_params_[FT] =
            ConfigList_var(new ConfigDirFilesList(global_config.ftPath().get().c_str(),
                                                  "ft",
                                                  config_path_.c_str()));
      }


      unsigned long sets_size = global_config.set().size();
      for (unsigned long i = 0; i < sets_size; i++)
        {
          std::string name      = global_config.set()[i].name();
          ConfigList_var cfg_var(new ConfigXmlList(global_config.set()[i].item()));
          request_params_[name] = cfg_var;
        }

      // client config
      const AdClientType& client_config = configuration->adClient();

      client_config_ = ClientConfig_var(new ClientConfig(client_config, request_params_));

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

const ConfigList_var& Configuration::get_list_by_name(const char* name) /*throw(InvalidConfigName)*/
{
  if (request_params_.find(name) == request_params_.end())
    {
      Stream::Error ostr;
      ostr << "List '" << name << "' not found in config file '" << config_path_ << "'";
      throw InvalidConfigName(ostr);
    }
  return request_params_[name];
}

const std::string& Configuration::description() const
{
  return description_;
}

const std::string& Configuration::confluence_report_path() const
{
  return confluence_report_path_;
}

unsigned long Configuration::threads_number() const
{
  return threads_number_;
}

unsigned long Configuration::execution_time() const
{
  return execution_time_;
}

const ConfigList_var& Configuration::server_urls() const
{
  return server_urls_;
}

const ClientConfig_var& Configuration::client_config() const
{
  return client_config_;
}

unsigned long Configuration::statistics_interval_dump() const
{
  return statistics_interval_dump_;
}

