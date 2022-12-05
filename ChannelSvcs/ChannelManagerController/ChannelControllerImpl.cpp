
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <set>
#include <vector>
#include <sstream>
#include <iostream>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Generics/Hash.hpp>
#include <Stream/MemoryStream.hpp>
#include <String/Tokenizer.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/ProcessControl.hpp>
#include <CORBACommons/Stats.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ConfigUtils.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelServer.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelLoadSessionImpl.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelManagerController_s.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/ChannelSvcs/ChannelManagerControllerConfig.hpp>

#include "ChannelControllerImpl.hpp"
#include "ChannelSessionFactory.hpp"
#include "ChannelClusterControlImpl.hpp"
#include "ChannelLoadSessionFactory.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  namespace
  {
    const char ASPECT[] = "ChannelManagerController";
  }

template<class STREAM, class CONTAINER>
STREAM&
log_chunks(STREAM& ostr, const CONTAINER& chunks, unsigned long count_chunks_)
{
  for(auto it = chunks.begin(); it != chunks.end(); it++)
  {
    if(it != chunks.begin())
    {
      ostr << ',';
    }
    ostr << *it;
  } 
  ostr << '/' << count_chunks_;
  return ostr;
}

ChannelControllerImpl::CSInfo::CSInfo() noexcept
  : control_ref(0),
    proccontrol_ref(0),
    server_ref(0),
    update_ref(0),
    process_stat_ref(0)
{
    check_sum[0] = check_sum[1] = 0;
}

void ChannelControllerImpl::add_refs_to_sum(
  std::size_t& check_sum,
  const xsd::AdServer::Configuration::MultiCorbaObjectRefType& refs)
  noexcept
{
  Generics::CRC32Hash hash(check_sum, check_sum);
  for(auto ref_it = refs.Ref().begin(); ref_it != refs.Ref().end(); ref_it++)
  {
    const xsd::AdServer::Configuration::CorbaObjectRefType& ref = *ref_it;
    if(ref.name().present())
    {
      hash.add(ref.name().get().c_str(), ref.name().get().size());
    }
    hash.add(ref.ref().c_str(), ref.ref().size());
    hash.add(ref.ref().c_str(), ref.ref().size());
    if(ref.Secure().present())
    {
      const xsd::AdServer::Configuration::SecureParamsType& secure = ref.Secure().get();
      hash.add(secure.key().c_str(), secure.key().size());
      hash.add(secure.key_word().c_str(), secure.key_word().size());
      hash.add(secure.certificate().c_str(), secure.certificate().size());
      hash.add(secure.certificate_authority().c_str(), secure.certificate_authority().size());
    }
  }
}

ChannelControllerImpl::ChannelControllerImpl(
  Logging::Logger* logger,
  const ChannelControllerConfig* config) /*throw(Exception)*/
  : callback_(new Logging::ActiveObjectCallbackImpl(logger, "ChannelControllerImpl", ASPECT)),
    scheduler_(new Generics::Planner(callback_)),
    task_runner_(new Generics::TaskRunner(callback_, 1)),
    update_period_(20),
    check_sum_(0)
{
  try
  {
    count_chunks_ = config->count_chunks();
    if(!count_chunks_)
    {
      Stream::Error ostr;
      ostr << __func__ << ": count chunks is zero";
      throw Exception(ostr);
    }
    control_servers_ = config->Primary().control();

    connection_.source_id = config->source_id();

    if(config->ColoSettings().present())
    {
      connection_.colo = config->ColoSettings().get().colo();
      connection_.version = config->ColoSettings().get().version();
    }

    c_adapter_ = new CORBACommons::CorbaClientAdapter();

    /*read and init refs to ChannelServers*/
    init_corba_refs_(config);

    const xsd::AdServer::Configuration::ChannelSourceType& type =
      config->ChannelSource();

    if(type.local_groups().present())
    {
      parse_groups_(
        type.local_groups().get(),
        connection_.local_groups);

      Generics::CRC32Hash hash(check_sum_, check_sum_);
      hash.add(
        type.local_groups().get().data(),
        type.local_groups().get().size());
    }

    if(type.RegularSource().present())
    {
      read_regular_connection_(type.RegularSource().get());
    }
    else if(type.ProxySource().present())
    {
      read_proxy_connection_(type.ProxySource().get());
    }
    else
    {
      Stream::Error ostr;
      ostr << __func__ << ": can't read connection information";
      throw Exception(ostr);
    }

    /*split chunks between servers*/
    assign_chunks_();

    if(type.ProxySource().present() &&
       type.ProxySource().get().ProxyRefs().present())
    {
      calc_proxy_sums_(
        type.ProxySource().get().ProxyRefs().get(),
        type.ProxySource().get().use_local_on_first_load());
    }

    add_child_object(task_runner_);
    add_child_object(scheduler_);

    scheduler_control_task_(0);
  }
  catch(const Exception& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": Exception: " << e.what();
    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << __func__ << " : eh::Exception: " << e.what();
    throw Exception(ostr);
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": CORBA::SystemException: " << e;
    throw Exception(ostr);
  }
}

