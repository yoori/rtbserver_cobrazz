#include <Logger/ActiveObjectCallback.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>
#include "Compatibility/UserColoReachProfileAdapter.hpp"

#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>
#include "UserColoReachContainer.hpp"

#include "Algs.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    UserColoReachContainer::UserColoReachContainer(
      Logging::Logger* logger,
      ColoReachProcessor* colo_reach_processor,
      bool household,
      unsigned long common_chunks_number,
      const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
      const char* file_prefix,
      ProfilingCommons::ProfileMapFactory::Cache* /*cache*/,
      const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits)
      /*throw(Exception)*/
      : logger_(ReferenceCounting::add_ref(logger)),
        colo_reach_processor_(ReferenceCounting::add_ref(colo_reach_processor)),
        HOUSEHOLD_(household)
    {
      static const char* FUN = "UserColoReachContainer::UserColoReachContainer()";

      try
      {
        typedef AdServer::ProfilingCommons::OptionalProfileAdapter<UserColoReachProfileAdapter>
          AdaptUserColoReachProfile;

        user_map_ = AdServer::ProfilingCommons::ProfileMapFactory::
          open_chunked_map<
            AdServer::Commons::UserId,
            AdServer::ProfilingCommons::UserIdAccessor,
            unsigned long (*)(const Generics::Uuid& uuid),
            AdaptUserColoReachProfile>(
              common_chunks_number,
              chunk_folders,
              file_prefix,
              user_level_map_traits,
              *this,
              Generics::ActiveObjectCallback_var(
                new Logging::ActiveObjectCallbackImpl(
                  logger_,
                  "UserColoReachContainer",
                  "ExpressionMatcher",
                  "ADS-IMPL-4024")),
              AdServer::Commons::uuid_distribution_hash,
              nullptr // file controller
              );
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't init profiles map. Caught eh::Exception: " <<
          ex.what();
        throw Exception(ostr);
      }
    }

    Generics::ConstSmartMemBuf_var
    UserColoReachContainer::get_profile(
      const AdServer::Commons::UserId& user_id)
      /*throw(Exception)*/
    {
      static const char* FUN = "UserColoReachContainer::get_profile()";

      try
      {
        return user_map_->get_profile(user_id);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't get profile. Caught eh::Exception: " <<
          e.what();
        throw Exception(ostr);
      }
    }

    void
    UserColoReachContainer::clear_expired() /*throw(Exception)*/
    {
      Generics::Time now = Generics::Time::get_time_of_day();
      user_map_->clear_expired(now - expire_time_);
    }

    void
    UserColoReachContainer::process_request(
      const RequestInfo& request_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "UserColoReachContainer::process_request()";

      if(!request_info.user_id.is_null())
      {
        ColoReachInfoList gmt_colo_reach_info_list;
        ColoReachInfoList isp_colo_reach_info_list;

        try
        {
          process_request_trans_(
            gmt_colo_reach_info_list,
            isp_colo_reach_info_list,
            request_info);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught eh::Exception "
            "on processing request transaction: " << e.what();
          throw Exception(ostr);
        }

        if(colo_reach_processor_.in())
        {
          try
          {
            for(ColoReachInfoList::const_iterator it =
                  gmt_colo_reach_info_list.begin();
                  it != gmt_colo_reach_info_list.end(); ++it)
            {
              colo_reach_processor_->process_gmt_colo_reach(*it);
            }

            for(ColoReachInfoList::const_iterator it =
                  isp_colo_reach_info_list.begin();
                  it != isp_colo_reach_info_list.end(); ++it)
            {
              colo_reach_processor_->process_isp_colo_reach(*it);
            }
          }
          catch(const eh::Exception& e)
          {
            Stream::Error ostr;
            ostr << FUN << ": Caught eh::Exception "
              "on request processing delegate: " << e.what();
            throw Exception(ostr);
          }
        }
      }
    }

    void
    UserColoReachContainer::process_request_trans_(
      ColoReachInfoList& gmt_colo_reach_info_list,
      ColoReachInfoList& isp_colo_reach_info_list,
      const RequestInfo& request_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "UserColoReachContainer::process_request_trans_()";

      ColoReachProcessor::ColoReachInfo gmt_colo_reach_info;
      ColoReachProcessor::ColoReachInfo isp_colo_reach_info;

      UserInfoMap::Transaction_var transaction;

      try
      {
        Generics::ConstSmartMemBuf_var mem_buf;

        try
        {
          transaction = user_map_->get_transaction(request_info.user_id);

          mem_buf = transaction->get_profile();
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": on read, for user_id = " <<
            request_info.user_id.to_string() <<
            "' caught eh::Exception: " << ex.what();
          throw Exception(ostr);
        }

        UserColoReachProfileWriter profile_writer;

        const Generics::Time& time = request_info.time;
        const Generics::Time date(Algs::round_to_day(time));

        const Generics::Time& isp_time = request_info.isp_time;
        const Generics::Time isp_date(Algs::round_to_day(isp_time));

        bool save_colo = false;
        bool colo_appeared = true;
        bool isp_colo_appeared = true;

        if(mem_buf.in())
        {
          UserColoReachProfileReader profile_reader(
            mem_buf->membuf().data(),
            mem_buf->membuf().size());

          profile_writer.init(mem_buf->membuf().data(),
            mem_buf->membuf().size());

          Generics::Time old_isp_time;
          bool old_isp_time_found =
            get_time_by_id(
              profile_reader.isp_colo_create_time().begin(),
              profile_reader.isp_colo_create_time().end(),
              request_info.colo_id,
              old_isp_time);

          const Generics::Time old_gmt_time =
            Generics::Time(profile_reader.create_time());

          bool changed_create_time = (time < old_gmt_time);
          // changed_create_time: revert and resave all data,
          //   with possible updated TZ for passed colo_id, insert new colo_id.
          // changed_isp_time only - change TZ only for passed and known id
          // If none of create_time or isp_create_time changed for passed id.
          //   (possible) insert new colo id, update appearance

          bool changed_isp_create_time = old_isp_time_found &&
            (isp_time < old_isp_time);

          if(changed_isp_create_time || changed_create_time)
          {
            save_colo = true;

            gmt_colo_reach_info_list.push_back(ColoReachProcessor::ColoReachInfo());

            ColoReachProcessor::ColoReachInfo& revert_gmt_colo_reach_info =
              gmt_colo_reach_info_list.back();

            revert_gmt_colo_reach_info.create_time = old_gmt_time;
            revert_gmt_colo_reach_info.household = HOUSEHOLD_;

            ColoReachProcessor::ColoReachInfo
              revert_tmp_collector, resave_tmp_collector;

            // collect old records
            collect_all_appearance(
              revert_tmp_collector.colocations,
              profile_reader.isp_colo_appears().begin(),
              profile_reader.isp_colo_appears().end(),
              -1,
              BaseAppearanceIdGetter());

            // collect new records
            collect_all_appearance(
              resave_tmp_collector.colocations,
              profile_reader.isp_colo_appears().begin(),
              profile_reader.isp_colo_appears().end(),
              1,
              BaseAppearanceIdGetter());

            // resave all appearances
            if(changed_create_time)
            {
              profile_writer.create_time() = time.tv_sec;

              collect_all_appearance(
                revert_gmt_colo_reach_info.colocations,
                profile_reader.colo_appears().begin(),
                profile_reader.colo_appears().end(),
                -1,
                BaseAppearanceIdGetter());

              collect_all_appearance(
                gmt_colo_reach_info.colocations,
                profile_reader.colo_appears().begin(),
                profile_reader.colo_appears().end(),
                1,
                BaseAppearanceIdGetter());

              update_id_time_list(
                profile_writer.isp_colo_create_time(),
                request_info.colo_id,
                old_gmt_time,
                time,
                isp_time,
                BaseIdTimeLess(),
                BaseInsertIdTimeAdapter<ColoCreateTimeWriter>());

              // revert and resave  records for all colo_id
              split_colo_reach_info_by_id(
                isp_colo_reach_info_list,
                revert_tmp_collector,
                request_info.colo_id,
                HOUSEHOLD_,
                profile_reader.isp_colo_create_time().begin(),
                profile_reader.isp_colo_create_time().end(),
                false);

              split_colo_reach_info_by_id(
                isp_colo_reach_info_list,
                resave_tmp_collector,
                request_info.colo_id,
                HOUSEHOLD_,
                profile_writer.isp_colo_create_time().begin(),
                profile_writer.isp_colo_create_time().end(),
                false);
            }

            if(changed_isp_create_time && !changed_create_time)
            {
              // Here we revert and resave only data for current colo_id
              // Only TZ for colo_id changed,
              update_id_time_list(
                profile_writer.isp_colo_create_time(),
                request_info.colo_id,
                old_gmt_time,
                time,
                isp_time,
                BaseIdTimeLess(),
                BaseInsertIdTimeAdapter<ColoCreateTimeWriter>());

              // revert and resave  records for all colo_id
              split_colo_reach_info_by_id(
                isp_colo_reach_info_list,
                revert_tmp_collector,
                request_info.colo_id,
                HOUSEHOLD_,
                profile_reader.isp_colo_create_time().begin(),
                profile_reader.isp_colo_create_time().end(),
                true);

              split_colo_reach_info_by_id(
                isp_colo_reach_info_list,
                resave_tmp_collector,
                request_info.colo_id,
                HOUSEHOLD_,
                profile_writer.isp_colo_create_time().begin(),
                profile_writer.isp_colo_create_time().end(),
                true);
            }
          }

          // FIXME: Smth strange: profile with create_time() == 0?
          if(profile_reader.create_time() == 0)
          {
            profile_writer.create_time() = time.tv_sec;
          }

          {
            // Possible add new colo_id in isp_colo_create_time
            Generics::Time new_isp_time;
            bool new_isp_time_found =
              get_time_by_id(
                profile_reader.isp_colo_create_time().begin(),
                profile_reader.isp_colo_create_time().end(),
                request_info.colo_id,
                new_isp_time);

            if (!new_isp_time_found || new_isp_time == Generics::Time::ZERO)
            {
              update_id_time_list(
                profile_writer.isp_colo_create_time(),
                request_info.colo_id,
                old_gmt_time,
                time,
                isp_time,
                BaseIdTimeLess(),
                BaseInsertIdTimeAdapter<ColoCreateTimeWriter>());
            }
          }

          // work with current request.
          colo_appeared = collect_appearance(
            gmt_colo_reach_info.colocations,
            profile_reader.colo_appears().begin(),
            profile_reader.colo_appears().end(),
            request_info.colo_id,
            date,
            BaseAppearanceLess());

          isp_colo_appeared = collect_appearance(
            isp_colo_reach_info.colocations,
            profile_reader.isp_colo_appears().begin(),
            profile_reader.isp_colo_appears().end(),
            request_info.colo_id,
            isp_date,
            BaseAppearanceLess());
        }
        else
        {
          // membuf.in() == 0, create new profile
          profile_writer.version() = CURRENT_USER_COLO_REACH_PROFILE_VERSION;
          profile_writer.create_time() = time.tv_sec;
          update_id_time_list(
            profile_writer.isp_colo_create_time(),
            request_info.colo_id,
            time,
            time,
            isp_time,
            BaseIdTimeLess(),
            BaseInsertIdTimeAdapter<ColoCreateTimeWriter>());

          add_total_appearance(
            gmt_colo_reach_info.colocations,
            request_info.colo_id,
            date);

          add_total_appearance(
            isp_colo_reach_info.colocations,
            request_info.colo_id,
            isp_date);
        }

        if(save_colo ||
           colo_appeared ||
           isp_colo_appeared)
        {
          gmt_colo_reach_info.create_time =
            Generics::Time(profile_writer.create_time());
          gmt_colo_reach_info.household = HOUSEHOLD_;

          Generics::Time isp_create_time;
          bool found_id_profile_writer = get_time_by_id(
            profile_writer.isp_colo_create_time().begin(),
            profile_writer.isp_colo_create_time().end(),
            request_info.colo_id,
            isp_create_time);
          const Generics::Time isp_create_date = Algs::round_to_day(isp_create_time);

          if (!found_id_profile_writer)
          {
            // smth goes wrong. at this point in profile writer
            //   isp_colo_create_time must contain element with request_info.colo_id
            Stream::Error ostr;
            ostr << FUN << "request_info.colo_id: " << request_info.colo_id <<
              " not found in profile_writer.isp_colo_create_time";
            throw Exception(ostr);
          }

          isp_colo_reach_info.create_time = isp_create_date;
          isp_colo_reach_info.household = HOUSEHOLD_;

          gmt_colo_reach_info_list.push_back(gmt_colo_reach_info);
          isp_colo_reach_info_list.push_back(isp_colo_reach_info);

          if(colo_appeared)
          {
            update_appearance(
              profile_writer.colo_appears(),
              request_info.colo_id,
              date,
              BaseAppearanceLess(),
              BaseInsertAppearanceAdapter<ColoIdAppearanceWriter>());
          }

          if(isp_colo_appeared)
          {
            update_appearance(
              profile_writer.isp_colo_appears(),
              request_info.colo_id,
              isp_date,
              BaseAppearanceLess(),
              BaseInsertAppearanceAdapter<ColoIdAppearanceWriter>());
          }
        }

        /* save profile */
        try
        {
          Generics::SmartMemBuf_var new_mem_buf(
            new Generics::SmartMemBuf(profile_writer.size()));

          profile_writer.save(
            new_mem_buf->membuf().data(), new_mem_buf->membuf().size());

          transaction->save_profile(
            Generics::transfer_membuf(new_mem_buf),
            request_info.time);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": on save, caught eh::Exception: " << ex.what();
          throw Exception(ostr);
        }
      }
      catch(const PlainTypes::CorruptedStruct& ex)
      {
        transaction->remove_profile();

        Stream::Error ostr;
        ostr << FUN << ": Caught PlainTypes::CorruptedStruct: " << ex.what();
        throw Exception(ostr);
      }
    }
  }
}

