#ifndef _USER_INFO_SVCS_USER_INFO_CONTAINER_TEST_APPLICATION_HPP_
#define _USER_INFO_SVCS_USER_INFO_CONTAINER_TEST_APPLICATION_HPP_

#include <map>
#include <set>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Singleton.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Logger/FileLogger.hpp>
#include <Logger/StreamLogger.hpp>

#include "UserInfoContainerTest.hpp"

const unsigned long SESSION_TIMEOUT = 43200;

typedef std::map<std::string, std::string> Args;

typedef std::unique_ptr<
  xsd::AdServer::Configuration::TestConfigurationType> TestConfigPtr;
typedef std::unique_ptr<
  xsd::AdServer::Configuration::UserInfoContainerTestConfigType> UICConfigPtr;
typedef std::unique_ptr<
  xsd::AdServer::Configuration::ChannelRulesType> ChannelRulesConfigPtr;

using namespace AdServer::UserInfoSvcs;

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  Application_() noexcept;

  virtual ~Application_() noexcept;

  void main(int& argc, char** argv) noexcept;

protected:
  void parse_args(unsigned long count, char** argv, Args& args)
    /*throw(eh::Exception)*/;

  static void find_required_argument(
    const Args& args,
    const char* argument_name,
    std::string& argument_value) /*throw(eh::Exception)*/;

  Logging::Logger_var logger_;
  UICConfigPtr uic_config_;
  ChannelRulesConfigPtr channels_config_;

//  xsd::AdServer::Configuration::ChannelRulesType ch_config_;
  
  
};

typedef Generics::Singleton<Application_> Application;

#endif 