/*split chunks between servers*/
void ChannelControllerImpl::assign_chunks_() /*throw(eh::Exception)*/
{
  for(CSConnection::CSGroups::iterator it = connection_.servers.begin();
      it != connection_.servers.end(); ++it)
  {
    CSConnection::CSInfoVector::iterator i = it->begin();
    bool all_assign = false;
    for(unsigned long j = 0; j < count_chunks_; j++)
    {
      (*i)->chunks.push_back(j);
      if(++i == it->end())
      {
        all_assign = true;
        i = it->begin();
      }
    }
    if(!all_assign)
    {
      logger()->sstream(Logging::Logger::WARNING, ASPECT, "ADS-IMPL-24")
       << __func__ << ": count of chunks less than servers, count servers = "
       << connection_.servers.size() << ", chunks = " << count_chunks_;
    }
    for(i = it->begin(); i != it->end(); ++i)
    {
      Stream::Error chunks_str;
      log_chunks(chunks_str, (*i)->chunks, count_chunks_);
      {
        Generics::CRC32Hash hash((*i)->check_sum[0], check_sum_);
        hash.add(chunks_str.str().data(), chunks_str.str().size());
      }
      (*i)->check_sum[1] = (*i)->check_sum[0];
    }
  }
}

/*recalc check sums for proxy sources*/
void ChannelControllerImpl::calc_proxy_sums_(
  const xsd::AdServer::Configuration::MultiCorbaObjectRefType& refs,
  bool use_local_on_first_load) noexcept
{
  for(CSConnection::CSGroups::iterator it = connection_.servers.begin();
      it != connection_.servers.end(); ++it)
  {
    unsigned long group_number = it - connection_.servers.begin();
    bool this_local_group = std::find(
      connection_.local_groups.begin(),
      connection_.local_groups.end(),
      group_number) !=
      connection_.local_groups.end();

    for(CSConnection::CSInfoVector::iterator i = it->begin();
        i != it->end(); ++i)
    {
      add_refs_to_sum((*i)->check_sum[1], refs);
      if(!use_local_on_first_load || this_local_group || connection_.local_groups.empty())
      {
        (*i)->check_sum[0] = (*i)->check_sum[1];
      }
    }

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << "for group " << it - connection_.servers.begin() << " chunks -> ";
      for(auto i = it->begin(); i != it->end(); ++i)
      {
        log_chunks(ostr, (*i)->chunks, count_chunks_) << "; check sums "
          << i - it->begin() << "->" << (*i)->check_sum[0] << ':'
          << (*i)->check_sum[1] << ' '; 
      }
      logger()->log(ostr.str(), Logging::Logger::TRACE, ASPECT);
    }
  }
}

void ChannelControllerImpl::parse_groups_(
  const String::SubString& groups_str,
  std::vector<unsigned long>& groups)
  /*throw(Exception)*/
{
  String::StringManip::SplitComma spliter(groups_str);
  String::SubString token;
  while(spliter.get_token(token))
  {
    unsigned long num;
    if(String::StringManip::str_to_int(token, num) &&
       num < connection_.servers.size())
    {
      groups.push_back(num);
    }
    else
    {
      Stream::Error err;
      err << __func__ << ": " << num << " isn't number group";
      throw Exception(err);
    }
  }
}

