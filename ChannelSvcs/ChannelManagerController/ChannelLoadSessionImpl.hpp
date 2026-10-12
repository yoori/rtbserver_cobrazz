#pragma once

#include <Generics/TaskRunner.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <ChannelSvcs/ChannelServer/ChannelUpdateGrpcClient.hpp>

namespace AdServer::ChannelSvcs
{
  class ChannelLoadSessionImpl final : public ChannelUpdateSource
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    ChannelLoadSessionImpl(
      const GroupLoadDescription& servers, long source, Logging::Logger* logger);
    ~ChannelLoadSessionImpl() noexcept override;

    void check(const ChannelUpdate::CheckQuery& query, ChannelUpdate::CheckDataPtr& data)
      override;
    void update_triggers(const ChannelIdList& ids, ChannelUpdate::UpdateDataPtr& result) override;
    void update_all_ccg(
      const ChannelUpdate::CCGQuery& query, ChannelUpdate::PosCCGResultPtr& result) override;
    std::uint32_t get_count_chunks() override;

  private:
    size_t get_item_() noexcept;
    void bad_(size_t index) noexcept;
    GroupLoadDescription& load_servers() noexcept;
    long source_id() const noexcept;

    using Mutex_ = Sync::PosixRWLock;
    using RGuard_ = Sync::PosixRGuard;
    using WGuard_ = Sync::PosixWGuard;

    GroupLoadDescription servers_;
    long source_id_;
    Generics::ActiveObjectCallback_var callback_;
    Generics::TaskRunner_var task_runner_;
    Mutex_ init_lock_;
    size_t index_ = 0;
  };

  template<class STREAM>
  STREAM& describe_description(STREAM& ostr, const GroupLoadDescription& servers)
  {
    ostr << "servers:";
    for (size_t group = 0; group < servers.size(); ++group)
    {
      for (size_t server = 0; server < servers[group].size(); ++server)
      {
        ostr << ' ' << group << '.' << server;
      }
    }

    return ostr;
  }
}
