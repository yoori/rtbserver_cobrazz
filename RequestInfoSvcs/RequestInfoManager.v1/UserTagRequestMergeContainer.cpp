#include <sstream>

#include <Generics/Time.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserTagRequestMergeProfile.hpp>

#include "UserTagRequestMergeContainer.hpp"

namespace Aspect
{
  const char TAG_REQUEST_MERGE_CONTAINER[] = "TagRequestMergeContainer";
}

namespace
{
  const unsigned long CURRENT_USER_TAG_REQUEST_PROFILE_VERSION = 1;
  const Generics::Time KEEP_GROUP_TIME = Generics::Time::ONE_DAY;
  const unsigned long KEEP_MAX_GROUPS = 100;
  const unsigned long MAX_TAGS_IN_GROUP = 20;
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    void init_tag_group_info(
      TagRequestGroupProcessor::TagRequestGroupInfo& tag_request_group_info,
      const TagGroupWriter& tag_request_group_writer)
    {
      tag_request_group_info.time =
        Generics::Time(tag_request_group_writer.min_time());
      tag_request_group_info.site_id = tag_request_group_writer.site_id();
      tag_request_group_info.country = tag_request_group_writer.country();
      tag_request_group_info.colo_id = tag_request_group_writer.colo_id();
      tag_request_group_info.ad_shown = tag_request_group_writer.ad_shown();
      std::copy(tag_request_group_writer.tags().begin(),
        tag_request_group_writer.tags().end(),
        std::inserter(tag_request_group_info.tags, tag_request_group_info.tags.begin()));
    }

    void fill_tag_group_info(
      TagRequestGroupProcessor::TagRequestGroupInfo& tag_request_group_info,
      const TagRequestInfo& tag_request_info)
    {
      tag_request_group_info.time = tag_request_info.time;
      tag_request_group_info.colo_id = tag_request_info.colo_id;
      tag_request_group_info.site_id = tag_request_info.site_id;
      tag_request_group_info.country = tag_request_info.country;      
      tag_request_group_info.tags.insert(tag_request_info.tag_id);
      tag_request_group_info.ad_shown = tag_request_info.ad_shown;
      tag_request_group_info.rollback = false;
    }

    void clear_excess_tag_request_groups(
      UserTagRequestMergeProfileWriter::tag_groups_Container& tag_groups,
      const Generics::Time& keep_time)
    {
      UserTagRequestMergeProfileWriter::tag_groups_Container::
        iterator erase_it = tag_groups.begin();

      unsigned long tag_groups_size = tag_groups.size();      
      long count_to_erase = tag_groups_size > KEEP_MAX_GROUPS ?
        KEEP_MAX_GROUPS - tag_groups_size : 0;

      while(erase_it != tag_groups.end() &&
        (erase_it->max_time() < keep_time.tv_sec || count_to_erase > 0))
      {
        ++erase_it;
        --count_to_erase;
      }

      tag_groups.erase(tag_groups.begin(), erase_it);
    }