void ChannelControllerImpl::read_proxy_connection_(
    const xsd::AdServer::Configuration::ProxySourceType& proxy_config)
    /*throw(Exception)*/
{
  try
  {
    read_list_refs_(
      proxy_config.CampaignServerCorbaRef(),
      connection_.campaign_server_refs);

    add_refs_to_sum(check_sum_, proxy_config.CampaignServerCorbaRef());

    if(proxy_config.ProxyRefs().present())
    {
      read_list_refs_(
        proxy_config.ProxyRefs().get(),
        connection_.proxy_refs);
      //proxy refs will add to sum latter
    }
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": CORBA::SystemException: " << e;
    throw Exception(ostr);
  }
  catch(const Exception& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": Exception: " << e.what();
    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": eh::Exception: " << e.what();
    throw Exception(ostr);
  }
}

void ChannelControllerImpl::read_regular_connection_(
    const xsd::AdServer::Configuration::RegularSourceType& regular_config)
    /*throw(Exception)*/
{
  try
  {
    read_list_refs_(
      regular_config.CampaignServerCorbaRef(),
      connection_.campaign_server_refs);

    connection_.pg_connection =
      regular_config.PGConnection().connection_string();

    add_refs_to_sum(check_sum_, regular_config.CampaignServerCorbaRef());

    Generics::CRC32Hash hash(check_sum_, check_sum_);
    hash.add(connection_.pg_connection.c_str(), connection_.pg_connection.size());
  }
  catch(const Exception& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": Exception: " << e.what();
    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": eh::Exception: " << e.what();
    throw Exception(ostr);
  }
}

void ChannelControllerImpl::read_list_refs_(
    const xsd::AdServer::Configuration::MultiCorbaObjectRefType& refs,
    CORBACommons::CorbaObjectRefList& list_refs)
    /*throw(Exception)*/
{
  try
  {
    Config::CorbaConfigReader::read_multi_corba_ref(refs, list_refs);
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": CORBA::SystemException: " << e ;
    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": eh::Exception: " << e.what();
    throw Exception(ostr);
  }
}

