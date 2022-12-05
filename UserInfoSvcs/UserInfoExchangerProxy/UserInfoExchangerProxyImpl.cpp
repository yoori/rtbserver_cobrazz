#include <list>
#include <vector>
#include <iterator>
#include <time.h>

#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>

#include "UserInfoExchangerProxyImpl.hpp"

namespace Aspect
{
  char USER_INFO_EXCANGER_PROXY[] = "UserInfoExchangerProxy";
}

namespace AdServer{
namespace UserInfoSvcs{

  /**
   * UserInfoExchangerProxyImpl
   */

  UserInfoExchangerProxyImpl::UserInfoExchangerProxyImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const UserInfoExchangerProxyConfig& user_info_exchanger_proxy_config)
    /*throw(Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger))
  {
    try
    {
      CORBACommons::CorbaObjectRef uie_obj_ref;
      Config::CorbaConfigReader::read_corba_ref(
          user_info_exchanger_proxy_config.UserInfoExchangerRef(),
          uie_obj_ref);

      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();
      CORBA::Object_var obj =
        corba_client_adapter_->resolve_object(uie_obj_ref);
      if(CORBA::is_nil(obj.in()))
      {
        throw Exception("resolve_object return nil reference");
      }
      uie_ref_ = AdServer::UserInfoSvcs::UserInfoExchanger::_narrow(obj.in());
      if(CORBA::is_nil(uie_ref_.in()))
      {
        throw Exception("_narrow return nil reference");
      }
    }
    catch(const Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangerProxyImpl::UserInfoExchangerProxyImpl(): "
        "Can't instantiate object. Caught Exception. "
        ": "
        << ex.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangerProxyImpl::UserInfoExchangerProxyImpl(): "
              "Can't instantiate object. Caught eh::Exception. "
              ": "
           << ex.what();
      throw Exception(ostr);
    }
  }

  UserInfoExchangerProxyImpl::~UserInfoExchangerProxyImpl() noexcept
  {
  }

  /* UserInfoExchange corba interface implementation */
  void
  UserInfoExchangerProxyImpl::register_users_request(
    const char* customer_id,
    const AdServer::UserInfoSvcs::ColoUsersRequestSeq& colo_users)
    /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
          AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/
  {
    try
    {
      uie_ref_->register_users_request(customer_id, colo_users);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;

      ostr << "UserInfoExchangerProxyImpl::register_users_request(): "
        << "Caught eh::Exception. : " << ex.what()
        << " Input parameters: provider_id = '" << customer_id << "'";

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCANGER_PROXY,
                   "ADS-IMPL-67");

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;

      ostr << "UserInfoExchangerProxyImpl::register_users_request(): "
        << "Caught CORBA::SystemException. : " << ex
        << " Input parameters: provider_id = '" << customer_id << "'";

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCANGER_PROXY,
                   "ADS-IMPL-67");

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
  }

  void
  UserInfoExchangerProxyImpl::receive_users(
    const char* customer_id,
    AdServer::UserInfoSvcs::UserProfileSeq_out user_profiles_out,
    const AdServer::UserInfoSvcs::ReceiveCriteria& receive_criteria)
    /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
          AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/
  {
    try
    {
      uie_ref_->receive_users(
        customer_id,
        user_profiles_out,
        receive_criteria);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;

      ostr << "UserInfoExchangerProxyImpl::receive_users(): "
        << "Caught eh::Exception. : " << ex.what()
        << " Input parameters: customer_id = '" << customer_id << "'"
        << " max_response_plain_size = "
        << receive_criteria.max_response_plain_size;

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCANGER_PROXY,
                   "ADS-IMPL-68");

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;

      ostr << "UserInfoExchangerProxyImpl::receive_users(): "
        << "Caught CORBA::SystemException. : " << ex
        << " Input parameters: customer_id = '" << customer_id << "'"
        << " max_response_plain_size = "
        << receive_criteria.max_response_plain_size;

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCANGER_PROXY,
                   "ADS-IMPL-68");

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
  }

  void
  UserInfoExchangerProxyImpl::get_users_requests(
    const char* provider_id,
    AdServer::UserInfoSvcs::UserIdSeq_out user_ids_out)
    /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
          AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/
  {
    try
    {
      uie_ref_->get_users_requests(provider_id, user_ids_out);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr
        << "UserInfoExchangerProxyImpl::get_users_requests(): "
        << "Caught eh::Exception. : " << ex.what()
        << "Input parameters: provider_id = '" << provider_id << "'";

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCANGER_PROXY,
                   "ADS-IMPL-69");

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;

      ostr
        << "UserInfoExchangerProxyImpl::get_users_requests(): "
        << "Caught CORBA::SystemException. : " << ex
        << " Input parameters: provider_id = '" << provider_id << "'";

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCANGER_PROXY,
                   "ADS-IMPL-69");

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
  }

  void
  UserInfoExchangerProxyImpl::send_users(
    const char* provider_id,
    const AdServer::UserInfoSvcs::UserProfileSeq& user_profiles_in)
    /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
          AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/
  {
    try
    {
      uie_ref_->send_users(provider_id, user_profiles_in);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;

      ostr << "UserInfoExchangerProxyImpl::send_users(): "
        "Caught eh::Exception. : " << ex.what()
        << " Input Parameters: provider_id = '" << provider_id << "'"
        << " users: ";

      for(CORBA::ULong i = 0; i < user_profiles_in.length(); ++i)
      {
        ostr << "    " << user_profiles_in[i].user_id;
      }

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCANGER_PROXY,
                   "ADS-IMPL-70");

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;

      ostr << "UserInfoExchangerProxyImpl::send_users(...). "
        "Caught CORBA::SystemException. : " << ex
        << " Input parameters: provider_id = '" << provider_id << "'"
        << "  users: ";

      for(CORBA::ULong i = 0; i < user_profiles_in.length(); ++i)
      {
        ostr << "    " << user_profiles_in[i].user_id;
      }

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCANGER_PROXY,
                   "ADS-IMPL-70");

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
  }

} /*UserInfoSvcs*/
} /*AdServer*/

