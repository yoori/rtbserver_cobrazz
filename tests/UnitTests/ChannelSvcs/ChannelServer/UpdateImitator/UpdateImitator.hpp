
#ifndef _AD_SERVER_TEST_CHANNEL_UPDATE_IMPL_HPP_
#define _AD_SERVER_TEST_CHANNEL_UPDATE_IMPL_HPP_

#include <eh/Exception.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/ProcessControlImpl.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUpdateBase.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUpdateBase_s.hpp>

namespace AdServer
{
namespace UnitTests
{
  typedef ::AdServer::ChannelSvcs::ChannelUpdateBase_v33 ChannelCurrent;

  class UpdateImitator:
    public virtual CORBACommons::ReferenceCounting::ServantImpl
      <POA_AdServer::ChannelSvcs::ChannelUpdate_v33>
  {
  public:
    enum Algorithm
    {
      ALG_RANDOM,
      ALG_CONST,
      ALG_GROW,
      ERROR
    };


    UpdateImitator(
      Algorithm alg,
      size_t count_channels,
      size_t count_triggers,
      size_t count_keywords,
      char trigger_type,
      size_t trigger_length,
      size_t start_point,
      size_t grow_step,
      bool dump_memory_stat)
      noexcept;

    //
    // IDL:AdServer/ChannelSvcs/ChannelCurrent/check:1.0
    //
    virtual void check(
      const ChannelCurrent::CheckQuery& query,
      ChannelCurrent::CheckData_out data)
      /*throw(AdServer::ChannelSvcs::ImplementationException,
            AdServer::ChannelSvcs::NotConfigured)*/;

    //
    // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_triggers:1.0
    //
    virtual void update_triggers(
        const ChannelSvcs::ChannelIdSeq& ids,
        ChannelCurrent::UpdateData_out result)
        /*throw(AdServer::ChannelSvcs::ImplementationException,
              AdServer::ChannelSvcs::NotConfigured)*/;

    //
    // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
    //
    virtual ::CORBA::ULong get_count_chunks()
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/;

    //
    // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_all_ccg:1.0
    //
    virtual void update_all_ccg(
      const ChannelCurrent::CCGQuery& query,
      ChannelCurrent::PosCCGResult_out result)
      /*throw(AdServer::ChannelSvcs::ImplementationException,
            AdServer::ChannelSvcs::NotConfigured)*/;

    const std::string control(const char* param_name, const char* parma_value);


    static Algorithm parse_alg_name(const std::string& name) noexcept;

  protected:
    typedef Sync::PosixRWLock Mutex_;
    typedef Sync::PosixRGuard ReadGuard_;
    typedef Sync::PosixWGuard WriteGuard_;

    virtual ~UpdateImitator() noexcept;

    void generate_triggers_() noexcept;
  private:
    typedef std::map<unsigned int, std::string> TriggerMap;
    int alg_;
    size_t count_channels_;
    size_t count_triggers_;
    size_t count_keywords_;
    size_t trigger_length_;
    size_t start_point_;
    size_t grow_step_;
    char trigger_type_;
    size_t count_channels_i_;
    TriggerMap triggers_;
    bool dump_memory_stat_;

    mutable Mutex_ lock_triggers_;
  };

  typedef ReferenceCounting::SmartPtr<UpdateImitator> UpdateImitator_var;

  class ControlImpl: public CORBACommons::ProcessControlImpl
  {
  public:
    ControlImpl(
      CORBACommons::OrbShutdowner* shutdowner,
      UpdateImitator* server) noexcept
      : CORBACommons::ProcessControlImpl(shutdowner),
        server_(ReferenceCounting::add_ref(server)) {};

    virtual
    char*
    control(const char* param_name, const char* param_value)
      /*throw(CORBACommons::OutOfMemory, CORBACommons::ImplementationError)*/;

  protected:
    virtual ~ControlImpl() noexcept {};

  private:
    UpdateImitator_var server_;
  };

  typedef ReferenceCounting::SmartPtr<ControlImpl> ControlImpl_var;

}
}
#endif //_AD_SERVER_TEST_CHANNEL_UPDATE_IMPL_HPP_