void ChannelControllerImpl::init_corba_refs_(
    const ChannelControllerConfig* config)
    /*throw(Exception)*/
{
  CORBACommons::CorbaObjectRef corba_object_ref;
  try
  {
    if(config->ControlGroup().empty())
    {
      throw Exception("the list of control groups is empty");
    }
    connection_.servers.reserve(config->ControlGroup().size());
    xsd::AdServer::Configuration::ChannelControllerConfigType::
      ControlGroup_sequence::const_iterator j;

    for(j = config->ControlGroup().begin();
        j != config->ControlGroup().end(); j++)
    {
      if(j->ChannelHost().size())
      {
        connection_.servers.resize(connection_.servers.size() + 1);
        CSConnection::CSInfoVector& vect = connection_.servers.back();
        vect.reserve(j->ChannelHost().size());

        xsd::AdServer::Configuration::ControlGroupType::
          ChannelHost_sequence::const_iterator i;

        CSInfo_var info;
        for(i = j->ChannelHost().begin();
            i != j->ChannelHost().end(); ++i)
        {
          //read server control reference
          Config::CorbaConfigReader::read_corba_ref(
              i->ChannelServerControlRef(),
              corba_object_ref);

          //resolve server control reference
          CORBA::Object_var obj = c_adapter_->resolve_object(corba_object_ref);

          if(CORBA::is_nil(obj.in()))
          {
            Stream::Error ostr;
            ostr << "ChannelServerControllerImpl::init_corba_refs_: "
              "resolve_object return nil reference on channel control."
              " Reference is " << corba_object_ref.object_ref;
            throw Exception(ostr);
          }
          info = new ChannelControllerImpl::CSInfo;

          info->control_ref =
            AdServer::ChannelSvcs::ChannelServerControl::_narrow(obj.in());

          if(CORBA::is_nil(info->control_ref.in()))
          {
            Stream::Error ostr;
            ostr << "ChannelServerControllerImpl::init_corba_refs_: "
              "_narrow return nil reference for channel control. "
              "Reference is " << corba_object_ref.object_ref;
            throw Exception(ostr);
          }

          //read process control reference
          Config::CorbaConfigReader::read_corba_ref(
              i->ChannelServerProcRef(),
              corba_object_ref);

          //resolve process control reference
          obj = c_adapter_->resolve_object(corba_object_ref);

          if(CORBA::is_nil(obj.in()))
          {
            Stream::Error ostr;
            ostr << "ChannelServerControllerImpl::init_corba_refs_: "
              "resolve_object return nil reference on process control. "
              "Reference is " << corba_object_ref.object_ref;
            throw Exception(ostr);
          }

          info->proccontrol_ref =
            CORBACommons::IProcessControl::_narrow(obj.in());

          if(CORBA::is_nil(info->proccontrol_ref.in()))
          {
            Stream::Error ostr;
            ostr << "ChannelServerControllerImpl::init_corba_refs_: "
              "_narrow return nil reference on process control. "
              "Reference is " << corba_object_ref.object_ref;
            throw Exception(ostr);
          }

          //read channel server reference
          Config::CorbaConfigReader::read_corba_ref(
              i->ChannelServerRef(),
              corba_object_ref);

          //resolve channel server reference
          obj = c_adapter_->resolve_object(corba_object_ref);

          if(CORBA::is_nil(obj.in()))
          {
            Stream::Error ostr;
            ostr << "ChannelServerControllerImpl::init_corba_refs_: "
              "resolve_object return nil reference for channel server. "
              "Reference is " << corba_object_ref.object_ref;
            throw Exception(ostr);
          }

          info->server_ref =
            AdServer::ChannelSvcs::ChannelServer::_narrow(obj.in());

          if(CORBA::is_nil(info->server_ref.in()))
          {
            Stream::Error ostr;
            ostr << "ChannelServerControllerImpl::init_corba_refs_: "
              "_narrow return nil reference for channel server. "
              "Reference is " << corba_object_ref.object_ref;
            throw Exception(ostr);
          }

          //read channel update reference
          Config::CorbaConfigReader::read_corba_ref(
              i->ChannelUpdateRef(),
              corba_object_ref);

          //resolve channel update reference
          obj = c_adapter_->resolve_object(corba_object_ref);

          if(CORBA::is_nil(obj.in()))
          {
            Stream::Error ostr;
            ostr << "ChannelServerControllerImpl::init_corba_refs_: "
              "resolve_object return nil reference for channel update. "
              "Reference is " << corba_object_ref.object_ref;
            throw Exception(ostr);
          }

          info->update_ref =
            AdServer::ChannelSvcs::ChannelUpdate_v33::_narrow(obj.in());

          if(CORBA::is_nil(info->update_ref.in()))
          {
            Stream::Error ostr;
            ostr << "ChannelServerControllerImpl::init_corba_refs_: "
              "_narrow return nil reference for channel update. "
              "Reference is " << corba_object_ref.object_ref;
            throw Exception(ostr);
          }

          if(i->ChannelStatRef().present())
          {
            //read process stat reference
            Config::CorbaConfigReader::read_corba_ref(
                i->ChannelStatRef().get(),
                corba_object_ref);

            //resolve process stat reference
            obj = c_adapter_->resolve_object(corba_object_ref);

            if(CORBA::is_nil(obj.in()))
            {
              logger()->sstream(Logging::Logger::ERROR,
                                ASPECT,
                                "ADS-IMPL-25")
                << "ChannelServerControllerImpl::init_corba_refs_: "
                "resolve_object return nil reference for process stat. "
                "Reference is " << corba_object_ref.object_ref;
            }
            else
            {
              info->process_stat_ref =
                CORBACommons::ProcessStatsControl::_narrow(obj.in());

              if(CORBA::is_nil(info->process_stat_ref.in()))
              {
                logger()->sstream(Logging::Logger::ERROR,
                                  ASPECT,
                                  "ADS-IMPL-25")
                  << "ChannelServerControllerImpl::init_corba_refs_: "
                  "_narrow return nil reference for process stat. "
                  "Reference is " << corba_object_ref.object_ref;
                info->process_stat_ref = 0;
              }
            }
          }
          vect.push_back(info);
        }
      }
      else
      {
        throw Exception("the list of channel server's is empty in config");
      }
    }
  }
  catch(const CORBA::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelControllerImpl::init_corba_refs_: "
      << "Catch CORBA::Exception on resolving "
      << corba_object_ref.object_ref <<
      ". : " << e;
    logger()->log(ostr.str(),
                  Logging::Logger::ERROR,
                  ASPECT,
                  "ADS-IMPL-25");
    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelControllerImpl::init_corba_refs_: "
      << "Catch CORBA::SystemException. : "
      << e.what();
    logger()->log(ostr.str(),
                  Logging::Logger::ERROR,
                  ASPECT,
                  "ADS-IMPL-25");
    throw Exception(ostr);
  }
  catch(...)
  {
    Stream::Error ostr;
    ostr << "ChannelControllerImpl::init_corba_refs_: "
      << "unknown Exception." ;
    logger()->log(ostr.str(),
                  Logging::Logger::ERROR,
                  ASPECT,
                  "ADS-IMPL-25");
    throw Exception(ostr);
  }
}

