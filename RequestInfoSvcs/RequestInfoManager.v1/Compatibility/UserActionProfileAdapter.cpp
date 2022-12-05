#include <Generics/Time.hpp>
#include <Commons/Algs.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserActionProfile.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Compatibility/UserActionProfile_v321.hpp>

#include "UserActionProfileAdapter.hpp"

namespace AdServer
{
  template<typename ResultType>
  struct ActionMarkerConverter
  {
    template<typename ActionMarkerReaderType>
    ResultType
    operator()(const ActionMarkerReaderType& reader) const
    {
      ResultType new_act_marker;
      new_act_marker.ccg_id() = reader.ccg_id();
      new_act_marker.cc_id() = reader.cc_id();
      new_act_marker.request_id() = reader.request_id();
      new_act_marker.time() = reader.time();
      return new_act_marker;
    }
  };

  template<typename ResultType>
  struct WaitActionConverter
  {
    template<typename WaitActionReaderType>
    ResultType
    operator()(const WaitActionReaderType& reader) const
    {
      ResultType new_wact;
      new_wact.ccg_id() = reader.ccg_id();
      new_wact.time() = reader.time();
      new_wact.count() = reader.count();
      return new_wact;
    }
  };

  namespace RequestInfoSvcs_v321
  {
    namespace RequestInfoSvcsTarget
    {
      using namespace ::AdServer::RequestInfoSvcs;
    }

    struct CustomActionMarkerKey
    {
      CustomActionMarkerKey(
        unsigned long time_val,
        unsigned long action_id_val,
        const char* action_request_id_val)
        : time(time_val),
          action_id(action_id_val),
          action_request_id(action_request_id_val)
      {}

      bool
      operator<(const CustomActionMarkerKey& right) const noexcept
      {
        return time < right.time || (
          (time == right.time && (
            action_id < right.action_id ||
            (action_id == right.action_id &&
             action_request_id < right.action_request_id))));
      }

      unsigned long time;
      unsigned long action_id;
      std::string action_request_id;
    };

