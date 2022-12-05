#include "ChannelServerSessionPool.hpp"

namespace FrontendCommons
{
  ChannelServerSessionPool::ChannelServerSessionPoolConfig::
  Resolver::Resolver(
    const CORBACommons::CorbaClientAdapter* corba_client_adapter)
    noexcept
    : corba_client_adapter_(ReferenceCounting::add_ref(corba_client_adapter))
  {}

  template<typename PoolType>
  PoolType*
  ChannelServerSessionPool::ChannelServerSessionPoolConfig::
  Resolver::resolve(
    const ObjectRef& ref)
    /*throw(Exception)*/
  {
    static const char* FUN = "ChannelServerSessionPoolConfig::Resolver::resolve()";

    try
    {
      CORBA::Object_var obj = corba_client_adapter_->resolve_object(ref);

      AdServer::ChannelSvcs::ChannelManagerController_var controller =
        AdServer::ChannelSvcs::ChannelManagerController::_narrow(obj);
      if(CORBA::is_nil(controller))
      {
        return 0;
      }

      AdServer::ChannelSvcs::ChannelServerSession_var channel_session =
        controller->get_channel_session();

      return AdServer::ChannelSvcs::ChannelServerSession::_narrow(
        channel_session);
    }
    catch (const AdServer::ChannelSvcs::ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve channel manager controller reference '" <<
        ref.object_ref << "'. AdServer::ChannelSvcs::"
        "ImplementationException caught: " <<
        ex.description;
      throw Exception(ostr);
    }
    catch (const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve channel manager controller reference '" <<
        ref.object_ref << "'. CORBA::Exception caught: " << ex;
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve channel manager controller reference '" <<
        ref.object_ref << "'. eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  ChannelServerSessionPool::ChannelServerSessionPool(
    const CORBACommons::CorbaObjectRefList& controller_refs,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    Generics::ActiveObjectCallback* callback)
    /*throw(Exception)*/
  {
    static const char* FUN = "ChannelServerSessionPool::ChannelServerSessionPool()";

    try
    {
      AdServer::ChannelSvcs::ChannelServerSessionFactory::init(
        *corba_client_adapter,
        &channel_session_factory_,
        0,
        callback,
        0);

      ChannelServerSessionPoolConfig pool_config(corba_client_adapter);
      std::copy(controller_refs.begin(),
        controller_refs.end(),
        std::back_inserter(pool_config.iors_list));
      pool_config.timeout = Generics::Time(10);

      pool_.reset(new ChannelServerSessionPoolImpl(
        pool_config, CORBACommons::ChoosePolicyType::PT_LOOP));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  ChannelServerSessionPool::match(
    const AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& query,
    AdServer::ChannelSvcs::ChannelServerBase::MatchResult_out result)
    /*throw(Exception)*/
  {
    static const char* FUN = "ChannelServerSessionPool::match()";

    try
    {
      ChannelServerSessionHandler channel_server_session =
        pool_->get_object<Exception>();

      try
      {
        channel_server_session->match(query, result);
      }
      catch(const AdServer::ChannelSvcs::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Caught ChannelServer::ImplementationException: " <<
          ex.description;

        throw Exception(ostr);
      }
      catch(const AdServer::ChannelSvcs::NotConfigured& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Caught ChannelServer::NotConfigured: " <<
          ex.description;

        channel_server_session.release_bad(ostr.str());

        throw Exception(ostr);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq*
  ChannelServerSessionPool::get_ccg_traits(
    const AdServer::ChannelSvcs::ChannelIdSeq& ids)
    /*throw(Exception)*/
  {
    static const char* FUN = "ChannelServerSessionPool::get_ccg_traits()";

    try
    {
      ChannelServerSessionHandler channel_server_session =
        pool_->get_object<Exception>();

      try
      {
        return channel_server_session->get_ccg_traits(ids);
      }
      catch(const AdServer::ChannelSvcs::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Caught ChannelServer::ImplementationException: " <<
          ex.description;

        throw Exception(ostr);
      }
      catch(const AdServer::ChannelSvcs::NotConfigured& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Caught ChannelServer::NotConfigured: " <<
          ex.description;

        channel_server_session.release_bad(ostr.str());

        throw Exception(ostr);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
}