void ChannelControllerImpl::pack_ref_list_(
  const CORBACommons::CorbaObjectRefList& refs,
  ::AdServer::ChannelSvcs::ChannelServerControl::CorbaObjectRefDefSeq& out)
  /*throw(eh::Exception, CORBACommons::CorbaObjectRef::Exception)*/
{
  out.length(refs.size());
  size_t i = 0;
  for(CORBACommons::CorbaObjectRefList::const_iterator ref_it = refs.begin();
      ref_it!=refs.end(); ref_it++)
  {
    ref_it->save(out[i++]);
  }
}


ChannelControllerImpl::~ChannelControllerImpl() noexcept
{
  logger()->log(String::SubString("ChannelControllerImpl destroying"),
      Logging::Logger::TRACE,
      ASPECT);
}

/* check channel servers and set sources if need */
void
ChannelControllerImpl::check_() noexcept
{
  unsigned long group_number = 0;
  for(CSConnection::CSGroups::iterator it = connection_.servers.begin();
    it != connection_.servers.end();
    ++it, ++group_number)
  {
    for(CSConnection::CSInfoVector::iterator i = it->begin(); i != it->end(); ++i)
    {
      size_t conf_index = 0;
      try
      {
        const unsigned long check_sum = (*i)->control_ref->check_configuration();

        if(control_servers_)
        {
          if((*i)->check_sum[1] != check_sum)
          {
            if(((*i)->check_sum[0] == check_sum))
            {
              // this is  first configuration, check state of server
              if((*i)->proccontrol_ref->is_alive() ==
                CORBACommons::AProcessControl::AS_READY)
              {
                // server ready, use second configuration
                conf_index++;
              }
              else
              {
                continue;
              }
            }
               
            if(check_sum)
            {
              logger()->sstream(Logging::Logger::NOTICE,
                ASPECT,
                "ADS-IMPL-21") <<
                "Change configuration for server " <<
                (i - it->begin()) <<
                ", old sum = " << check_sum <<
                ", new sum = " << (*i)->check_sum[0];
            }

            ::AdServer::ChannelSvcs::ChunkKeySeq sources;
            size_t index = 0;
            sources.length((*i)->chunks.size());
            for(auto j = (*i)->chunks.begin(); j != (*i)->chunks.end(); ++j)
            {
              sources[index++] = *j;
            }

            try
            {
              // if this_local_group is true - this group should be loaded from source
              // other groups must be loaded from local group
              const bool this_local_group = connection_.local_groups.empty() ||
                std::find(
                  connection_.local_groups.begin(),
                  connection_.local_groups.end(),
                  group_number) != connection_.local_groups.end();

              if(!this_local_group)
              {
                // local group (load from other ChannelServer group)
                ::AdServer::ChannelSvcs::ChannelServerControl::ProxySourceInfo proxy_info;
                proxy_info.count_chunks = count_chunks_;
                proxy_info.colo = connection_.colo;
                proxy_info.version << connection_.version;
                proxy_info.check_sum = (*i)->check_sum[conf_index];
                proxy_info.local_descriptor.length(connection_.local_groups.size());
                size_t local_index = 0;
                for(auto gr_it = connection_.local_groups.begin();
                  gr_it != connection_.local_groups.end();
                  ++gr_it, ++local_index)
                {
                  pack_load_description_(
                    connection_.servers[*gr_it],
                    proxy_info.local_descriptor[local_index],
                    count_chunks_);
                }

                pack_ref_list_(connection_.campaign_server_refs, proxy_info.campaign_refs);

                (*i)->control_ref->set_proxy_sources(proxy_info, sources);
              }
              else if(connection_.pg_connection.empty())
              {
                // remote configuration (load from other cluster)
                ::AdServer::ChannelSvcs::ChannelServerControl::ProxySourceInfo proxy_info;
                proxy_info.count_chunks = count_chunks_;
                proxy_info.colo = connection_.colo;
                proxy_info.version << connection_.version;
                proxy_info.check_sum = (*i)->check_sum[conf_index];

                if((*i)->check_sum[0] == (*i)->check_sum[1] || conf_index > 0)
                {
                  // second configuration
                  pack_ref_list_(connection_.proxy_refs, proxy_info.proxy_refs);
                }

                pack_ref_list_(connection_.campaign_server_refs, proxy_info.campaign_refs);

                (*i)->control_ref->set_proxy_sources(proxy_info, sources);
              }
              else
              {
                // central configuration (load from DB)
                ::AdServer::ChannelSvcs::ChannelServerControl::DBSourceInfo db_info;
                pack_ref_list_(
                  connection_.campaign_server_refs,
                  db_info.campaign_refs);

                db_info.pg_connection << connection_.pg_connection;

                db_info.count_chunks = count_chunks_;
                db_info.colo = connection_.colo;
                db_info.version << connection_.version;
                db_info.check_sum = (*i)->check_sum[conf_index];
                (*i)->control_ref->set_sources(db_info, sources);
              }

              if(logger()->log_level() >= Logging::Logger::TRACE)
              {
                Stream::Error ostr;
                ostr << "set sources: ";
                log_chunks(ostr, (*i)->chunks, count_chunks_) <<
                  " for server " << group_number << '.' << (i - it->begin());
                logger()->log(ostr.str(), Logging::Logger::TRACE, ASPECT);
              }
            }
            catch(const AdServer::ChannelSvcs::ImplementationException& e)
            {
              Stream::Error ostr;
              ostr << __func__ << ": ImplementationException on "
                "setting sources for " << group_number << '.' <<
                (i - it->begin()) << " server: " << e.description;
              logger()->log(ostr.str(),
                Logging::Logger::CRITICAL,
                ASPECT,
                "ADS-IMPL-22");
            }
            catch(const CORBA::SystemException& e)
            {
              Stream::Error ostr;
              ostr << __func__ << ": CORBA::SystemException on "
                "setting sources for " << group_number << '.' <<
                (i - it->begin()) << " server: " << e;
              logger()->log(ostr.str(),
                Logging::Logger::CRITICAL,
                ASPECT,
                "ADS-IMPL-22");
            }
          }
        }
      }
      catch(const eh::Exception& e)
      {
        logger()->sstream(Logging::Logger::ERROR, ASPECT, "ADS-IMPL-22") <<
          __func__ << ": eh::Exception: " << e.what();
      }
      catch(const CORBA::SystemException& e)
      {
        logger()->sstream(Logging::Logger::ERROR, ASPECT, "ADS-IMPL-22") <<
          __func__ << ": CORBA::SystemException on query to "
          "server " << (i - it->begin()) << ":" << e;
      }
    }
  }

  scheduler_control_task_(update_period_);
}

