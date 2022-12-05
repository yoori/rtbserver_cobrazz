#ifndef CHANNELSEARCHSERVICE_CHANNELMATCHER_HPP
#define CHANNELSEARCHSERVICE_CHANNELMATCHER_HPP

#include <set>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>

#include <Commons/UserInfoManip.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelIndex.hpp>

namespace AdServer
{
namespace ChannelSearchSvcs
{
  class ChannelMatcher:
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      
    typedef CampaignSvcs::ExpressionChannelHolderMap ChannelMap;
    typedef AdServer::CampaignSvcs::ChannelIdSet ChannelIdSet;

    typedef std::set<unsigned long> CCGIdSet;

    struct Config: public virtual ReferenceCounting::AtomicImpl
    {
    public:
      struct ChannelTraits
      {
        CCGIdSet ccg_ids;
      };

      typedef std::map<unsigned long, ChannelTraits> ChannelTraitsMap;

    public:
      ChannelMap expression_channels;
      ChannelTraitsMap expression_channel_traits_map;

    protected:
      virtual ~Config() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<Config> Config_var;

    struct ChannelMatchResult
    {
      CCGIdSet ccg_ids;
      ChannelIdSet matched_simple_channels;
    };

    typedef std::map<unsigned long, ChannelMatchResult>
      ChannelMatchResultMap;

  public:
    ChannelMatcher() noexcept;

    virtual ~ChannelMatcher() noexcept {};

    Config_var config() const /*throw(Exception)*/;

    void config(Config* config) /*throw(Exception)*/;

    void match(
      ChannelMatchResultMap& result,
      const ChannelIdSet& history_channels)
      /*throw(Exception)*/;

    void search(
      ChannelIdSet& result_channels,
      const ChannelIdSet& history_channels)
      /*throw(Exception)*/;

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;
    typedef AdServer::CampaignSvcs::ExpressionChannelIndex_var
      ExpressionChannelIndex_var;

    struct ExpressionChannelSearchIndex: public ReferenceCounting::AtomicImpl
    {
      typedef std::map<unsigned long, ChannelIdSet> ChannelSearchMap;
      ChannelSearchMap channel_search_map;

    protected:
      virtual ~ExpressionChannelSearchIndex() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<ExpressionChannelSearchIndex>
      ExpressionChannelSearchIndex_var;

  private:
    ExpressionChannelIndex_var get_channel_index_() const
      /*throw(Exception)*/;

    ExpressionChannelSearchIndex_var
    get_channel_search_index_() const /*throw(Exception)*/;

  private:
    Logging::Logger_var logger_;
    mutable SyncPolicy::Mutex lock_;
    Config_var config_;
    ExpressionChannelIndex_var channel_index_;
    ExpressionChannelSearchIndex_var channel_search_index_;
  };

  typedef ReferenceCounting::SmartPtr<ChannelMatcher>
    ChannelMatcher_var;
}
}

#endif /*CHANNELSEARCHSERVICE_CHANNELMATCHER_HPP*/