    void
    insert_tag_request(
      TagRequestGroupProcessor::TagRequestGroupInfoList& tag_group_info_list,
      UserTagRequestMergeProfileWriter::tag_groups_Container& tag_groups,
      const TagRequestInfo& tag_request_info,
      const Generics::Time& merge_by_time_bound)
    {
      UserTagRequestMergeProfileWriter::tag_groups_Container::
        reverse_iterator target_git = tag_groups.rend();

      unsigned long referer_hash = tag_request_info.referer_hash ?
        *tag_request_info.referer_hash : 0;

      /* group merging condition(F - all fields excluding pl and referer_hash):
       *   left.F == right.F && (
       *     left.page_load_id == right.page_load_id ||
       *       (left.page_load_id == 0 || right.page_load_id == 0) &&
       *       left.referer_hash == right.referer_hash &&
       *       (time condition)
       */
      if(tag_request_info.page_load_id &&
        *tag_request_info.page_load_id)
      {
        for(UserTagRequestMergeProfileWriter::tag_groups_Container::
              reverse_iterator change_git = tag_groups.rbegin();
            change_git != tag_groups.rend(); ++change_git)
        {
          if((change_git->page_load_id() == *tag_request_info.page_load_id ||
                (change_git->page_load_id() == 0 &&
                change_git->referer_hash() == referer_hash &&
                (tag_request_info.time + merge_by_time_bound).tv_sec >=
                   change_git->min_time() &&
                tag_request_info.time.tv_sec <=
                  change_git->max_time() + merge_by_time_bound.tv_sec)) &&
             change_git->site_id() == tag_request_info.site_id &&
             change_git->colo_id() == tag_request_info.colo_id &&
             change_git->country() == tag_request_info.country)
          {
            target_git = change_git;
            break;
          }
        }
      }
      else
      {
        for(UserTagRequestMergeProfileWriter::tag_groups_Container::
              reverse_iterator change_git = tag_groups.rbegin();
            change_git != tag_groups.rend(); ++change_git)
        {
          if(change_git->max_time() + merge_by_time_bound.tv_sec <
             tag_request_info.time.tv_sec)
          {
            break;
          }

          if(change_git->referer_hash() == referer_hash &&
             (tag_request_info.time + merge_by_time_bound).tv_sec >=
               change_git->min_time() &&
             change_git->site_id() == tag_request_info.site_id &&
             change_git->colo_id() == tag_request_info.colo_id &&
             change_git->country() == tag_request_info.country)
          {
            target_git = change_git;
            break;
          }
        }
      }

      if(target_git != tag_groups.rend())
      {
        // change exists group
        if(tag_request_info.time.tv_sec > target_git->max_time() &&
           target_git != tag_groups.rbegin())
        {
          // find new position for changed max_time
          UserTagRequestMergeProfileWriter::tag_groups_Container::
            iterator ins_it = target_git.base();
          while(ins_it != tag_groups.end() &&
            ins_it->max_time() < tag_request_info.time.tv_sec)
          {
            ++ins_it;
          }
          tag_groups.splice(ins_it, tag_groups, --target_git.base());
          target_git = UserTagRequestMergeProfileWriter::tag_groups_Container::
            reverse_iterator(ins_it);
        }

        // rollback previous delegated tag request group
        tag_group_info_list.push_back(TagRequestGroupProcessor::TagRequestGroupInfo());
        init_tag_group_info(*tag_group_info_list.rbegin(), *target_git);
        tag_group_info_list.rbegin()->rollback = true;

        // delegate new tag request group
        tag_group_info_list.push_back(*tag_group_info_list.rbegin());
        tag_group_info_list.rbegin()->tags.insert(tag_request_info.tag_id);
        tag_group_info_list.rbegin()->ad_shown |=
          (tag_request_info.ad_shown ? 1 : 0);
        tag_group_info_list.rbegin()->rollback = false;

        if(target_git->tags().size() + 1 < MAX_TAGS_IN_GROUP)
        {
          target_git->tags().push_back(tag_request_info.tag_id);
          target_git->ad_shown() |= (tag_request_info.ad_shown ? 1 : 0);
          target_git->min_time() = std::min(
            target_git->min_time(), static_cast<uint32_t>(tag_request_info.time.tv_sec));
          target_git->max_time() = std::max(
            target_git->max_time(), static_cast<uint32_t>(tag_request_info.time.tv_sec));
          if(tag_request_info.page_load_id &&
            *tag_request_info.page_load_id)
          {
            target_git->page_load_id() = *tag_request_info.page_load_id;
          }
        }
        else
        {
          tag_groups.erase(--target_git.base());
        }
      }
      else
      {
        // create new group
        UserTagRequestMergeProfileWriter::tag_groups_Container::
          reverse_iterator ins_it = tag_groups.rbegin();
        while(ins_it != tag_groups.rend() &&
          ins_it->max_time() > tag_request_info.time.tv_sec)
        {
          ++ins_it;
        }

        TagGroupWriter new_tag_group_writer;
        new_tag_group_writer.page_load_id() = tag_request_info.page_load_id ?
          *tag_request_info.page_load_id : 0;
        new_tag_group_writer.referer_hash() = referer_hash;
        new_tag_group_writer.min_time() = tag_request_info.time.tv_sec;
        new_tag_group_writer.max_time() = tag_request_info.time.tv_sec;
        new_tag_group_writer.site_id() = tag_request_info.site_id;
        new_tag_group_writer.country() = tag_request_info.country;
        new_tag_group_writer.colo_id() = tag_request_info.colo_id;
        new_tag_group_writer.ad_shown() = tag_request_info.ad_shown;
        new_tag_group_writer.tags().push_back(tag_request_info.tag_id);
        tag_groups.insert(ins_it.base(), new_tag_group_writer);

        tag_group_info_list.push_back(TagRequestGroupProcessor::TagRequestGroupInfo());
        fill_tag_group_info(*tag_group_info_list.rbegin(), tag_request_info);
      }
    }
  }

  const Generics::Time UserTagRequestMergeContainer::DEFAULT_EXPIRE_TIME =
    Generics::Time::ONE_DAY;

  UserTagRequestMergeContainer::UserTagRequestMergeContainer(
    Logging::Logger* logger,
    TagRequestGroupProcessor* tag_request_group_processor,
    const Generics::Time& time_merge_bound,
    const char* file_base_path,
    const char* file_prefix,
    ProfilingCommons::ProfileMapFactory::Cache* cache,
    const Generics::Time& expire_time,
    const Generics::Time& extend_time_period)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      tag_request_group_processor_(ReferenceCounting::add_ref(tag_request_group_processor)),
      time_merge_bound_(time_merge_bound),
      expire_time_(expire_time)
  {
    static const char* FUN = "UserTagRequestMergeContainer::UserTagRequestMergeContainer()";

    Generics::Time extend_time_period_val(extend_time_period);
    
    if(extend_time_period_val == Generics::Time::ZERO)
    {
      extend_time_period_val = expire_time / 4;
    }

    try
    {
      user_map_ = AdServer::ProfilingCommons::ProfileMapFactory::
        open_transaction_expire_map<
          AdServer::Commons::UserId,
          ProfilingCommons::UserIdAccessor>(
          file_base_path,
          file_prefix,
          extend_time_period_val,
          cache);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init ProfileMap. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  Generics::ConstSmartMemBuf_var
  UserTagRequestMergeContainer::get_profile(
    const AdServer::Commons::UserId& user_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserTagRequestMergeContainer::get_profile()";
    try
    {
      return user_map_->get_profile(user_id);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get profile for id = " << user_id <<
        ". Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserTagRequestMergeContainer::process_tag_request(
    const TagRequestInfo& tag_request_info)
    /*throw(TagRequestProcessor::Exception)*/
  {
    static const char* FUN = "UserTagRequestMergeContainer::process_tag_request()";

    if(tag_request_info.user_id.is_null() ||
       !tag_request_info.tag_id)
    {
      return;
    }

    TagRequestGroupProcessor::TagRequestGroupInfoList tag_request_group_info_list;

    if(!tag_request_info.referer_hash &&
       !tag_request_info.page_load_id)
    {
      // don't save group into profile - it can't be merged with some other request
      tag_request_group_info_list.push_back(TagRequestGroupProcessor::TagRequestGroupInfo());
      fill_tag_group_info(*tag_request_group_info_list.rbegin(), tag_request_info);
    }
    else
    {
      process_tag_request_trans_(
        tag_request_group_info_list,
        tag_request_info);
    }

    try
    {
      for(TagRequestGroupProcessor::TagRequestGroupInfoList::const_iterator it =
            tag_request_group_info_list.begin();
          it != tag_request_group_info_list.end(); ++it)
      {
        tag_request_group_processor_->process_tag_request_group(*it);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw TagRequestProcessor::Exception(ostr);
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": Processed request: " << std::endl;
      tag_request_info.print(ostr, "  ");

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::TAG_REQUEST_MERGE_CONTAINER);
    }
  }

  void UserTagRequestMergeContainer::process_tag_request_trans_(
    TagRequestGroupProcessor::TagRequestGroupInfoList& tag_request_group_info_list,
    const TagRequestInfo& tag_request_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserTagRequestMergeContainer::process_tag_request_trans_()";

    try
    {
      UserTagRequestMergeProfileWriter user_profile_writer;

      UserMap::Transaction_var transaction =
        user_map_->get_transaction(tag_request_info.user_id);
      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();
  
      if(mem_buf.in())
      {
        user_profile_writer.init(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());
      }
      else
      {
        user_profile_writer.version() = CURRENT_USER_TAG_REQUEST_PROFILE_VERSION;
      }

      // insert new group
      insert_tag_request(
        tag_request_group_info_list,
        user_profile_writer.tag_groups(),
        tag_request_info,
        time_merge_bound_);

      clear_excess_tag_request_groups(
        user_profile_writer.tag_groups(),
        tag_request_info.time - KEEP_GROUP_TIME);

      // save profile
      unsigned long sz = user_profile_writer.size();
      Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));
  
      user_profile_writer.save(new_mem_buf->membuf().data(), sz);
  
      transaction->save_profile(
        Generics::transfer_membuf(new_mem_buf),
        tag_request_info.time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw UserTagRequestMergeContainer::Exception(ostr);
    }
  }
  
  void UserTagRequestMergeContainer::clear_expired()
    /*throw(Exception)*/
  {
    Generics::Time now = Generics::Time::get_time_of_day();
    user_map_->clear_expired(now - expire_time_);
  }
} /* namespace RequestInfoSvcs */
} /* namespace AdServer */