    void
    convert_to_v330(Generics::MemBuf& membuf)
    {
      static const char* FUN = "RequestInfoSvcs_321::convert_to_v330()";

      typedef std::map<
        std::string, RequestInfoSvcsTarget::CustomActionMarkerWriter>
        ActionRequestIdToCustomActionMarkerWriterMap;

      typedef std::map<
        CustomActionMarkerKey, RequestInfoSvcsTarget::CustomActionMarkerWriter>
        CustomActionMarkerWriterMap;

      try
      {
        RequestInfoSvcsTarget::UserActionProfileWriter profile_writer;
        profile_writer.version() = 330;

        AdServer::RequestInfoSvcs_v321::UserActionProfileReader old_profile_reader(
          membuf.data(), membuf.size());

        std::copy(
          old_profile_reader.action_markers().begin(),
          old_profile_reader.action_markers().end(),
          Algs::modify_inserter(
            std::back_inserter(profile_writer.action_markers()),
            ActionMarkerConverter<RequestInfoSvcsTarget::ActionMarkerWriter>()));

        std::copy(
          old_profile_reader.wait_actions().begin(),
          old_profile_reader.wait_actions().end(),
          Algs::modify_inserter(
            std::back_inserter(profile_writer.wait_actions()),
            WaitActionConverter<RequestInfoSvcsTarget::WaitActionWriter>()));

        for(AdServer::RequestInfoSvcs_v321::UserActionProfileReader::
              done_impressions_Container::const_iterator it =
                old_profile_reader.done_impressions().begin();
            it != old_profile_reader.done_impressions().end(); ++it)
        {
          RequestInfoSvcsTarget::DoneImpressionWriter done_imp_writer;
          done_imp_writer.ccg_id() = (*it).ccg_id();
          done_imp_writer.time() = (*it).time();
          done_imp_writer.request_id() = (*it).request_id();
          profile_writer.done_impressions().push_back(done_imp_writer);
        }

        // pack custom_action_markers by action_request_id
        ActionRequestIdToCustomActionMarkerWriterMap custom_action_markers_map;

        for(AdServer::RequestInfoSvcs_v321::UserActionProfileReader::
              custom_action_markers_Container::const_iterator it =
                old_profile_reader.custom_action_markers().begin();
            it != old_profile_reader.custom_action_markers().end(); ++it)
        {
          ActionRequestIdToCustomActionMarkerWriterMap::iterator cam_it =
            custom_action_markers_map.find((*it).action_request_id());

          if(cam_it != custom_action_markers_map.end())
          {
            RequestInfoSvcsTarget::CustomActionMarkerWriter::
              ccg_ids_Container::iterator ccg_id_it = std::lower_bound(
                cam_it->second.ccg_ids().begin(),
                cam_it->second.ccg_ids().end(),
                (*it).ccg_id());
            if(ccg_id_it == cam_it->second.ccg_ids().end() ||
               *ccg_id_it != (*it).ccg_id())
            {
              cam_it->second.ccg_ids().insert(ccg_id_it, (*it).ccg_id());
            }
          }
          else
          {
            RequestInfoSvcsTarget::CustomActionMarkerWriter custom_action_marker_writer;
            custom_action_marker_writer.time() = (*it).time();
            custom_action_marker_writer.action_id() = (*it).action_id();
            custom_action_marker_writer.action_request_id() = (*it).action_request_id();
            custom_action_marker_writer.ccg_ids().push_back((*it).ccg_id());
            custom_action_marker_writer.referer() = (*it).referer();
            custom_action_marker_writer.order_id() = (*it).order_id();
            custom_action_marker_writer.action_value() = (*it).action_value();
            custom_action_markers_map.insert(
              std::make_pair((*it).action_request_id(), custom_action_marker_writer));
          }
        }

        // order custom_action_markers_map by time, action_id
        CustomActionMarkerWriterMap ordered_custom_action_markers_map;

        for(ActionRequestIdToCustomActionMarkerWriterMap::
              const_iterator cam_it = custom_action_markers_map.begin();
            cam_it != custom_action_markers_map.end(); ++cam_it)
        {
          ordered_custom_action_markers_map.insert(std::make_pair(
            CustomActionMarkerKey(
              cam_it->second.time(),
              cam_it->second.action_id(),
              cam_it->second.action_request_id().c_str()),
            cam_it->second));
        }

        for(CustomActionMarkerWriterMap::const_iterator cam_it =
              ordered_custom_action_markers_map.begin();
            cam_it != ordered_custom_action_markers_map.end(); ++cam_it)
        {
          profile_writer.custom_action_markers().push_back(cam_it->second);
        }

        for(AdServer::RequestInfoSvcs_v321::UserActionProfileReader::
              custom_done_actions_Container::const_iterator it =
                old_profile_reader.custom_done_actions().begin();
            it != old_profile_reader.custom_done_actions().end(); ++it)
        {
          RequestInfoSvcsTarget::DoneActionWriter done_action_writer;
          done_action_writer.action_id() = (*it).action_id();
          done_action_writer.time() = (*it).time();
          profile_writer.custom_done_actions().push_back(done_action_writer);
        }

        Generics::MemBuf mb(profile_writer.size());
        profile_writer.save(mb.data(), mb.size());
        membuf.assign(mb.data(), mb.size());
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't adapt profile: " << ex.what();
        throw RequestInfoSvcs::UserActionProfileAdapter::Exception(ostr);
      }
    }
  }
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  Generics::ConstSmartMemBuf_var
  UserActionProfileAdapter::operator()(
    const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/
  {
    static const char* FUN = "UserActionProfileAdapter::operator()";

    /* check profile version and convert to latest if version is old */
    unsigned long version_head_size =
      UserActionProfileVersionReader::FIXED_SIZE;

    if(mem_buf->membuf().size() < version_head_size)
    {
      Stream::Error ostr;
      ostr << FUN << "Corrupt header: size of profile = " <<
        mem_buf->membuf().size();
      throw Exception(ostr);
    }

    UserActionProfileVersionReader version_reader(
      mem_buf->membuf().data(), version_head_size);

    if(version_reader.version() != CURRENT_ACTION_INFO_PROFILE_VERSION)
    {
      Generics::SmartMemBuf_var result_mem_buf = Algs::copy_membuf(mem_buf);
      Generics::MemBuf& membuf = result_mem_buf->membuf();

      unsigned long current_version = version_reader.version();

      if(current_version == 321)
      {
        RequestInfoSvcs_v321::convert_to_v330(membuf);
        current_version = 330;
      }

      if(current_version != CURRENT_ACTION_INFO_PROFILE_VERSION)
      {
        Stream::Error ostr;
        ostr << FUN << ": incorrect version after adaptation = " <<
          current_version;
        throw Exception(ostr);
      }

      return Generics::transfer_membuf(result_mem_buf);
    }

    return ReferenceCounting::add_ref(mem_buf);
  }
}
}