void
ChannelControllerImpl::scheduler_control_task_(unsigned long period)
  noexcept
{
  try
  {
    Task_var msg = new ControlTask(this, task_runner_);
    Generics::Time tm = Generics::Time::get_time_of_day() + period;

    scheduler_->schedule(msg, tm);
  }
  catch(const eh::Exception& e)
  {
    logger()->sstream(Logging::Logger::ALERT, ASPECT, "ADS-IMPL-20")
      << __func__ << ": eh::Exception: " << e.what();
  }
}

//
// IDL:AdServer/ChannelSvcs/ChannelManagerController/get_channel_session:1.0
//
::AdServer::ChannelSvcs::ChannelServerSession*
ChannelControllerImpl::get_channel_session()
{
  static const char* FN = "ChannelControllerImpl::get_channel_session: ";
  try
  {
    AdServer::ChannelSvcs::Protected::GroupDescriptionSeq descr;
    descr.length(connection_.servers.size());
    size_t j = 0;
    for(CSConnection::CSGroups::const_iterator it = connection_.servers.begin();
        it != connection_.servers.end(); ++it,j++)
    {
      size_t k = 0;
      descr[j].length(it->size());
      for(CSConnection::CSInfoVector::const_iterator i = it->begin();
          i != it->end(); ++i,k++)
      {
        descr[j][k].channel_server = (*i)->server_ref;
      }
    }
    return new ChannelServerSessionImpl(descr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << FN << "eh::Exception : " << e.what();
    CORBACommons::throw_desc<
      ChannelSvcs::ImplementationException>(
        ostr.str());
  }
  catch(CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << FN << "CORBA::SystemException : " << e;
    CORBACommons::throw_desc<
      ChannelSvcs::ImplementationException>(
        ostr.str());
  }
  return 0; // never reach
}

//
// IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
//
::CORBA::ULong ChannelControllerImpl::get_count_chunks()
    /*throw(::CORBA::SystemException)*/
{
  return count_chunks_;
}

void ChannelControllerImpl::pack_load_description_(
  const CSConnection::CSInfoVector& info,
  AdServer::ChannelSvcs::ChannelLoadDescriptionSeq& descr,
  unsigned long count_chunks)
  /*throw(eh::Exception)*/
{
  descr.length(info.size());
  size_t k = 0;
  for(auto i = info.begin(); i != info.end(); i++, k++)
  {
    descr[k].load_server = (*i)->update_ref;
    descr[k].chunks.length((*i)->chunks.size());
    std::copy(
      (*i)->chunks.begin(),
      (*i)->chunks.end(),
      descr[k].chunks.get_buffer());
    descr[k].count_chunks = count_chunks;
  }
}

//
// IDL:AdServer/ChannelSvcs/ChannelManagerController/get_load_session:1.0
//
::AdServer::ChannelSvcs::ChannelLoadSession*
ChannelControllerImpl::get_load_session()
{
  try
  {
    AdServer::ChannelSvcs::GroupLoadDescriptionSeq descr;
    descr.length(connection_.servers.size());
    size_t j = 0;
    for(CSConnection::CSGroups::const_iterator it = connection_.servers.begin();
        it != connection_.servers.end(); ++it, j++)
    {
      pack_load_description_(*it, descr[j], count_chunks_);
    }
    return new ChannelLoadSessionImpl(descr, connection_.source_id);
  }
  catch(const ChannelLoadSessionImpl::Exception& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": ImplementationException: " << e.what();
    CORBACommons::throw_desc<
      ChannelSvcs::ImplementationException>(
        ostr.str());
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": eh::Exception: " << e.what();
    CORBACommons::throw_desc<
      ChannelSvcs::ImplementationException>(
        ostr.str());
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << __func__ << ": CORBA::SystemException: " << e;
    CORBACommons::throw_desc<
      ChannelSvcs::ImplementationException>(
        ostr.str());
  }
  return 0; // never reach
}

::AdServer::ChannelSvcs::ChannelClusterSession*
ChannelControllerImpl::get_control_session()
  /*throw(AdServer::ChannelSvcs::ChannelClusterControl::ImplementationException)*/
{
  try
  {
    AdServer::ChannelSvcs::ProcessControlDescriptionSeq descr;
    size_t j = 0;
    for(CSConnection::CSGroups::const_iterator it = connection_.servers.begin();
        it != connection_.servers.end(); ++it)
    {
      j += it->size();
    }
    descr.length(j);
    j = 0;
    for(CSConnection::CSGroups::const_iterator it = connection_.servers.begin();
        it != connection_.servers.end(); ++it)
    {
      for(CSConnection::CSInfoVector::const_iterator i = it->begin();
          i != it->end(); ++i,j++)
      {
        descr[j] = (*i)->proccontrol_ref;
      }
    }
    return new AdServer::ChannelSvcs::ChannelClusterSessionImpl(descr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelControllerImpl::get_control_session: "
      "Caught eh::Exception. : " << e.what();
    CORBACommons::throw_desc<
      ChannelSvcs::ChannelClusterControl::ImplementationException>(
        ostr.str());
  }
  return 0; // never reach
}

CORBACommons::StatsValueSeq* ChannelControllerImpl::get_stats()
  /*throw(CORBACommons::ProcessStatsControl::ImplementationException)*/
{
  const char* FUN="ChannelControllerImpl::get_stats()";
  try
  {
    switch(connection_.servers.size())
    {
      case 0:
        return new CORBACommons::StatsValueSeq;
      case 1:
        if(connection_.servers.front().size() == 1 &&
           connection_.servers[0][0]->process_stat_ref)
        {
          return connection_.servers[0][0]->process_stat_ref->get_stats();
        }
        else
        {
          CORBACommons::ProcessStatsControl::ImplementationException e;
          e.description = "ProcessStatsControl interface doesn't support";
          throw e;
        }
      default:
        {
          typedef std::map<std::string, size_t> ValueMap;
          ValueMap stat_data;
          ValueMap::iterator fnd;
          size_t value_num;
          CORBACommons::StatsValueSeq_var res;
          for(CSConnection::CSGroups::const_iterator it = connection_.servers.begin();
              it != connection_.servers.end(); ++it)
          {
            for(CSConnection::CSInfoVector::const_iterator i = it->begin();
                i != it->end(); ++i)
            {
              if((*i)->process_stat_ref)
              {
                res = (*i)->process_stat_ref->get_stats();
              }
              else
              {
                CORBACommons::ProcessStatsControl::ImplementationException e;
                e.description = "ProcessStatsControl interface doesn't support";
                throw e;
              }
              for(size_t j = 0; j < res->length(); j++)
              {
                CORBACommons::StatsValue& val = (*res)[j];
                val.value >>= value_num;
                fnd = stat_data.find(val.key.in());
                if(fnd != stat_data.end())
                {
                  fnd->second += value_num;
                }
                else
                {
                  stat_data[val.key.in()] = value_num;
                }
              }
            }
          }
          res = new CORBACommons::StatsValueSeq;
          res->length(stat_data.size());
          value_num = 0;
          for(ValueMap::const_iterator i = stat_data.begin();
              i!=stat_data.end(); ++i)
          {
            (*res)[value_num].key << i->first;
            (*res)[value_num].value <<= i->second;
            value_num++;
          }
          return res._retn();
        }
    }
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << FUN << ": Caught eh::Exception. : "
      << e.what();
    CORBACommons::throw_desc<
      CORBACommons::ProcessStatsControl::ImplementationException>(ostr.str());
  }
  catch(const CORBACommons::ProcessStatsControl::ImplementationException& e)
  {
    Stream::Error ostr;
    ostr << FUN<< ": Caught CORBACommons::"
      "ProcessStatsControl::ImplementationException. : "
      << e.description;
    CORBACommons::throw_desc<
      CORBACommons::ProcessStatsControl::ImplementationException>(ostr.str());
  }
  catch(const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << FUN<< ": Caught CORBA::SystemException. : "
      << e;
    CORBACommons::throw_desc<
      CORBACommons::ProcessStatsControl::ImplementationException>(ostr.str());
  }
  return 0; // never reach
}

ChannelClusterControlImpl::ChannelClusterControlImpl(
  ChannelControllerImpl* controller) noexcept:
  delegate_(ReferenceCounting::add_ref(controller))
{
}

ChannelClusterControlImpl::~ChannelClusterControlImpl() noexcept
{
}

::AdServer::ChannelSvcs::ChannelClusterSession*
ChannelClusterControlImpl::get_control_session()
  /*throw(AdServer::ChannelSvcs::ChannelClusterControl::ImplementationException)*/
{
  return delegate_->get_control_session();
}

CORBACommons::AProcessControl::ALIVE_STATUS
ChannelClusterControlImpl::is_alive() noexcept
{
  try
  {
    ::AdServer::ChannelSvcs::ChannelClusterSession_var session =
      get_control_session();
    return session->is_alive();
  }
  catch(const AdServer::ChannelSvcs::
        ChannelClusterControl::ImplementationException&)
  {
    return CORBACommons::IProcessControl::AS_NOT_ALIVE;
  }
}

char*
ChannelClusterControlImpl::comment() /*throw(CORBACommons::OutOfMemory)*/
{
  try
  {
    ::AdServer::ChannelSvcs::ChannelClusterSession_var session =
      get_control_session();
    return session->comment();
  }
  catch(const AdServer::ChannelSvcs::
        ChannelClusterControl::ImplementationException& e)
  {
    Stream::Error ostr;
    ostr << "AdServer::ChannelSvcs::"
      "ChannelClusterControlImpl::ImplementationException:"
      << e.description;
    CORBA::String_var str;
    str << ostr.str();
    return str._retn();
  }
}

ChannelStatImpl::ChannelStatImpl(ChannelControllerImpl* controller) noexcept:
  delegate_(ReferenceCounting::add_ref(controller))
{
}

CORBACommons::StatsValueSeq* ChannelStatImpl::get_stats()
  /*throw(CORBACommons::ProcessStatsControl::ImplementationException)*/
{
  return delegate_->get_stats();
}

} /* ChannelSvcs */
} /* AdServer */
